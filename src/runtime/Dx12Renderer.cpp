#include "Dx12Renderer.hpp"

#include "RuntimeLog.hpp"
#include "backend/tasking/TaskAffinity.hpp"
#include "ui/menu/ImGuiMenuBackend.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>
#include <wincodec.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
    struct DecodedImage
    {
        UINT width{};
        UINT height{};
        std::vector<std::uint8_t> pixels;
    };

    bool DecodeImage(const std::string& path, DecodedImage& image)
    {
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool uninitialize = SUCCEEDED(comResult);
        if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
            return false;

        Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        const auto widePath = std::filesystem::path(path).wstring();

        bool success = SUCCEEDED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))) &&
            SUCCEEDED(factory->CreateDecoderFromFilename(
                widePath.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                &decoder)) &&
            SUCCEEDED(decoder->GetFrame(0, &frame)) &&
            SUCCEEDED(frame->GetSize(&image.width, &image.height)) &&
            image.width != 0 && image.height != 0 &&
            SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
            SUCCEEDED(converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom));

        if (success)
        {
            const auto stride = image.width * 4U;
            const auto byteCount = static_cast<std::size_t>(stride) * image.height;
            image.pixels.resize(byteCount);
            success = SUCCEEDED(converter->CopyPixels(
                nullptr,
                stride,
                static_cast<UINT>(image.pixels.size()),
                image.pixels.data()));
        }

        if (uninitialize)
            CoUninitialize();
        return success;
    }
}

namespace Sick::Runtime
{
    Dx12Renderer::Dx12Renderer() = default;

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
        srvDescription.NumDescriptors = 2;
        srvDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(m_Device->CreateDescriptorHeap(&srvDescription, IID_PPV_ARGS(&m_SrvHeap))))
            return false;
        m_SrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
        m_MenuFont = io.Fonts->AddFontDefault();
        io.FontDefault = m_MenuFont;

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
        Log::Write("D3D12 renderer ready; F4 toggles menu, Numpad 8/2/4/6 navigates, Numpad 5 selects, Numpad 0 goes back");
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

        auto& menu = m_Frontend.Menu();
        if (pressed(VK_F4))
            menu.Handle(Ui::MenuInput::Toggle);
        if (!menu.Controller().IsOpen())
            return;
        if (pressed(VK_NUMPAD8))
            menu.Handle(Ui::MenuInput::Up);
        if (pressed(VK_NUMPAD2))
            menu.Handle(Ui::MenuInput::Down);
        if (pressed(VK_NUMPAD4))
            menu.Handle(Ui::MenuInput::Left);
        if (pressed(VK_NUMPAD6))
            menu.Handle(Ui::MenuInput::Right);
        if (pressed(VK_NUMPAD5))
            menu.Handle(Ui::MenuInput::Select);
        if (pressed(VK_NUMPAD0) || pressed(VK_INSERT) || pressed(VK_BACK))
            menu.Handle(Ui::MenuInput::Back);
    }

    void Dx12Renderer::ApplyMenuAssets(Ui::SickMenu& menu)
    {
        const auto generation = menu.AssetGeneration();
        const bool forceReload = generation != m_AppliedAssetGeneration;
        if (forceReload)
            m_AppliedAssetGeneration = generation;

        const auto fontPath = menu.SelectedFontPath();
        if (forceReload || fontPath != m_AppliedFontPath)
            ApplyFont(menu, fontPath);

        const auto bannerPath = menu.SelectedBannerPath();
        if (!forceReload && bannerPath == m_AppliedBannerPath)
            return;

        ClearBannerTexture(menu);
        m_AppliedBannerPath = bannerPath;
        if (!bannerPath.empty() && !LoadBannerTexture(menu, bannerPath))
            Log::Write("failed to load selected menu banner: " + bannerPath);
    }

    void Dx12Renderer::ApplyFont(Ui::SickMenu&, const std::string& path)
    {
        WaitForGpu();
        auto& io = ImGui::GetIO();
        ImGui_ImplDX12_InvalidateDeviceObjects();
        io.Fonts->Clear();
        m_MenuFont = path.empty() ? io.Fonts->AddFontDefault() : io.Fonts->AddFontFromFileTTF(path.c_str(), 24.0F);
        if (!m_MenuFont)
        {
            Log::Write("failed to load selected menu font; falling back to ImGui default: " + path);
            m_MenuFont = io.Fonts->AddFontDefault();
        }
        io.FontDefault = m_MenuFont;
        if (!ImGui_ImplDX12_CreateDeviceObjects())
            Log::Write("failed to recreate ImGui DX12 font resources");
        m_AppliedFontPath = path;
    }

    bool Dx12Renderer::LoadBannerTexture(Ui::SickMenu& menu, const std::string& path)
    {
        DecodedImage image;
        if (!DecodeImage(path, image))
            return false;

        D3D12_RESOURCE_DESC textureDescription{};
        textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDescription.Width = image.width;
        textureDescription.Height = image.height;
        textureDescription.DepthOrArraySize = 1;
        textureDescription.MipLevels = 1;
        textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDescription.SampleDesc.Count = 1;
        textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        D3D12_HEAP_PROPERTIES defaultHeap{};
        defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeap.CreationNodeMask = 1;
        defaultHeap.VisibleNodeMask = 1;

        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        if (FAILED(m_Device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &textureDescription,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&texture))))
            return false;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT rows{};
        UINT64 rowSize{};
        UINT64 uploadBytes{};
        m_Device->GetCopyableFootprints(
            &textureDescription, 0, 1, 0, &footprint, &rows, &rowSize, &uploadBytes);

        D3D12_RESOURCE_DESC bufferDescription{};
        bufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDescription.Width = uploadBytes;
        bufferDescription.Height = 1;
        bufferDescription.DepthOrArraySize = 1;
        bufferDescription.MipLevels = 1;
        bufferDescription.SampleDesc.Count = 1;
        bufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES uploadHeap{};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeap.CreationNodeMask = 1;
        uploadHeap.VisibleNodeMask = 1;

        Microsoft::WRL::ComPtr<ID3D12Resource> upload;
        if (FAILED(m_Device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &bufferDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&upload))))
            return false;

        std::uint8_t* mapped{};
        D3D12_RANGE readRange{0, 0};
        if (FAILED(upload->Map(0, &readRange, reinterpret_cast<void**>(&mapped))))
            return false;
        const auto sourceStride = static_cast<std::size_t>(image.width) * 4U;
        for (UINT row = 0; row < image.height; ++row)
        {
            std::memcpy(
                mapped + footprint.Offset + static_cast<std::size_t>(row) * footprint.Footprint.RowPitch,
                image.pixels.data() + static_cast<std::size_t>(row) * sourceStride,
                sourceStride);
        }
        upload->Unmap(0, nullptr);

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
        if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))) ||
            FAILED(m_Device->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list))))
            return false;

        D3D12_TEXTURE_COPY_LOCATION destination{};
        destination.pResource = texture.Get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destination.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION source{};
        source.pResource = upload.Get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint = footprint;
        list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        list->ResourceBarrier(1, &barrier);
        if (FAILED(list->Close()))
            return false;

        ID3D12CommandList* commands[]{list.Get()};
        m_Queue->ExecuteCommandLists(1, commands);
        WaitForGpu();

        auto cpu = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
        auto gpu = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
        cpu.ptr += m_SrvDescriptorSize;
        gpu.ptr += m_SrvDescriptorSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Texture2D.MipLevels = 1;
        m_Device->CreateShaderResourceView(texture.Get(), &view, cpu);

        m_BannerTexture = std::move(texture);
        menu.SetHeaderTexture(static_cast<Ui::MenuTexture>(gpu.ptr));
        return true;
    }

    void Dx12Renderer::ClearBannerTexture(Ui::SickMenu& menu)
    {
        if (m_BannerTexture)
            WaitForGpu();
        menu.SetHeaderTexture(0);
        m_BannerTexture.Reset();
    }

    void Dx12Renderer::Render(bool pollKeyboardFallback)
    {
        Backend::Tasking::ScopedTaskAffinity affinity{Backend::Tasking::TaskAffinity::Render};
        if (!m_Ready)
            return;
        auto& frame = m_Frames[m_SwapChain->GetCurrentBackBufferIndex()];
        WaitForFrame(frame);

        auto& menu = m_Frontend.Menu();
        ApplyMenuAssets(menu);

        if (FAILED(frame.allocator->Reset()) || FAILED(m_CommandList->Reset(frame.allocator.Get(), nullptr)))
            return;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (pollKeyboardFallback)
        {
            PollKeyboardFallback();
            const auto displaySize = ImGui::GetIO().DisplaySize;
            Ui::ImGuiMenuBackend::Submit(menu.Draw({displaySize.x, displaySize.y}), nullptr, m_MenuFont);
        }
        else
        {
            Ui::ImGuiMenuBackend::Render(menu, {}, nullptr, m_MenuFont);
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
            ClearBannerTexture(m_Frontend.Menu());
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        m_Ready = false;
        m_MenuFont = nullptr;
        m_AppliedAssetGeneration = 0;
        m_AppliedBannerPath.clear();
        m_AppliedFontPath.clear();
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
        m_Window = nullptr;
    }
}
