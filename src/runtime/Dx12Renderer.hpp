#pragma once

#include "frontend/FrontendCore.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>

struct ImFont;
struct ID3D12CommandQueue;
struct IDXGISwapChain;

namespace Sick::Runtime
{
    class Dx12Renderer final
    {
    public:
        Dx12Renderer();
        bool Initialize(IDXGISwapChain* swapChain, ID3D12CommandQueue* queue, HWND window);
        void Render(bool pollKeyboardFallback = false);
        void BeforeResize();
        bool AfterResize();
        void Shutdown();
        [[nodiscard]] bool Ready() const noexcept { return m_Ready; }
        LRESULT HandleWindowMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    private:
        struct Frame
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
            Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
            D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
            std::uint64_t fenceValue{};
        };

        bool CreateRenderTargets();
        void WaitForFrame(Frame& frame);
        void WaitForGpu();
        void PollKeyboardFallback();
        void ApplyMenuAssets(Ui::SickMenu& menu);
        void ApplyFont(Ui::SickMenu& menu, const std::string& path);
        bool LoadBannerTexture(Ui::SickMenu& menu, const std::string& path);
        void ClearBannerTexture(Ui::SickMenu& menu);

        HWND m_Window{};
        bool m_Ready{};
        Frontend::FrontendCore m_Frontend;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_BannerTexture;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_FenceEvent{};
        std::uint64_t m_NextFenceValue{1};
        std::uint64_t m_AppliedAssetGeneration{};
        UINT m_SrvDescriptorSize{};
        DXGI_FORMAT m_Format{DXGI_FORMAT_R8G8B8A8_UNORM};
        std::vector<Frame> m_Frames;
        ImFont* m_MenuFont{};
        std::string m_AppliedBannerPath;
        std::string m_AppliedFontPath;
    };
}
