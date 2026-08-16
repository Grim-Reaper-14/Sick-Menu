#include "RuntimeLog.hpp"

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace
{
    std::mutex g_LogMutex;
    std::ofstream g_LogFile;
    PVOID g_ExceptionHandler{};
    std::uintptr_t g_ModuleBase{};
    std::size_t g_ModuleSize{};

    LONG CALLBACK LogModuleException(EXCEPTION_POINTERS* exception) noexcept
    {
        if (!exception || !exception->ExceptionRecord)
            return EXCEPTION_CONTINUE_SEARCH;
        const auto address = reinterpret_cast<std::uintptr_t>(
            exception->ExceptionRecord->ExceptionAddress);
        if (address < g_ModuleBase || address >= g_ModuleBase + g_ModuleSize)
            return EXCEPTION_CONTINUE_SEARCH;

        std::ostringstream message;
        message << "CRASH inside SickMenu.dll: code=0x" << std::hex
                << exception->ExceptionRecord->ExceptionCode
                << " address=0x" << address;
        Sick::Runtime::Log::Write(message.str());
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

namespace Sick::Runtime::Log
{
    bool Initialize(HMODULE module) noexcept
    {
        if (!module)
            return false;

        g_ModuleBase = reinterpret_cast<std::uintptr_t>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_ModuleBase);
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
            const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(g_ModuleBase + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE)
                g_ModuleSize = nt->OptionalHeader.SizeOfImage;
        }

        if (AllocConsole())
        {
            SetConsoleTitleW(L"Sick Menu - GTA V Enhanced");
            FILE* stream{};
            static_cast<void>(freopen_s(&stream, "CONOUT$", "w", stdout));
            static_cast<void>(freopen_s(&stream, "CONOUT$", "w", stderr));
        }

        wchar_t modulePath[MAX_PATH]{};
        if (GetModuleFileNameW(module, modulePath, MAX_PATH))
        {
            std::wstring path{modulePath};
            const auto separator = path.find_last_of(L"\\/");
            path.resize(separator == std::wstring::npos ? 0 : separator + 1);
            path += L"SickMenu.log";
            g_LogFile.open(path, std::ios::out | std::ios::trunc);
        }

        g_ExceptionHandler = AddVectoredExceptionHandler(1, &LogModuleException);
        Write("logger initialized");
        return true;
    }

    void Write(std::string_view message) noexcept
    {
        try
        {
            std::scoped_lock lock(g_LogMutex);
            std::cout << "[SickMenu] " << message << std::endl;
            if (g_LogFile)
            {
                g_LogFile << "[SickMenu] " << message << std::endl;
                g_LogFile.flush();
            }
        }
        catch (...)
        {
        }
    }

    void Shutdown() noexcept
    {
        Write("logger shutting down");
        if (g_ExceptionHandler)
        {
            RemoveVectoredExceptionHandler(g_ExceptionHandler);
            g_ExceptionHandler = nullptr;
        }
        {
            std::scoped_lock lock(g_LogMutex);
            if (g_LogFile)
                g_LogFile.close();
        }
        FreeConsole();
    }
}
