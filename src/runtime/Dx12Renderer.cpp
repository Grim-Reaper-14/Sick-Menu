#include "Dx12Renderer.hpp"

#include "ui/menu/ImGuiMenuBackend.hpp"
#include "game/scripts/ScriptFunctionCatalog.hpp"
#include "RuntimeLog.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <utility>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Sick::Runtime
{
    Dx12Renderer::Dx12Renderer() :
        m_Menu(Ui::SickMenuCallbacks{
            .regularAction = [] {
                const auto* specification = Game::Scripts::ScriptFunctionCatalog::Find(
                    Game::Scripts::KnownScriptFunction::GetFmmcVariationCount);
                if (!specification)
                    return;
                const auto function = specification->Bind();
                const auto result = function.TryCall<int>();
                OutputDebugStringW(result
                    ? L"[SickMenu] Script VM test succeeded\n"
                    : L"[SickMenu] Script VM test unavailable (freemode not loaded or signature changed)\n");
            }})
    {
    }

    bool Dx12Renderer::Initialize(IDXGISwapChain* swapChain, ID3D12CommandQueue* queue, HWND window)
    {
        Log::Write("initializing D3D12 renderer");
        if (m_Ready || !swapChain || !queue || !window)
            return m_Ready;

        if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain))) ||
            FAILED(m_SwapChain->GetDevice(IID_PPV_ARGS(&m_Device))))
        {
            Log::Write("failed to query IDXGISwapChain3 or ID3D12Device");
            return false;
        }

        m_Queue = queue;
        m_Window = window;

        DXGI_SWAP_CHAIN_DESC description{};
        if (FAILED(m_SwapChain->GetDesc(&description)) || description.BufferCount < 2)
            return false;
        m_Format = description.BufferDesc.Format;
        m_Frames.resize(description.BufferCount);

        D3D12_DESCRIPTOR_HEAP_DESC rtvDescription{};
        rtvDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDescription.NumDescriptors = description.BufferCount;
        if (FAILED(m_Device->CreateDescriptorHeap(&rtvDescription, IID_PPV_ARGS(&m_RtvHeap))))
            return false;

        D3D12_DESCRIPTOR_HEAP_DESC srvDescription{};
        srvDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDescription.NumDescriptors = 1;
        srvDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(m_Device->CreateDescriptorHeap(&srvDescription, IID_PPV_ARGS(&m_SrvHeap))))
            return false;

        for (auto& frame : m_Frames)
        {
            if (FAILED(m_Device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&frame.allocator))))
                return false;
        }
        if (FAILED(m_Device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_Frames.front().allocator.Get(),
                nullptr,
                IID_PPV_ARGS(&m_CommandList))) ||
            FAILED(m_CommandList->Close()) ||
            FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence))))
            return false;

        m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!m_FenceEvent || !CreateRenderTargets())
            return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        ImGui_ImplDX12_InitInfo initInfo{};
        initInfo.Device = m_Device.Get();
        initInfo.CommandQueue = m_Queue.Get();
        initInfo.NumFramesInFlight = static_cast<int>(m_Frames.size());
        initInfo.RTVFormat = m_Format;
        initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
        initInfo.SrvDescriptorHeap = m_SrvHeap.Get();
        initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
                                               D3D12_CPU_DESCRIPTOR_HANDLE* cpu,
                                               D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
            *cpu = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            *gpu = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        };
        initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*,
                                              D3D12_CPU_DESCRIPTOR_HANDLE,
                                              D3D12_GPU_DESCRIPTOR_HANDLE) {};
        if (!ImGui_ImplWin32_Init(m_Window) || !ImGui_ImplDX12_Init(&initInfo))
            return false;

        m_Ready = true;
        Log::Write("D3D12 renderer ready; press F4 to open the menu");
        return true;
    }

    bool Dx12Renderer::CreateRenderTargets()
    {
        const auto increment = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto handle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (std::size_t index = 0; index < m_Frames.size(); ++index)
        {
            auto& frame = m_Frames[index];
            frame.rtv = handle;
            if (FAILED(m_SwapChain->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&frame.backBuffer))))
                return false;
            m_Device->CreateRenderTargetView(frame.backBuffer.Get(), nullptr, frame.rtv);
            handle.ptr += increment;
        }
        return true;
    }

    void Dx12Renderer::WaitForFrame(Frame& frame)
    {
        if (!frame.fenceValue || m_Fence->GetCompletedValue() >= frame.fenceValue)
            return;
        if (SUCCEEDED(m_Fence->SetEventOnCompletion(frame.fenceValue, m_FenceEvent)))
            WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    void Dx12Renderer::WaitForGpu()
    {
        if (!m_Queue || !m_Fence || !m_FenceEvent)
            return;
        const auto value = m_NextFenceValue++;
        if (SUCCEEDED(m_Queue->Signal(m_Fence.Get(), value)) &&
            m_Fence->GetCompletedValue() < value &&
            SUCCEEDED(m_Fence->SetEventOnCompletion(value, m_FenceEvent)))
            WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    void Dx12Renderer::PollKeyboardFallback()
    {
        const auto pressed = [](int virtualKey) {
            return (GetAsyncKeyState(virtualKey) & 1) != 0;
        };

        auto& controller = m_Menu.Controller();
        if (pressed(VK_F4))
            controller.Handle(Ui::MenuInput::Toggle);
        if (!controller.IsOpen())
            return;
        if (pressed(VK_UP))
            controller.Handle(Ui::MenuInput::Up);
        if (pressed(VK_DOWN))
            controller.Handle(Ui::MenuInput::Down);
        if (pressed(VK_LEFT))
            controller.Handle(Ui::MenuInput::Left);
        if (pressed(VK_RIGHT))
            controller.Handle(Ui::MenuInput::Right);
        if (pressed(VK_RETURN))
            controller.Handle(Ui::MenuInput::Select);
        if (pressed(VK_BACK))
            controller.Handle(Ui::MenuInput::Back);
    }

    void Dx12Renderer::Render(bool pollKeyboardFallback)
    {
        if (!m_Ready)
            return;
        auto& frame = m_Frames[m_SwapChain->GetCurrentBackBufferIndex()];
        WaitForFrame(frame);
        if (FAILED(frame.allocator->Reset()) || FAILED(m_CommandList->Reset(frame.allocator.Get(), nullptr)))
            return;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (pollKeyboardFallback)
        {
            PollKeyboardFallback();
            const auto displaySize = ImGui::GetIO().DisplaySize;
            Ui::ImGuiMenuBackend::Submit(m_Menu.Draw({displaySize.x, displaySize.y}));
        }
        else
        {
            Ui::ImGuiMenuKeys keys{};
            keys.toggle = ImGuiKey_F4;
            Sick::Ui::ImGuiMenuBackend::Render(m_Menu, keys);
        }
        ImGui::Render();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = frame.backBuffer.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_CommandList->ResourceBarrier(1, &barrier);
        m_CommandList->OMSetRenderTargets(1, &frame.rtv, FALSE, nullptr);
        ID3D12DescriptorHeap* heaps[]{m_SrvHeap.Get()};
        m_CommandList->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
        m_CommandList->ResourceBarrier(1, &barrier);
        if (FAILED(m_CommandList->Close()))
            return;
        ID3D12CommandList* commandLists[]{m_CommandList.Get()};
        m_Queue->ExecuteCommandLists(1, commandLists);
        frame.fenceValue = m_NextFenceValue++;
        static_cast<void>(m_Queue->Signal(m_Fence.Get(), frame.fenceValue));
    }

    void Dx12Renderer::BeforeResize()
    {
        if (!m_Ready)
            return;
        WaitForGpu();
        ImGui_ImplDX12_InvalidateDeviceObjects();
        for (auto& frame : m_Frames)
            frame.backBuffer.Reset();
    }

    bool Dx12Renderer::AfterResize()
    {
        if (!m_Ready)
            return false;
        DXGI_SWAP_CHAIN_DESC description{};
        if (FAILED(m_SwapChain->GetDesc(&description)) || description.BufferCount != m_Frames.size())
            return false;
        m_Format = description.BufferDesc.Format;
        return CreateRenderTargets() && ImGui_ImplDX12_CreateDeviceObjects();
    }

    LRESULT Dx12Renderer::HandleWindowMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        return m_Ready ? ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam) : 0;
    }

    void Dx12Renderer::Shutdown()
    {
        Log::Write("shutting down D3D12 renderer");
        if (m_Ready)
        {
            WaitForGpu();
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        m_Ready = false;
        m_Frames.clear();
        m_CommandList.Reset();
        m_RtvHeap.Reset();
        m_SrvHeap.Reset();
        m_Fence.Reset();
        m_Device.Reset();
        m_Queue.Reset();
        m_SwapChain.Reset();
        if (m_FenceEvent)
        {
            CloseHandle(m_FenceEvent);
            m_FenceEvent = nullptr;
        }
    }
}
