#pragma once

#include "Dx12Renderer.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <windows.h>

struct ID3D12CommandQueue;
struct IDXGISwapChain;

namespace Sick::Runtime
{
    class GtaRuntime final
    {
    public:
        bool Initialize();
        void Shutdown();
        void RequestStop() noexcept { m_StopRequested.store(true, std::memory_order_release); }
        [[nodiscard]] bool StopRequested() const noexcept { return m_StopRequested.load(std::memory_order_acquire); }

    private:
        using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
        using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
        using RunScriptThreadsFn = bool (*)(int);

        static HRESULT __stdcall PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
        static HRESULT __stdcall ResizeBuffersHook(
            IDXGISwapChain* swapChain,
            UINT bufferCount,
            UINT width,
            UINT height,
            DXGI_FORMAT format,
            UINT flags);
        static bool RunScriptThreadsHook(int operationsToExecute);
        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        [[nodiscard]] bool ResolveGamePointers();
        [[nodiscard]] bool InstallHooks();
        [[nodiscard]] bool InitializeNativeBackend();

        static inline GtaRuntime* s_Instance{};
        std::atomic_bool m_StopRequested{};
        IDXGISwapChain** m_SwapChainAddress{};
        ID3D12CommandQueue** m_CommandQueueAddress{};
        void* m_RunScriptThreadsAddress{};
        void* m_InitNativeTablesAddress{};
        HWND m_Window{};
        WNDPROC m_OriginalWindowProc{};
        PresentFn m_OriginalPresent{};
        ResizeBuffersFn m_OriginalResizeBuffers{};
        RunScriptThreadsFn m_OriginalRunScriptThreads{};
        Dx12Renderer m_Renderer;
        std::mutex m_RenderMutex;
        std::mutex m_GameMutex;
        std::uint32_t m_NativeBackendRetry{};
        std::uint32_t m_ScriptHostRetry{};
        std::uint32_t m_ScriptContextRetry{};
        bool m_NativeBootstrapFinalized{};
        bool m_MinHookInitialized{};
        bool m_WndProcInstalled{};
    };
}
