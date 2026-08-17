#include "LoggingEngine.hpp"

#include "backend/tasking/TaskAffinity.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System::Logging
{
    namespace
    {
        const char* LevelName(LogLevel level) noexcept
        {
            switch (level)
            {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Warn: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            case LogLevel::Info:
            default: return "INFO";
            }
        }

        const char* AffinityName(Tasking::TaskAffinity affinity) noexcept
        {
            switch (affinity)
            {
            case Tasking::TaskAffinity::Game: return "game";
            case Tasking::TaskAffinity::Background: return "background";
            case Tasking::TaskAffinity::Render: return "render";
            case Tasking::TaskAffinity::Unspecified:
            default: return "unspecified";
            }
        }

        std::string Timestamp(const std::chrono::system_clock::time_point& time)
        {
            const auto value = std::chrono::system_clock::to_time_t(time);
            std::tm utc{};
#if defined(_WIN32)
            gmtime_s(&utc, &value);
#else
            gmtime_r(&value, &utc);
#endif
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                time.time_since_epoch()).count() % 1000;
            std::ostringstream stream;
            stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
                   << '.' << std::setw(3) << std::setfill('0') << milliseconds << 'Z';
            return stream.str();
        }

        std::string JsonEscape(std::string_view value)
        {
            std::string escaped;
            escaped.reserve(value.size() + 8);
            for (const char character : value)
            {
                switch (character)
                {
                case '\\': escaped += "\\\\"; break;
                case '"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += character; break;
                }
            }
            return escaped;
        }

        std::filesystem::path ArchivePath(const std::filesystem::path& path, std::size_t index)
        {
            return path.parent_path() /
                (path.stem().string() + "." + std::to_string(index) + path.extension().string());
        }
    }

    bool LoggingEngine::Initialize(
        std::filesystem::path textPath,
        std::filesystem::path structuredPath,
        std::size_t maxPending,
        std::uintmax_t maxFileBytes,
        std::size_t retainedFiles) noexcept
    {
        if (m_Ready.load(std::memory_order_acquire))
            return true;
        try
        {
            m_TextPath = std::move(textPath);
            m_StructuredPath = std::move(structuredPath);
            m_MaxPending = std::max<std::size_t>(maxPending, 64);
            m_MaxFileBytes = std::max<std::uintmax_t>(maxFileBytes, 64 * 1024);
            m_RetainedFiles = std::clamp<std::size_t>(retainedFiles, 1, 20);
            std::error_code error;
            std::filesystem::create_directories(m_TextPath.parent_path(), error);
            if (error || !OpenSinks())
                return false;
            {
                std::scoped_lock lock(m_QueueMutex);
                m_Stopping = false;
                m_Writing = false;
                m_Queue.clear();
            }
            {
                std::scoped_lock lock(m_RecentMutex);
                m_Recent.clear();
            }
            m_Accepted.store(0, std::memory_order_release);
            m_Written.store(0, std::memory_order_release);
            m_Dropped.store(0, std::memory_order_release);
            m_SinkFailures.store(0, std::memory_order_release);
            m_Rotations.store(0, std::memory_order_release);
            m_LastDrainMicros.store(0, std::memory_order_release);
            m_PeakPending.store(0, std::memory_order_release);
            m_Ready.store(true, std::memory_order_release);
            try
            {
                m_Worker = std::thread([this]() { Worker(); });
            }
            catch (...)
            {
                m_Ready.store(false, std::memory_order_release);
                {
                    std::scoped_lock lock(m_QueueMutex);
                    m_Stopping = true;
                }
                if (m_TextFile)
                    m_TextFile.close();
                if (m_StructuredFile)
                    m_StructuredFile.close();
                return false;
            }
            return true;
        }
        catch (...)
        {
            m_Ready.store(false, std::memory_order_release);
            return false;
        }
    }

    void LoggingEngine::Shutdown() noexcept
    {
        if (!m_Ready.exchange(false, std::memory_order_acq_rel))
            return;
        {
            std::scoped_lock lock(m_QueueMutex);
            m_Stopping = true;
        }
        m_Condition.notify_all();
        if (m_Worker.joinable())
            m_Worker.join();
        if (m_TextFile)
        {
            m_TextFile.flush();
            m_TextFile.close();
        }
        if (m_StructuredFile)
        {
            m_StructuredFile.flush();
            m_StructuredFile.close();
        }
    }

    bool LoggingEngine::Enqueue(LogRecord record) noexcept
    {
        try
        {
            if (!m_Ready.load(std::memory_order_acquire))
                return false;
            std::size_t pending{};
            {
                std::scoped_lock lock(m_QueueMutex);
                if (m_Stopping || m_Queue.size() >= m_MaxPending)
                {
                    m_Dropped.fetch_add(1, std::memory_order_relaxed);
                    return false;
                }
                m_Queue.push_back(std::move(record));
                pending = m_Queue.size();
            }
            m_Accepted.fetch_add(1, std::memory_order_relaxed);
            RecordPeak(pending);
            m_Condition.notify_one();
            return true;
        }
        catch (...)
        {
            m_Dropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
    }

    void LoggingEngine::Flush() noexcept
    {
        const auto affinity = Tasking::CurrentTaskAffinity();
        if (affinity == Tasking::TaskAffinity::Game || affinity == Tasking::TaskAffinity::Render)
            return;
        std::unique_lock lock(m_QueueMutex);
        m_FlushCondition.wait(lock, [this]() { return m_Queue.empty() && !m_Writing; });
        if (m_TextFile)
            m_TextFile.flush();
        if (m_StructuredFile)
            m_StructuredFile.flush();
    }

    bool LoggingEngine::Ready() const noexcept
    {
        return m_Ready.load(std::memory_order_acquire);
    }

    LoggerSnapshot LoggingEngine::Snapshot() const noexcept
    {
        std::size_t pending{};
        {
            std::scoped_lock lock(m_QueueMutex);
            pending = m_Queue.size();
        }
        return {
            .accepted = m_Accepted.load(std::memory_order_acquire),
            .written = m_Written.load(std::memory_order_acquire),
            .dropped = m_Dropped.load(std::memory_order_acquire),
            .sinkFailures = m_SinkFailures.load(std::memory_order_acquire),
            .rotations = m_Rotations.load(std::memory_order_acquire),
            .pending = pending,
            .peakPending = m_PeakPending.load(std::memory_order_acquire),
            .lastDrainMicros = m_LastDrainMicros.load(std::memory_order_acquire),
        };
    }

    std::vector<LogRecord> LoggingEngine::Recent(std::size_t maximum) const
    {
        std::scoped_lock lock(m_RecentMutex);
        maximum = std::min(maximum, m_Recent.size());
        return std::vector<LogRecord>(m_Recent.end() - static_cast<std::ptrdiff_t>(maximum), m_Recent.end());
    }

    void LoggingEngine::Worker() noexcept
    {
        Tasking::ScopedTaskAffinity affinity{Tasking::TaskAffinity::Background};
        for (;;)
        {
            std::vector<LogRecord> batch;
            batch.reserve(BatchSize);
            {
                std::unique_lock lock(m_QueueMutex);
                m_Condition.wait(lock, [this]() { return m_Stopping || !m_Queue.empty(); });
                if (m_Stopping && m_Queue.empty())
                    return;
                while (!m_Queue.empty() && batch.size() < BatchSize)
                {
                    batch.push_back(std::move(m_Queue.front()));
                    m_Queue.pop_front();
                }
                m_Writing = true;
            }

            const auto start = std::chrono::steady_clock::now();
            WriteBatch(batch);
            m_LastDrainMicros.store(static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count()), std::memory_order_release);

            {
                std::scoped_lock lock(m_QueueMutex);
                m_Writing = false;
                if (m_Queue.empty())
                    m_FlushCondition.notify_all();
            }
        }
    }

    void LoggingEngine::WriteBatch(std::vector<LogRecord>& batch) noexcept
    {
        try
        {
            std::ostringstream text;
            std::ostringstream structured;
            for (const auto& record : batch)
            {
                const auto timestamp = Timestamp(record.timestamp);
                text << '[' << timestamp << "] [" << LevelName(record.level) << "] ["
                     << record.channel << "] [" << AffinityName(record.affinity) << ':' << record.threadId
                     << "] " << record.message;
                for (const auto& field : record.fields)
                    text << ' ' << field.key << '=' << field.value;
                if (record.sourceFile)
                    text << " (" << record.sourceFile << ':' << record.sourceLine << ')';
                text << '\n';

                structured << "{\"seq\":" << record.sequence
                           << ",\"timestamp\":\"" << JsonEscape(timestamp)
                           << "\",\"level\":\"" << LevelName(record.level)
                           << "\",\"channel\":\"" << JsonEscape(record.channel)
                           << "\",\"message\":\"" << JsonEscape(record.message)
                           << "\",\"thread\":" << record.threadId
                           << ",\"affinity\":\"" << AffinityName(record.affinity)
                           << "\",\"monotonic_us\":" << record.monotonicMicros
                           << ",\"source_file\":\"" << JsonEscape(record.sourceFile ? record.sourceFile : "")
                           << "\",\"source_function\":\"" << JsonEscape(record.sourceFunction ? record.sourceFunction : "")
                           << "\",\"line\":" << record.sourceLine << ",\"fields\":{";
                for (std::size_t index = 0; index < record.fields.size(); ++index)
                {
                    if (index != 0)
                        structured << ',';
                    structured << '"' << JsonEscape(record.fields[index].key) << "\":\""
                               << JsonEscape(record.fields[index].value) << '"';
                }
                structured << "}}\n";
            }

            const auto textValue = text.str();
            const auto structuredValue = structured.str();
            RotateIfNeeded(std::max(textValue.size(), structuredValue.size()));

            if (m_TextFile)
            {
                m_TextFile << textValue;
                m_TextFile.flush();
                if (m_TextFile)
                    m_TextBytes += textValue.size();
                else
                    m_SinkFailures.fetch_add(batch.size(), std::memory_order_relaxed);
            }
            else
                m_SinkFailures.fetch_add(batch.size(), std::memory_order_relaxed);

            if (m_StructuredFile)
            {
                m_StructuredFile << structuredValue;
                m_StructuredFile.flush();
                if (m_StructuredFile)
                    m_StructuredBytes += structuredValue.size();
                else
                    m_SinkFailures.fetch_add(batch.size(), std::memory_order_relaxed);
            }
            else
                m_SinkFailures.fetch_add(batch.size(), std::memory_order_relaxed);

            std::cout << textValue;
            std::cout.flush();
#if defined(_WIN32)
            OutputDebugStringA(textValue.c_str());
#endif
            {
                std::scoped_lock lock(m_RecentMutex);
                for (auto& record : batch)
                {
                    if (m_Recent.size() >= RecentCapacity)
                        m_Recent.pop_front();
                    m_Recent.push_back(record);
                }
            }
            m_Written.fetch_add(batch.size(), std::memory_order_relaxed);
        }
        catch (...)
        {
            m_SinkFailures.fetch_add(batch.size(), std::memory_order_relaxed);
        }
    }

    void LoggingEngine::RotateIfNeeded(std::uintmax_t incomingBytes) noexcept
    {
        if (m_TextBytes + incomingBytes <= m_MaxFileBytes &&
            m_StructuredBytes + incomingBytes <= m_MaxFileBytes)
            return;
        if (m_TextFile)
            m_TextFile.close();
        if (m_StructuredFile)
            m_StructuredFile.close();
        RotateOne(m_TextPath);
        RotateOne(m_StructuredPath);
        if (OpenSinks(true))
            m_Rotations.fetch_add(1, std::memory_order_relaxed);
        else
            m_SinkFailures.fetch_add(1, std::memory_order_relaxed);
    }

    void LoggingEngine::RotateOne(const std::filesystem::path& path) noexcept
    {
        std::error_code error;
        std::filesystem::remove(ArchivePath(path, m_RetainedFiles), error);
        for (std::size_t index = m_RetainedFiles; index > 1; --index)
        {
            error.clear();
            const auto from = ArchivePath(path, index - 1);
            if (!std::filesystem::exists(from, error) || error)
                continue;
            error.clear();
            std::filesystem::rename(from, ArchivePath(path, index), error);
        }
        error.clear();
        if (std::filesystem::exists(path, error) && !error)
        {
            error.clear();
            std::filesystem::rename(path, ArchivePath(path, 1), error);
        }
    }

    bool LoggingEngine::OpenSinks(bool truncate) noexcept
    {
        const auto mode = std::ios::out | (truncate ? std::ios::trunc : std::ios::app);
        m_TextFile.open(m_TextPath, mode);
        m_StructuredFile.open(m_StructuredPath, mode);
        if (!m_TextFile || !m_StructuredFile)
            return false;
        std::error_code error;
        m_TextBytes = std::filesystem::file_size(m_TextPath, error);
        if (error)
            m_TextBytes = 0;
        error.clear();
        m_StructuredBytes = std::filesystem::file_size(m_StructuredPath, error);
        if (error)
            m_StructuredBytes = 0;
        return true;
    }

    void LoggingEngine::RecordPeak(std::size_t pending) noexcept
    {
        auto peak = m_PeakPending.load(std::memory_order_relaxed);
        while (pending > peak &&
               !m_PeakPending.compare_exchange_weak(peak, pending, std::memory_order_relaxed))
        {
        }
    }
}
