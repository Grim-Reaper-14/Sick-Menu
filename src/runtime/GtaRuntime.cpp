#include "GtaRuntime.hpp"

#include "PatternScanner.hpp"
#include "RuntimeLog.hpp"
#include "backend/BackendApi.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/EnhancedScriptHost.hpp"
#include "game/enhanced/ScriptGlobal.hpp"
#include "game/natives/NativeContext.hpp"
#include "game/natives/generated/EnhancedNativeHashes.hpp"
#include "game/natives/generated/NativeHashes.hpp"
#include "game/scripts/ScriptTypes.hpp"

#include <MinHook.h>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <string>
#include <thread>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

namespace
{
    using NativeHandler = Sick::Game::Natives::NativeHandler;
    using LiveScriptThread = Sick::Game::Enhanced::LiveScriptThread;
    using LiveScriptThreadArray = Sick::Game::Enhanced::LiveScriptThreadArray;
    using LiveScriptTlsContext = Sick::Game::Enhanced::LiveScriptTlsContext;

    constexpr std::wstring_view GameExecutable = L"GTA5_Enhanced.exe";
    constexpr Sick::Game::Enhanced::BuildId ReferenceEnhancedBuild = 115813;
    constexpr std::size_t GlobalPageShift = 18;
    constexpr std::size_t GlobalPageSize = std::size_t{1} << GlobalPageShift;
    constexpr std::size_t GlobalPageCount = 64;
    constexpr std::size_t GlobalCapacity = GlobalPageSize * GlobalPageCount;
    constexpr auto FreemodeScript = Sick::Game::Scripts::Joaat("freemode");
    constexpr auto MainPersistentScript = Sick::Game::Scripts::Joaat("main_persistent");
    constexpr auto StartupScript = Sick::Game::Scripts::Joaat("startup");

    std::array<NativeHandler, Sick::Game::Natives::NativeCount> g_RuntimeNativeHandlers{};
    LiveScriptThreadArray* g_RuntimeScriptThreads{};
    std::int64_t** g_RuntimeScriptGlobals{};

    static_assert(offsetof(Sick::Game::Enhanced::LiveScriptProgram, nativeCount) == 0x2C);
    static_assert(offsetof(Sick::Game::Enhanced::LiveScriptProgram, nativeEntrypoints) == 0x40);
    static_assert(offsetof(Sick::Game::Enhanced::LiveScriptTlsContext, currentScriptThread) == 0x7A0);

    NativeHandler ResolveRuntimeNative(
        Sick::Game::NativeHash hash,
        Sick::Game::Enhanced::BuildId) noexcept
    {
        const auto index = Sick::Game::Natives::Generated::IndexForHash(hash);
        const auto offset = Sick::Game::Natives::ToNativeOffset(index);
        return offset < g_RuntimeNativeHandlers.size() ? g_RuntimeNativeHandlers[offset] : nullptr;
    }

    void* ResolveRuntimeScriptGlobal(std::size_t index) noexcept
    {
        if (!g_RuntimeScriptGlobals || index >= GlobalCapacity)
            return nullptr;

        const auto page = index >> GlobalPageShift;
        const auto offset = index & (GlobalPageSize - 1);
        auto* pageAddress = g_RuntimeScriptGlobals[page];
        return pageAddress ? static_cast<void*>(pageAddress + offset) : nullptr;
    }

    [[nodiscard]] LiveScriptThread* FindScriptThread(std::uint32_t scriptHash) noexcept
    {
        if (!g_RuntimeScriptThreads || !g_RuntimeScriptThreads->data)
            return nullptr;

        const auto size = g_RuntimeScriptThreads->size;
        const auto capacity = g_RuntimeScriptThreads->capacity;
        if (size == 0 || size > capacity || capacity > 4096)
            return nullptr;

        for (std::uint16_t index = 0; index < size; ++index)
        {
            auto* thread = g_RuntimeScriptThreads->data[index];
            if (thread && thread->context.threadId != 0 && thread->scriptHash == scriptHash)
                return thread;
        }

        return nullptr;
    }

    [[nodiscard]] LiveScriptThread* FindPreferredScriptThread() noexcept
    {
        if (auto* thread = FindScriptThread(FreemodeScript))
            return thread;
        if (auto* thread = FindScriptThread(MainPersistentScript))
            return thread;
        return FindScriptThread(StartupScript);
    }

    [[nodiscard]] LiveScriptTlsContext* ResolveRuntimeTls() noexcept
    {
#if defined(_MSC_VER) && defined(_M_X64)
        const auto tlsStorage = static_cast<std::uintptr_t>(__readgsqword(0x58));
        if (tlsStorage == 0)
            return nullptr;
        return *reinterpret_cast<LiveScriptTlsContext**>(tlsStorage);
#else
        return nullptr;
#endif
    }

    class RuntimeScriptScope final
    {
    public:
        RuntimeScriptScope(LiveScriptTlsContext& tls, LiveScriptThread& thread) noexcept :
            m_Tls(tls),
            m_OriginalThread(tls.currentScriptThread),
            m_OriginalActive(tls.scriptThreadActive)
        {
            m_Tls.currentScriptThread = &thread;
            m_Tls.scriptThreadActive = true;
        }

        ~RuntimeScriptScope()
        {
            m_Tls.scriptThreadActive = m_OriginalActive;
            m_Tls.currentScriptThread = m_OriginalThread;
        }

        RuntimeScriptScope(const RuntimeScriptScope&) = delete;
        RuntimeScriptScope& operator=(const RuntimeScriptScope&) = delete;

    private:
        LiveScriptTlsContext& m_Tls;
        LiveScriptThread* m_OriginalThread{};
        bool m_OriginalActive{};
    };

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
        const auto threadsMatch = FindUnique(*image, "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97");
        const auto globalsMatch = FindUnique(*image, "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8");
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

        g_RuntimeScriptThreads = nullptr;
        if (threadsMatch)
        {
            const auto threads = Rip(*image, *threadsMatch + 3);
            if (threads && image->Contains(*threads, sizeof(LiveScriptThreadArray)))
                g_RuntimeScriptThreads = reinterpret_cast<LiveScriptThreadArray*>(*threads);
        }

        g_RuntimeScriptGlobals = nullptr;
        if (globalsMatch)
        {
            const auto globals = Rip(*image, *globalsMatch + 10);
            if (globals && image->Contains(*globals, GlobalPageCount * sizeof(std::int64_t*)))
                g_RuntimeScriptGlobals = reinterpret_cast<std::int64_t**>(*globals);
        }

        if (g_RuntimeScriptGlobals)
            Game::Enhanced::ScriptGlobal::BindResolver(&ResolveRuntimeScriptGlobal);
        else
            Game::Enhanced::ScriptGlobal::ResetResolver();

        m_Window = FindGameWindow();
        Log::Write(m_Window ? "GTA window and core patterns resolved" : "GTA window was not found");
        if (!m_InitNativeTablesAddress)
            Log::Write("native table pattern unavailable; native backend will remain disabled");
        if (!g_RuntimeScriptThreads)
            Log::Write("script thread table unavailable; native feature ticks will remain disabled");
        if (!g_RuntimeScriptGlobals)
            Log::Write("script global table unavailable; ScriptGlobal access will remain disabled");
        return m_CommandQueueAddress && m_SwapChainAddress && m_RunScriptThreadsAddress && m_Window;
    }

    bool GtaRuntime::InitializeNativeBackend()
    {
        // Called only while RuntimeScriptScope has installed a real GTA script
        // thread in TLS. Enhanced hashes are translated before InitNativeTables,
        // matching the Gen9 flow used by YimMenuV2's Enhanced invoker.
        m_NativeBootstrapFinalized = true;
        Log::Write("initializing native backend in GTA script TLS context");

        using InitNativeTablesFn = void (*)(Game::Enhanced::LiveScriptProgram*);
        if (!m_InitNativeTablesAddress || !IsExecutableAddress(m_InitNativeTablesAddress))
        {
            Log::Write("native backend initialization failed: InitNativeTables address is not executable");
            return false;
        }

        auto entries = Game::Natives::Generated::EnhancedNativeHashes;
        g_RuntimeNativeHandlers.fill(nullptr);
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            if (entries[index] == 0)
            {
                Log::Write(
                    "native backend initialization failed: Enhanced hash missing at native index " +
                    std::to_string(index));
                return false;
            }
        }

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
            std::to_string(Game::Natives::NativeCount) + " Enhanced handlers resolved");
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

        m_NativeBootstrapFinalized = false;
        m_NativeBackendRetry = 0;
        m_ScriptContextRetry = 0;
        Log::Write("native backend resolution deferred to GTA script TLS context");

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

            if (!Game::Enhanced::EnhancedGame::ScriptFunctionsReady() && ++runtime->m_ScriptHostRetry >= 300)
            {
                runtime->m_ScriptHostRetry = 0;
                static_cast<void>(Game::Enhanced::EnhancedGame::InitializeScriptHost());
            }

            auto* thread = FindPreferredScriptThread();
            auto* tls = ResolveRuntimeTls();
            if (thread && tls)
            {
                runtime->m_ScriptContextRetry = 0;
                RuntimeScriptScope scope{*tls, *thread};

                if (!Game::Enhanced::EnhancedGame::Ready() && !runtime->m_NativeBootstrapFinalized &&
                    ++runtime->m_NativeBackendRetry >= 300)
                {
                    runtime->m_NativeBackendRetry = 0;
                    static_cast<void>(runtime->InitializeNativeBackend());
                }

                if (Game::Enhanced::EnhancedGame::Ready())
                    Game::Enhanced::EnhancedGame::Tick();
            }
            else if (++runtime->m_ScriptContextRetry >= 300)
            {
                runtime->m_ScriptContextRetry = 0;
                Log::Write("waiting for a valid GTA script TLS context (freemode/main_persistent/startup)");
            }
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
            g_RuntimeScriptThreads = nullptr;
            g_RuntimeScriptGlobals = nullptr;
        }
        m_NativeBootstrapFinalized = false;
        m_NativeBackendRetry = 0;
        m_ScriptHostRetry = 0;
        m_ScriptContextRetry = 0;
        s_Instance = nullptr;
    }
}
