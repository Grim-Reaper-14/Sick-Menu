#include "sick/core/application.hpp"

#include <Windows.h>

namespace
{
    DWORD WINAPI bootstrap(const LPVOID parameter)
    {
        const auto module = static_cast<HMODULE>(parameter);
        sick::core::Application application(module);
        const DWORD result = application.run();
        FreeLibraryAndExitThread(module, result);
    }
}

BOOL APIENTRY DllMain(const HMODULE module, const DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);

        const HANDLE thread = CreateThread(nullptr, 0, bootstrap, module, 0, nullptr);
        if (thread == nullptr)
            return FALSE;

        CloseHandle(thread);
    }

    return TRUE;
}
