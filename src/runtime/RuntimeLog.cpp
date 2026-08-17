#include "RuntimeLog.hpp"

#include "backend/BackendCore.hpp"
#include "backend/system/LoggerApi.hpp"

#include <cstdio>
#include <cstdint>
#include <filesystem>
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
        const auto address = reinterpret_cast<std::uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        if (address < g_ModuleBase || address >= g_ModuleBase + g_ModuleSize)
            return EXCEPTION_CONTINUE_SEARCH;

        char message[192]{};
        static_cast<void>(std::snprintf(
            message,
            sizeof(message),
            "CRASH inside SickMenu.dll: code=0x%08lX address=0x%llX",
            static_cast<unsigned long>(exception->ExceptionRecord->ExceptionCode),
            static_cast<unsigned long long>(address)));
        Sick::Backend::System::LoggerApi::Get().Emergency(message);
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

        // Runtime logging is file/debugger based. Do not allocate a process
        // console from an injected DLL: the console is unrelated to the GTA
        // render window and must never control overlay visibility or lifetime.

        std::filesystem::path moduleDirectory;
        wchar_t modulePath[MAX_PATH]{};
        if (GetModuleFileNameW(module, modulePath, MAX_PATH))
            moduleDirectory = std::filesystem::path(modulePath).parent_path();

        const bool backendReady = Backend::BackendCore::Get().Initialize(moduleDirectory);
        g_ExceptionHandler = AddVectoredExceptionHandler(1, &LogModuleException);
        Write(backendReady
            ? "backend logger initialized"
            : "backend core initialization failed; emergency logging active");
        return backendReady;
    }

    void Write(std::string_view message) noexcept
    {
        auto& logger = Backend::System::LoggerApi::Get();
        if (logger.Ready())
            logger.Info("runtime", message);
        else
            logger.Emergency(message);
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
    }
}
