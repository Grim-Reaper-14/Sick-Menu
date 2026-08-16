#include "GtaRuntime.hpp"

#include "PatternScanner.hpp"
#include "RuntimeLog.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/EnhancedScriptHost.hpp"
#include "game/natives/NativeContext.hpp"
#include "game/natives/generated/NativeHashes.hpp"

#include <MinHook.h>
#include <array>
#include <chrono>
#include <cwchar>
#include <string>
#include <thread>

namespace
{
    using NativeHandler = Sick::Game::Natives::NativeHandler;
    std::array<NativeHandler, Sick::Game::Natives::NativeCount> g_RuntimeNativeHandlers{};

    NativeHandler ResolveRuntimeNative(Sick::Game::NativeHash hash, Sick::Game::Enhanced::BuildId)
    {
        const auto index = Sick::Game::Natives::Generated::IndexForHash(hash);
        const auto offset = Sick::Game::Natives::ToNativeOffset(index);
        return offset < g_RuntimeNativeHandlers.size() ? g_RuntimeNativeHandlers[offset] : nullptr;
    }
}

namespace
{
    constexpr std::wstring_view GameExecutable = L"GTA5_Enhanced.exe";

    [[nodiscard]] bool IsEnhancedProcess()
    {
        std::array<wchar_t, MAX_PATH> path{};
        if (!GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size())))
            return false;
        const auto* filename = std::wcsrchr(path.data(), L'\\');
        filename = filename ? filename + 1 : path.data();
        return _wcsicmp(filename, GameExecutable.data()) == 0;
    }

    struct WindowSearch
    {
        DWORD processId{};
        HWND result{};
    };

    BOOL CALLBACK FindProcessWindow(HWND window, LPARAM parameter)
    {
        auto& search = *reinterpret_cast<WindowSearch*>(parameter);
        DWORD processId{};
        GetWindowThreadProcessId(window, &processId);
        if (processId == search.processId && IsWindowVisible(window) && GetWindow(window, GW_OWNER) == nullptr)
        {
            search.result = window;
            return FALSE;
        }
        return TRUE;
    }

    [[nodiscard]] HWND FindGameWindow()
    {
        WindowSearch search{GetCurrentProcessId(), nullptr};
        EnumWindows(&FindProcessWindow, reinterpret_cast<LPARAM>(&search));
        return search.result;
    }

    bool CreateAndLogHook(const char* name, void* target, void* detour, void** original)
    {
        const auto status = MH_CreateHook(target, detour, original);
        if (status == MH_OK)
        {
            Sick::Runtime::Log::Write(std::string{name} + " hook created");
            return true;
        }
        Sick::Runtime::Log::Write(
            std::string{name} + " hook creation failed, MinHook status=" + std::to_string(static_cast<int>(status)));
        return false;
    }
}

namespace Sick::Runtime
{
    bool GtaRuntime::ResolveGamePointers()
    {
        Log::Write("scanning GTA5_Enhanced.exe patterns");
        if (!IsEnhancedProcess())
        {
            Log::Write("refusing to initialize outside GTA5_Enhanced.exe");
            return false;
        }

        const auto image = LoadModuleImage(GameExecutable);
        if (!image)
            return false;
        const auto swapMatch = FindUnique(*image, "72 C7 EB 02 31 C0 8B 0D");
        const auto runMatch = FindUnique(*image, "BE 40 5D C6 00");
        const auto nativeMatch = FindUnique(*image, "EB 2A 0F 1F 40 00 48 8B 54 17 10");
        if (!swapMatch || !runMatch || !nativeMatch ||
            *runMatch < image->base + 0xA || *nativeMatch < image->base + 0x2A)
            return false;
        const auto queueAddress = Rip(*image, *swapMatch + 0x1D);
        const auto swapAddress = Rip(*image, *swapMatch + 0x24);
        if (!queueAddress || !swapAddress)
            return false;

        m_CommandQueueAddress = reinterpret_cast<ID3D12CommandQueue**>(*queueAddress);
        m_SwapChainAddress = reinterpret_cast<IDXGISwapChain**>(*swapAddress);
        m_RunScriptThreadsAddress = reinterpret_cast<void*>(*runMatch - 0xA);
        m_InitNativeTablesAddress = reinterpret_cast<void*>(*nativeMatch - 0x2A);
        m_Window = FindGameWindow();
        Log::Write(m_Window ? "GTA window and core patterns resolved" : "GTA window was not found");
        return m_CommandQueueAddress && m_SwapChainAddress && m_RunScriptThreadsAddress && m_Window;
    }

    bool GtaRuntime::InitializeNativeBackend()
    {
        Log::Write("initializing native backend through InitNativeTables");
        using InitNativeTablesFn = void (*)(Game::Enhanced::LiveScriptProgram*);
        if (!m_InitNativeTablesAddress)
            return false;

        std::array<std::uint64_t, Game::Natives::NativeCount> entries{
            0x259BE71D8A81D4FAULL,
            0x566C977EEAE1C0D1ULL,
            0xF3A281B1AA86DBA9ULL,
            0xD25E9BDC14A0B649ULL,
        };
        Game::Enhanced::LiveScriptProgram program{};
        program.nativeCount = static_cast<std::uint32_t>(entries.size());
        program.nativeEntrypoints = entries.data();
        reinterpret_cast<InitNativeTablesFn>(m_InitNativeTablesAddress)(&program);

        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            if (!entries[index])
                return false;
            g_RuntimeNativeHandlers[index] = reinterpret_cast<NativeHandler>(entries[index]);
        }

        constexpr Game::Enhanced::BuildId ReferenceEnhancedBuild = 115813;
        const bool initialized = Game::Enhanced::EnhancedGame::InitializeIndexed(
            ReferenceEnhancedBuild,
            &ResolveRuntimeNative);
        Log::Write(initialized ? "native backend ready" : "native backend initialization failed");
        return initialized;
    }

    bool GtaRuntime::InstallHooks()
    {
        if (!m_SwapChainAddress || !m_CommandQueueAddress || !*m_SwapChainAddress || !*m_CommandQueueAddress)
            return false;
        void** vtable = *reinterpret_cast<void***>(*m_SwapChainAddress);
        if (!vtable)
            return false;

        const auto initStatus = MH_Initialize();
        if (initStatus != MH_OK)
        {
            Log::Write("MinHook initialization failed, status=" + std::to_string(static_cast<int>(initStatus)));
            return false;
        }
        m_MinHookInitialized = true;
        Log::Write("MinHook initialized");

        if (!CreateAndLogHook("Present", vtable[8], reinterpret_cast<void*>(&PresentHook), reinterpret_cast<void**>(&m_OriginalPresent)) ||
            !CreateAndLogHook("ResizeBuffers", vtable[13], reinterpret_cast<void*>(&ResizeBuffersHook), reinterpret_cast<void**>(&m_OriginalResizeBuffers)) ||
            !CreateAndLogHook("RunScriptThreads", m_RunScriptThreadsAddress, reinterpret_cast<void*>(&RunScriptThreadsHook), reinterpret_cast<void**>(&m_OriginalRunScriptThreads)))
            return false;

        const auto enableStatus = MH_EnableHook(MH_ALL_HOOKS);
        if (enableStatus != MH_OK)
        {
            Log::Write("MinHook enable failed, status=" + std::to_string(static_cast<int>(enableStatus)));
            return false;
        }
        Log::Write("Present, ResizeBuffers, and RunScriptThreads hooks enabled");

        SetLastError(ERROR_SUCCESS);
        const auto previous = SetWindowLongPtrW(
            m_Window,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&WindowProc));
        const auto windowProcError = GetLastError();
        if (!previous && windowProcError != ERROR_SUCCESS)
        {
            m_OriginalWindowProc = nullptr;
            m_WndProcInstalled = false;
            Log::Write(
                "WndProc subclass failed, Windows error=" + std::to_string(static_cast<unsigned long>(windowProcError)) +
                "; continuing with render-thread keyboard polling fallback");
        }
        else
        {
            m_OriginalWindowProc = reinterpret_cast<WNDPROC>(previous);
            m_WndProcInstalled = true;
            Log::Write("WndProc subclass installed");
        }
        return true;
    }

    bool GtaRuntime::Initialize()
    {
        if (s_Instance)
            return false;
        s_Instance = this;

        for (int attempt = 0; attempt < 120 && !StopRequested(); ++attempt)
        {
            if (ResolveGamePointers() && m_SwapChainAddress && m_CommandQueueAddress &&
                *m_SwapChainAddress && *m_CommandQueueAddress)
                break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (!m_SwapChainAddress || !m_CommandQueueAddress || !*m_SwapChainAddress || !*m_CommandQueueAddress)
        {
            Log::Write("GTA Enhanced swap chain/window discovery failed");
            s_Instance = nullptr;
            return false;
        }

        Log::Write("initializing Enhanced script host");
        const bool scriptHostReady = Game::Enhanced::EnhancedGame::InitializeScriptHost();
        Log::Write(scriptHostReady
            ? "Enhanced script host ready"
            : "Enhanced script host not ready yet; game-thread hook will retry");
        if (!InstallHooks())
        {
            Log::Write("hook installation failed");
            Shutdown();
            return false;
        }
        Log::Write("DLL initialized; press F4 for the menu and End to unload");
        return true;
    }

    HRESULT __stdcall GtaRuntime::PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
    {
        auto* runtime = s_Instance;
        if (runtime && !runtime->StopRequested())
        {
            std::scoped_lock lock(runtime->m_RenderMutex);
            if (!runtime->m_Renderer.Ready())
                static_cast<void>(runtime->m_Renderer.Initialize(swapChain, *runtime->m_CommandQueueAddress, runtime->m_Window));
            runtime->m_Renderer.Render(!runtime->m_WndProcInstalled);
        }
        return runtime && runtime->m_OriginalPresent
            ? runtime->m_OriginalPresent(swapChain, syncInterval, flags)
            : E_FAIL;
    }

    HRESULT __stdcall GtaRuntime::ResizeBuffersHook(
        IDXGISwapChain* swapChain,
        UINT bufferCount,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT flags)
    {
        auto* runtime = s_Instance;
        if (!runtime || !runtime->m_OriginalResizeBuffers)
            return E_FAIL;
        std::scoped_lock lock(runtime->m_RenderMutex);
        runtime->m_Renderer.BeforeResize();
        const auto result = runtime->m_OriginalResizeBuffers(
            swapChain, bufferCount, width, height, format, flags);
        if (SUCCEEDED(result))
            static_cast<void>(runtime->m_Renderer.AfterResize());
        return result;
    }

    bool GtaRuntime::RunScriptThreadsHook(int operationsToExecute)
    {
        auto* runtime = s_Instance;
        if (!runtime || !runtime->m_OriginalRunScriptThreads)
            return false;
        const bool result = runtime->m_OriginalRunScriptThreads(operationsToExecute);
        if (!runtime->StopRequested())
        {
            std::scoped_lock lock(runtime->m_GameMutex);
            if (!Game::Enhanced::EnhancedGame::Ready() && ++runtime->m_ScriptHostRetry >= 300)
            {
                runtime->m_ScriptHostRetry = 0;
                static_cast<void>(runtime->InitializeNativeBackend());
            }
            if (!Game::Enhanced::EnhancedGame::ScriptFunctionsReady() && ++runtime->m_ScriptHostRetry >= 300)
            {
                runtime->m_ScriptHostRetry = 0;
                static_cast<void>(Game::Enhanced::EnhancedGame::InitializeScriptHost());
            }
            Game::Enhanced::EnhancedGame::Tick();
        }
        return result;
    }

    LRESULT CALLBACK GtaRuntime::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        auto* runtime = s_Instance;
        if (runtime)
        {
            std::scoped_lock lock(runtime->m_RenderMutex);
            static_cast<void>(runtime->m_Renderer.HandleWindowMessage(window, message, wparam, lparam));
        }
        return runtime && runtime->m_WndProcInstalled && runtime->m_OriginalWindowProc
            ? CallWindowProcW(runtime->m_OriginalWindowProc, window, message, wparam, lparam)
            : DefWindowProcW(window, message, wparam, lparam);
    }

    void GtaRuntime::Shutdown()
    {
        Log::Write("runtime shutdown requested");
        m_StopRequested.store(true, std::memory_order_release);
        if (m_MinHookInitialized)
            static_cast<void>(MH_DisableHook(MH_ALL_HOOKS));
        if (m_WndProcInstalled && m_OriginalWindowProc && m_Window && IsWindow(m_Window))
        {
            SetLastError(ERROR_SUCCESS);
            const auto result = SetWindowLongPtrW(
                m_Window,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(m_OriginalWindowProc));
            const auto error = GetLastError();
            if (!result && error != ERROR_SUCCESS)
                Log::Write("WndProc restore failed, Windows error=" + std::to_string(static_cast<unsigned long>(error)));
            else
                Log::Write("WndProc restored");
        }
        m_WndProcInstalled = false;
        m_OriginalWindowProc = nullptr;
        {
            std::scoped_lock lock(m_RenderMutex);
            m_Renderer.Shutdown();
        }
        if (m_MinHookInitialized)
            static_cast<void>(MH_Uninitialize());
        m_MinHookInitialized = false;
        {
            std::scoped_lock lock(m_GameMutex);
            Game::Enhanced::EnhancedGame::Shutdown();
        }
        s_Instance = nullptr;
    }
}
