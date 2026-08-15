#pragma once

#include "LogContext.hpp"
#include "Sink.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace Sick::Core::Logging
{
    struct LoggerConfig
    {
        Level minimumLevel{Level::Debug};
        std::size_t queueCapacity{8192};
        std::size_t recentCapacity{512};
        bool console{true};
        bool debugger{true};
        std::filesystem::path filePath{"logs/reaper.log"};
        std::size_t rotateBytes{8 * 1024 * 1024};
        std::size_t rotateFiles{5};
        bool jsonFile{false};
    };

    struct LoggerStats
    {
        std::uint64_t enqueued{};
        std::uint64_t dispatched{};
        std::uint64_t dropped{};
        std::uint64_t emergencyDispatches{};
        std::uint64_t anomalyDetections{};
        std::size_t queueDepth{};
        std::size_t recentRecords{};
    };

    class Logger final
    {
    public:
        static Logger& Get() noexcept;
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        bool Start(LoggerConfig config = {});
        void Stop() noexcept;
        [[nodiscard]] bool Running() const noexcept;

        void AddSink(SinkPtr sink);
        void ClearSinks() noexcept;

        void Submit(
            Level level,
            std::string_view category,
            std::string message,
            EventId event = EventId::None,
            std::vector<Field> fields = {},
            std::source_location source = {}) noexcept;

        void Flush() noexcept;
        [[nodiscard]] std::vector<LogRecord> Recent(std::size_t maxRecords = 0) const;
        [[nodiscard]] LoggerStats Stats() const noexcept;

        void SetMinimumLevel(Level level) noexcept;
        [[nodiscard]] Level MinimumLevel() const noexcept;

    private:
        Logger() = default;

        void WorkerLoop() noexcept;
        void Dispatch(const LogRecord& record) noexcept;
        void StoreRecent(const LogRecord& record) noexcept;
        [[nodiscard]] bool MakeQueueRoomLocked(Level incoming) noexcept;

        mutable std::mutex m_QueueMutex;
        std::condition_variable m_QueueCv;
        std::condition_variable m_DrainedCv;
        std::deque<LogRecord> m_Queue;
        std::size_t m_QueueCapacity{8192};
        std::size_t m_InFlight{};

        mutable std::mutex m_SinkMutex;
        std::vector<SinkPtr> m_Sinks;

        mutable std::mutex m_RecentMutex;
        std::deque<LogRecord> m_Recent;
        std::size_t m_RecentCapacity{512};

        std::thread m_Worker;
        std::atomic_bool m_Running{};
        std::atomic<Level> m_Minimum{Level::Debug};
        std::atomic<std::uint64_t> m_Sequence{};
        std::atomic<std::uint64_t> m_Enqueued{};
        std::atomic<std::uint64_t> m_Dispatched{};
        std::atomic<std::uint64_t> m_Dropped{};
        std::atomic<std::uint64_t> m_EmergencyDispatches{};
    };

    bool Initialize(LoggerConfig config = {});
    void Shutdown() noexcept;
    void Flush() noexcept;

    template <typename... Args>
    void WriteAt(
        Level level,
        std::string_view category,
        std::source_location source,
        std::string_view format,
        Args&&... args) noexcept
    {
        try
        {
            Logger::Get().Submit(
                level,
                category,
                Detail::Format(format, std::forward<Args>(args)...),
                EventId::None,
                {},
                source);
        }
        catch (...)
        {
        }
    }

    template <typename... Args>
    void EventAt(
        Level level,
        EventId event,
        std::string_view category,
        std::vector<Field> fields,
        std::source_location source,
        std::string_view format,
        Args&&... args) noexcept
    {
        try
        {
            Logger::Get().Submit(
                level,
                category,
                Detail::Format(format, std::forward<Args>(args)...),
                event,
                std::move(fields),
                source);
        }
        catch (...)
        {
        }
    }

    template <typename... Args>
    void Trace(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Trace, category, {}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Debug(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Debug, category, {}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Info(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Info, category, {}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Warn(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Warn, category, {}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Error(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Error, category, {}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Critical(std::string_view category, std::string_view format, Args&&... args) noexcept
    {
        WriteAt(Level::Critical, category, {}, format, std::forward<Args>(args)...);
    }
}

#define REAPER_TRACE(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Trace, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_DEBUG(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Debug, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_INFO(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Info, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_WARN(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Warn, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_ERROR(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Error, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_CRITICAL(category, format, ...) \
    ::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Critical, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__)
#define REAPER_ENSURE(condition, category, format, ...) \
    ((condition) ? true : (::Sick::Core::Logging::WriteAt(::Sick::Core::Logging::Level::Error, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__), false))
