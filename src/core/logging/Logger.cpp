#include "Logger.hpp"
#include "AnomalyDetector.hpp"
#include "sinks/ConsoleSink.hpp"
#include "sinks/DebuggerSink.hpp"
#include "sinks/FileSink.hpp"

#include <algorithm>
#include <functional>
#include <iterator>

namespace Sick::Core::Logging
{
    Logger& Logger::Get() noexcept
    {
        static Logger logger;
        return logger;
    }

    Logger::~Logger()
    {
        Stop();
    }

    bool Logger::Start(LoggerConfig config)
    {
        bool expected = false;
        if (!m_Running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return true;

        {
            std::scoped_lock lock(m_QueueMutex, m_RecentMutex);
            m_QueueCapacity = config.queueCapacity;
            m_RecentCapacity = config.recentCapacity;
            while (m_Recent.size() > m_RecentCapacity)
                m_Recent.pop_front();
        }

        m_Minimum.store(config.minimumLevel, std::memory_order_release);

        try
        {
            m_Worker = std::thread(&Logger::WorkerLoop, this);
        }
        catch (...)
        {
            m_Running.store(false, std::memory_order_release);
            return false;
        }

        Submit(Level::Info, "Logger", "Reaper observability pipeline started", EventId::LoggerStarted);
        return true;
    }

    void Logger::Stop() noexcept
    {
        const bool wasRunning = m_Running.exchange(false, std::memory_order_acq_rel);
        if (wasRunning)
            m_QueueCv.notify_all();

        if (m_Worker.joinable())
        {
            try
            {
                m_Worker.join();
            }
            catch (...)
            {
            }
        }

        Flush();
    }

    bool Logger::Running() const noexcept
    {
        return m_Running.load(std::memory_order_acquire);
    }

    void Logger::AddSink(SinkPtr sink)
    {
        if (!sink)
            return;

        std::scoped_lock lock(m_SinkMutex);
        m_Sinks.push_back(std::move(sink));
    }

    void Logger::ClearSinks() noexcept
    {
        try
        {
            std::scoped_lock lock(m_SinkMutex);
            m_Sinks.clear();
        }
        catch (...)
        {
        }
    }

    void Logger::Submit(
        Level level,
        std::string_view category,
        std::string message,
        EventId event,
        std::vector<Field> fields,
        std::source_location source) noexcept
    {
        if (!Enabled(level, m_Minimum.load(std::memory_order_acquire)))
            return;

        try
        {
            const auto context = CaptureContext();

            LogRecord record;
            record.sequence = m_Sequence.fetch_add(1, std::memory_order_relaxed) + 1;
            record.wallTime = std::chrono::system_clock::now();
            record.monotonicTime = std::chrono::steady_clock::now();
            record.level = level;
            record.event = event;
            record.category = std::string{category};
            record.message = std::move(message);
            record.threadId = static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
            record.threadName = context.threadName;
            record.correlationId = context.correlationId;
            record.spanId = context.spanId;
            record.source = source;

            record.fields.reserve(context.fields.size() + fields.size() + 3);
            record.fields.insert(record.fields.end(), context.fields.begin(), context.fields.end());
            record.fields.insert(
                record.fields.end(),
                std::make_move_iterator(fields.begin()),
                std::make_move_iterator(fields.end()));

            try
            {
                const auto anomaly = AnomalyDetector::Get().Observe(
                    record.level,
                    record.event,
                    record.category,
                    record.message);

                if (anomaly.burst)
                {
                    record.fields.push_back(MakeField("anomaly", "burst"));
                    record.fields.push_back(MakeField("burst_count", anomaly.count));
                    if (anomaly.newlyDetected)
                        record.fields.push_back(MakeField("anomaly_new", true));
                }
            }
            catch (...)
            {
            }

            StoreRecent(record);

            if (level == Level::Critical || !Running())
            {
                m_EmergencyDispatches.fetch_add(1, std::memory_order_relaxed);
                Dispatch(record);
                return;
            }

            bool emergency = false;
            bool queued = false;
            {
                std::unique_lock lock(m_QueueMutex);
                if (!MakeQueueRoomLocked(level))
                {
                    m_Dropped.fetch_add(1, std::memory_order_relaxed);
                    emergency = level >= Level::Error;
                }
                else
                {
                    m_Queue.push_back(std::move(record));
                    m_Enqueued.fetch_add(1, std::memory_order_relaxed);
                    queued = true;
                }
            }

            if (emergency)
            {
                m_EmergencyDispatches.fetch_add(1, std::memory_order_relaxed);
                Dispatch(record);
                return;
            }

            if (queued)
                m_QueueCv.notify_one();
        }
        catch (...)
        {
            m_Dropped.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool Logger::MakeQueueRoomLocked(Level incoming) noexcept
    {
        if (m_QueueCapacity == 0)
            return false;

        if (m_Queue.size() < m_QueueCapacity)
            return true;

        const auto lowPriority = std::find_if(
            m_Queue.begin(),
            m_Queue.end(),
            [](const LogRecord& record)
            {
                return record.level <= Level::Info;
            });

        if (lowPriority != m_Queue.end())
        {
            m_Queue.erase(lowPriority);
            m_Dropped.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        if (incoming >= Level::Error && !m_Queue.empty())
        {
            m_Queue.pop_front();
            m_Dropped.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        return false;
    }

    void Logger::WorkerLoop() noexcept
    {
        SetThreadName("ReaperLogWorker");

        for (;;)
        {
            LogRecord record;

            {
                std::unique_lock lock(m_QueueMutex);
                m_QueueCv.wait(lock, [this]
                {
                    return !m_Running.load(std::memory_order_acquire) || !m_Queue.empty();
                });

                if (m_Queue.empty())
                {
                    if (!m_Running.load(std::memory_order_acquire))
                        break;
                    continue;
                }

                record = std::move(m_Queue.front());
                m_Queue.pop_front();
                ++m_InFlight;
            }

            Dispatch(record);

            {
                std::scoped_lock lock(m_QueueMutex);
                --m_InFlight;
                if (m_Queue.empty() && m_InFlight == 0)
                    m_DrainedCv.notify_all();
            }
        }

        std::scoped_lock lock(m_QueueMutex);
        if (m_Queue.empty() && m_InFlight == 0)
            m_DrainedCv.notify_all();
    }

    void Logger::Dispatch(const LogRecord& record) noexcept
    {
        try
        {
            std::vector<SinkPtr> sinks;
            {
                std::scoped_lock lock(m_SinkMutex);
                sinks = m_Sinks;
            }

            for (const auto& sink : sinks)
            {
                if (sink)
                    sink->Write(record);
            }

            m_Dispatched.fetch_add(1, std::memory_order_relaxed);
        }
        catch (...)
        {
        }
    }

    void Logger::StoreRecent(const LogRecord& record) noexcept
    {
        try
        {
            std::scoped_lock lock(m_RecentMutex);
            if (m_RecentCapacity == 0)
                return;

            while (m_Recent.size() >= m_RecentCapacity)
                m_Recent.pop_front();

            m_Recent.push_back(record);
        }
        catch (...)
        {
        }
    }

    void Logger::Flush() noexcept
    {
        try
        {
            if (Running() && std::this_thread::get_id() != m_Worker.get_id())
            {
                std::unique_lock lock(m_QueueMutex);
                m_DrainedCv.wait(lock, [this]
                {
                    return m_Queue.empty() && m_InFlight == 0;
                });
            }

            std::vector<SinkPtr> sinks;
            {
                std::scoped_lock lock(m_SinkMutex);
                sinks = m_Sinks;
            }

            for (const auto& sink : sinks)
            {
                if (sink)
                    sink->Flush();
            }
        }
        catch (...)
        {
        }
    }

    std::vector<LogRecord> Logger::Recent(std::size_t maxRecords) const
    {
        std::scoped_lock lock(m_RecentMutex);

        const auto count = maxRecords == 0 || maxRecords > m_Recent.size()
            ? m_Recent.size()
            : maxRecords;
        const auto start = m_Recent.size() - count;

        return std::vector<LogRecord>{
            m_Recent.begin() + static_cast<std::ptrdiff_t>(start),
            m_Recent.end()
        };
    }

    LoggerStats Logger::Stats() const noexcept
    {
        LoggerStats stats;
        stats.enqueued = m_Enqueued.load(std::memory_order_relaxed);
        stats.dispatched = m_Dispatched.load(std::memory_order_relaxed);
        stats.dropped = m_Dropped.load(std::memory_order_relaxed);
        stats.emergencyDispatches = m_EmergencyDispatches.load(std::memory_order_relaxed);
        stats.anomalyDetections = AnomalyDetector::Get().DetectionCount();

        try
        {
            {
                std::scoped_lock lock(m_QueueMutex);
                stats.queueDepth = m_Queue.size();
            }
            {
                std::scoped_lock lock(m_RecentMutex);
                stats.recentRecords = m_Recent.size();
            }
        }
        catch (...)
        {
        }

        return stats;
    }

    void Logger::SetMinimumLevel(Level level) noexcept
    {
        m_Minimum.store(level, std::memory_order_release);
    }

    Level Logger::MinimumLevel() const noexcept
    {
        return m_Minimum.load(std::memory_order_acquire);
    }

    bool Initialize(LoggerConfig config)
    {
        auto& logger = Logger::Get();
        logger.Stop();
        logger.ClearSinks();

        if (config.console)
            logger.AddSink(std::make_shared<ConsoleSink>(config.minimumLevel));

        if (config.debugger)
            logger.AddSink(std::make_shared<DebuggerSink>(config.minimumLevel));

        if (!config.filePath.empty())
        {
            logger.AddSink(std::make_shared<FileSink>(
                config.filePath,
                config.rotateBytes,
                config.rotateFiles,
                config.jsonFile,
                config.minimumLevel));
        }

        return logger.Start(std::move(config));
    }

    void Shutdown() noexcept
    {
        Logger::Get().Stop();
    }

    void Flush() noexcept
    {
        Logger::Get().Flush();
    }
}
