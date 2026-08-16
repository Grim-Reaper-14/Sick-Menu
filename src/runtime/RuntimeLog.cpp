#include "RuntimeLog.hpp"

#include "backend/BackendCore.hpp"
#include "backend/system/Logger.hpp"

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
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
        Sick::Backend::System::Logger::Get().WriteImmediate(message.str());
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

        std::filesystem::path moduleDirectory;
        wchar_t modulePath[MAX_PATH]{};
        if (GetModuleFileNameW(module, modulePath, MAX_PATH))
            moduleDirectory = std::filesystem::path(modulePath).parent_path();

        const bool backendReady = Backend::BackendCore::Get().Initialize(moduleDirectory);
        g_ExceptionHandler = AddVectoredExceptionHandler(1, &LogModuleException);
        Write(backendReady
            ? "backend logger initialized"
            : "backend core initialization failed; logging to console fallback");
        return backendReady;
    }

    void Write(std::string_view message) noexcept
    {
        Backend::System::Logger::Get().Write(message);
    }

    void Shutdown() noexcept
    {
        Write("logger shutting down");
        Backend::BackendCore::Get().Shutdown();
        if (g_ExceptionHandler)
        {
            RemoveVectoredExceptionHandler(g_ExceptionHandler);
            g_ExceptionHandler = nullptr;
        }
        FreeConsole();
    }
}
