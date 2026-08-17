#include "GtaRuntime.hpp"

#include "PatternScanner.hpp"
#include "RuntimeLog.hpp"
#include "backend/BackendApi.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/EnhancedScriptHost.hpp"
#include "game/natives/NativeContext.hpp"
#include "game/natives/generated/NativeHashes.hpp"

#include <MinHook.h>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <string>
#include <thread>

namespace
{
    using NativeHandler = Sick::Game::Natives::NativeHandler;

    constexpr std::wstring_view GameExecutable = L"GTA5_Enhanced.exe";
    constexpr Sick::Game::Enhanced::BuildId ReferenceEnhancedBuild = 115813;

    std::array<NativeHandler, Sick::Game::Natives::NativeCount> g_RuntimeNativeHandlers{};

    static_assert(offsetof(Sick::Game::Enhanced::LiveScriptProgram, nativeCount) == 0x2C);
    static_assert(offsetof(Sick::Game::Enhanced::LiveScriptProgram, nativeEntrypoints) == 0x40);

    NativeHandler ResolveRuntimeNative(
        Sick::Game::NativeHash hash,
        Sick::Game::Enhanced::BuildId) noexcept
    {
        const auto index = Sick::Game::Natives::Generated::IndexForHash(hash);
        const auto offset = Sick::Game::Natives::ToNativeOffset(index);
        return offset < g_RuntimeNativeHandlers.size() ? g_RuntimeNativeHandlers[offset] : nullptr;
    }

    [[nodiscard]] bool IsExecutableAddress(const void* address) noexcept
    {
        if (!address)
            return false;

        MEMORY_BASIC_INFORMATION information{};
        if (VirtualQuery(address, &information, sizeof(information)) != sizeof(information))
            return false;
        if (information.State != MEM_COMMIT || (information.Protect & PAGE_GUARD) != 0 ||
            (information.Protect & PAGE_NOACCESS) != 0)
            return false;

        const auto protection = information.Protect & 0xFFU;
        return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
    }

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
        if (!swapMatch || !runMatch || *runMatch < image->base + 0xA)
            return false;
        const auto queueAddress = Rip(*image, *swapMatch + 0x1D);
        const auto swapAddress = Rip(*image, *swapMatch + 0x24);
        if (!queueAddress || !swapAddress)
            return false;

        m_CommandQueueAddress = reinterpret_cast<ID3D12CommandQueue**>(*queueAddress);
        m_SwapChainAddress = reinterpret_cast<IDXGISwapChain**>(*swapAddress);
        m_RunScriptThreadsAddress = reinterpret_cast<void*>(*runMatch - 0xA);
        m_InitNativeTablesAddress = nativeMatch && *nativeMatch >= image->base + 0x2A
            ? reinterpret_cast<void*>(*nativeMatch - 0x2A)
            : nullptr;
        m_Window = FindGameWindow();
        Log::Write(m_Window ? "GTA window and core patterns resolved" : "GTA window was not found");
        if (!m_InitNativeTablesAddress)
            Log::Write("native table pattern unavailable; native backend will remain disabled");
        return m_CommandQueueAddress && m_SwapChainAddress && m_RunScriptThreadsAddress && m_Window;
    }

    bool GtaRuntime::InitializeNativeBackend()
    {
        // This function is intentionally reached from RunScriptThreadsHook rather
        // than the DLL bootstrap thread. InitNativeTables operates on script
        // program metadata, so resolving the scratch table from the game thread
        // keeps it in the same execution context as GTA's script runtime.
        m_NativeBootstrapFinalized = true;
        Log::Write("initializing native backend from game thread");

        using InitNativeTablesFn = void (*)(Game::Enhanced::LiveScriptProgram*);
        if (!m_InitNativeTablesAddress || !IsExecutableAddress(m_InitNativeTablesAddress))
        {
            Log::Write("native backend initialization failed: InitNativeTables address is not executable");
            return false;
        }

        // InitNativeTables expects each native slot to contain the original
        // native hash. GTA replaces each slot in place with the corresponding
        // executable handler. The previous runtime fed a small set of already
        // cross-mapped values instead, which did not match that contract.
        auto entries = Game::Natives::Generated::NativeHashes;
        g_RuntimeNativeHandlers.fill(nullptr);

        Game::Enhanced::LiveScriptProgram program{};
        program.nativeCount = static_cast<std::uint32_t>(entries.size());
        program.nativeEntrypoints = entries.data();

        reinterpret_cast<InitNativeTablesFn>(m_InitNativeTablesAddress)(&program);

        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const auto handlerAddress = reinterpret_cast<void*>(
                static_cast<std::uintptr_t>(entries[index]));
            if (!IsExecutableAddress(handlerAddress))
            {
                g_RuntimeNativeHandlers.fill(nullptr);
                Log::Write(
                    "native backend initialization failed: native index " +
                    std::to_string(index) + " did not resolve to executable memory");
                return false;
            }

            g_RuntimeNativeHandlers[index] = reinterpret_cast<NativeHandler>(handlerAddress);
        }

        const bool initialized = Game::Enhanced::EnhancedGame::InitializeIndexed(
            ReferenceEnhancedBuild,
            &ResolveRuntimeNative);
        if (!initialized)
        {
            g_RuntimeNativeHandlers.fill(nullptr);
            Log::Write("native backend initialization failed: indexed handler table rejected");
            return false;
        }

        Log::Write(
            "native backend ready: " + std::to_string(entries.size()) + "/" +
            std::to_string(Game::Natives::NativeCount) + " handlers resolved");
        return true;
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

        // GTA Enhanced has shown unstable behavior when the injected DLL replaces
        // the game's WndProc. Input is intentionally handled from the render
        // thread instead, which avoids changing the game's window procedure.
        m_OriginalWindowProc = nullptr;
        m_WndProcInstalled = false;
        Log::Write("WndProc subclass disabled; using render-thread keyboard polling fallback");
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

        // Native resolution is deferred until RunScriptThreadsHook has executed
        // inside GTA's script thread. This preserves the stable bootstrap/render
        // behavior while still allowing the native backend to become available.
        m_NativeBootstrapFinalized = false;
        m_NativeBackendRetry = 0;
        Log::Write("native backend resolution deferred to game thread");

        Log::Write("DLL initialized; F4 opens the menu and End unloads the DLL");
        return true;
    }

    HRESULT __stdcall GtaRuntime::PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
    {
        auto* runtime = s_Instance;
        if (runtime && !runtime->StopRequested() && Backend::BackendApi::Get().ExitGtaRequested())
        {
            Log::Write("Exit GTA confirmed from menu; posting WM_CLOSE");
            if (runtime->m_Window && IsWindow(runtime->m_Window))
                PostMessageW(runtime->m_Window, WM_CLOSE, 0, 0);
            runtime->RequestStop();
        }

        if (runtime && !runtime->StopRequested())
        {
            std::scoped_lock lock(runtime->m_RenderMutex);
            bool initializedThisPresent{};
            if (!runtime->m_Renderer.Ready())
            {
                if (runtime->m_CommandQueueAddress && *runtime->m_CommandQueueAddress)
                {
                    initializedThisPresent = runtime->m_Renderer.Initialize(
                        swapChain,
                        *runtime->m_CommandQueueAddress,
                        runtime->m_Window);
                }
            }

            // Never submit our first overlay command list from the same Present
            // invocation that created the renderer resources. Let GTA complete
            // that Present first and begin drawing on the next frame.
            if (runtime->m_Renderer.Ready() && !initializedThisPresent)
                runtime->m_Renderer.Render(true);
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
            if (!Game::Enhanced::EnhancedGame::Ready() && !runtime->m_NativeBootstrapFinalized &&
                ++runtime->m_NativeBackendRetry >= 300)
            {
                runtime->m_NativeBackendRetry = 0;
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
            g_RuntimeNativeHandlers.fill(nullptr);
        }
        m_NativeBootstrapFinalized = false;
        m_NativeBackendRetry = 0;
        m_ScriptHostRetry = 0;
        s_Instance = nullptr;
    }
}
