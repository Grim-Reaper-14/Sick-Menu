#include "GtaRuntime.hpp"
#include "RuntimeLog.hpp"

#include <chrono>
#include <thread>
#include <windows.h>

namespace
{
    DWORD WINAPI Bootstrap(void* parameter)
    {
        const auto module = static_cast<HMODULE>(parameter);
        Sick::Runtime::Log::Initialize(module);
        Sick::Runtime::Log::Write("bootstrap thread started");
        Sick::Runtime::GtaRuntime runtime;
        if (runtime.Initialize())
        {
            while (!runtime.StopRequested() && !(GetAsyncKeyState(VK_END) & 1))
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            runtime.RequestStop();
            runtime.Shutdown();
        }
        else
        {
            Sick::Runtime::Log::Write("runtime initialization failed; DLL will unload");
        }
        Sick::Runtime::Log::Shutdown();
        FreeLibraryAndExitThread(module, 0);
        return 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void*)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        if (const auto thread = CreateThread(nullptr, 0, &Bootstrap, instance, 0, nullptr))
            CloseHandle(thread);
    }
    return TRUE;
}
