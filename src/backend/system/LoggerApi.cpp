#include "LoggerApi.hpp"

#include "FileSystem.hpp"
#include "backend/tasking/TaskAffinity.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <utility>

namespace Sick::Backend::System
{
    LoggerApi& LoggerApi::Get() noexcept
    {
        static LoggerApi api;
        return api;
    }

    bool LoggerApi::Initialize(const FileSystem& files) noexcept
    {
        return InitializePaths(files.LogFile(), files.StructuredLogFile(), files.EmergencyLogFile());
    }

    bool LoggerApi::InitializePaths(
        const std::filesystem::path& textPath,
        const std::filesystem::path& structuredPath,
        const std::filesystem::path& emergencyPath) noexcept
    {
        if (Ready())
            return true;
        m_Start = std::chrono::steady_clock::now();
        m_Sequence.store(1, std::memory_order_release);
        static_cast<void>(m_Emergency.Initialize(emergencyPath));
        if (!m_Engine.Initialize(textPath, structuredPath))
        {
            m_Emergency.Write("logging engine initialization failed");
            return false;
        }
        return true;
    }

    void LoggerApi::Shutdown() noexcept
    {
        m_Engine.Shutdown();
        m_Emergency.Shutdown();
    }

    void LoggerApi::Flush() noexcept
    {
        m_Engine.Flush();
    }

    void LoggerApi::Write(
        LogLevel level,
        std::string_view channel,
        std::string_view message,
        Fields fields,
        const std::source_location& source) noexcept
    {
        try
        {
            Logging::LogRecord record{};
            record.sequence = m_Sequence.fetch_add(1, std::memory_order_relaxed);
            record.timestamp = std::chrono::system_clock::now();
            record.monotonicMicros = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - m_Start).count());
            record.threadId = static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
            record.affinity = Tasking::CurrentTaskAffinity();
            record.level = level;
            record.channel.assign(channel);
            record.message.assign(message);
            record.fields = std::move(fields);
            record.sourceFile = source.file_name();
            record.sourceFunction = source.function_name();
            record.sourceLine = source.line();
            if (!m_Engine.Enqueue(std::move(record)) && level >= LogLevel::Error)
                m_Emergency.Write(message);
        }
        catch (...)
        {
            if (level >= LogLevel::Error)
                m_Emergency.Write(message);
        }
    }

    void LoggerApi::Trace(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Trace, channel, message, std::move(fields), source); }
    void LoggerApi::Debug(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Debug, channel, message, std::move(fields), source); }
    void LoggerApi::Info(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Info, channel, message, std::move(fields), source); }
    void LoggerApi::Warn(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Warn, channel, message, std::move(fields), source); }
    void LoggerApi::Error(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Error, channel, message, std::move(fields), source); }
    void LoggerApi::Critical(std::string_view channel, std::string_view message, Fields fields, const std::source_location& source) noexcept
    { Write(LogLevel::Critical, channel, message, std::move(fields), source); }

    void LoggerApi::Emergency(std::string_view message) noexcept
    {
        m_Emergency.Write(message);
        if (!m_Emergency.Ready())
        {
            try
            {
                std::cerr << "[SickMenu Emergency] " << message << '\n';
                std::cerr.flush();
            }
            catch (...)
            {
            }
        }
    }

    bool LoggerApi::Ready() const noexcept
    {
        return m_Engine.Ready();
    }

    LoggerSnapshot LoggerApi::Snapshot() const noexcept
    {
        return m_Engine.Snapshot();
    }

    std::vector<Logging::LogRecord> LoggerApi::Recent(std::size_t maximum) const
    {
        return m_Engine.Recent(maximum);
    }
}
