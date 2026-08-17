#include "EmergencyLogWriter.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System::Logging
{
    bool EmergencyLogWriter::Initialize(const std::filesystem::path& path) noexcept
    {
        Shutdown();
#if defined(_WIN32)
        const auto handle = CreateFileW(
            path.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return false;
        m_Handle = handle;
#else
        m_Handle = std::fopen(path.string().c_str(), "ab");
        if (!m_Handle)
            return false;
#endif
        m_Ready.store(true, std::memory_order_release);
        return true;
    }

    void EmergencyLogWriter::Shutdown() noexcept
    {
        if (!m_Ready.exchange(false, std::memory_order_acq_rel))
            return;
#if defined(_WIN32)
        if (m_Handle)
            CloseHandle(static_cast<HANDLE>(m_Handle));
#else
        if (m_Handle)
            std::fclose(static_cast<std::FILE*>(m_Handle));
#endif
        m_Handle = nullptr;
    }

    void EmergencyLogWriter::Write(std::string_view message) noexcept
    {
        static constexpr char Prefix[] = "[SickMenu Emergency] ";
        static constexpr char Newline[] = "\r\n";
        if (!m_Ready.load(std::memory_order_acquire) || !m_Handle)
            return;
#if defined(_WIN32)
        DWORD written{};
        static_cast<void>(WriteFile(m_Handle, Prefix, static_cast<DWORD>(sizeof(Prefix) - 1), &written, nullptr));
        static_cast<void>(WriteFile(m_Handle, message.data(), static_cast<DWORD>(message.size()), &written, nullptr));
        static_cast<void>(WriteFile(m_Handle, Newline, static_cast<DWORD>(sizeof(Newline) - 1), &written, nullptr));
        static_cast<void>(FlushFileBuffers(m_Handle));

        char debugger[1024]{};
        const auto count = std::min<std::size_t>(message.size(), sizeof(debugger) - 1);
        std::memcpy(debugger, message.data(), count);
        OutputDebugStringA("[SickMenu Emergency] ");
        OutputDebugStringA(debugger);
        OutputDebugStringA("\n");
#else
        auto* file = static_cast<std::FILE*>(m_Handle);
        std::fwrite(Prefix, 1, sizeof(Prefix) - 1, file);
        std::fwrite(message.data(), 1, message.size(), file);
        std::fwrite("\n", 1, 1, file);
        std::fflush(file);
#endif
    }

    bool EmergencyLogWriter::Ready() const noexcept
    {
        return m_Ready.load(std::memory_order_acquire);
    }
}
