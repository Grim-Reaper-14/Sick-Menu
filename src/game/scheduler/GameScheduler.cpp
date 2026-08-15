#include "GameScheduler.hpp"
#include "core/diagnostics/HealthMonitor.hpp"
#include "core/diagnostics/Metrics.hpp"
#include "core/logging/Logger.hpp"

#include <algorithm>
#include <exception>

namespace Sick::Game
{
    GameScheduler& GameScheduler::Get() noexcept
    {
        static GameScheduler scheduler;
        return scheduler;
    }

    void GameScheduler::Queue(Job job)
    {
        if (!job)
            return;

        std::size_t pending{};
        {
            std::scoped_lock lock(m_Mutex);
            m_Jobs.push(std::move(job));
            pending = m_Jobs.size();
        }

        Core::Metrics::SetGauge("scheduler.pending", static_cast<double>(pending));
    }

    std::size_t GameScheduler::Tick(std::size_t maxJobs)
    {
        Core::Health::Heartbeat("GameScheduler");
        std::size_t executed = 0;

        while (executed < maxJobs)
        {
            Job job;

            {
                std::scoped_lock lock(m_Mutex);
                if (m_Jobs.empty())
                    break;

                job = std::move(m_Jobs.front());
                m_Jobs.pop();
            }

            const auto started = std::chrono::steady_clock::now();
            try
            {
                job();
            }
            catch (const std::exception& error)
            {
                Core::Metrics::Increment("scheduler.exceptions");
                try
                {
                    Core::Logging::Logger::Get().Submit(
                        Core::Logging::Level::Error,
                        "Scheduler",
                        Core::Logging::Detail::Format("Scheduled job threw: {}", error.what()),
                        Core::Logging::EventId::SchedulerJobException,
                        {Core::Logging::MakeField("exception", error.what())});
                }
                catch (...)
                {
                }
                throw;
            }
            catch (...)
            {
                Core::Metrics::Increment("scheduler.exceptions");
                try
                {
                    Core::Logging::Logger::Get().Submit(
                        Core::Logging::Level::Error,
                        "Scheduler",
                        "Scheduled job threw an unknown exception",
                        Core::Logging::EventId::SchedulerJobException);
                }
                catch (...)
                {
                }
                throw;
            }

            const auto elapsed = std::chrono::steady_clock::now() - started;
            const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            const auto elapsedMs = static_cast<double>(elapsedUs) / 1000.0;
            Core::Metrics::Observe("scheduler.job_ms", elapsedMs);

            const auto threshold = SlowJobThreshold();
            if (threshold.count() > 0 && elapsed > threshold)
            {
                Core::Metrics::Increment("scheduler.slow_jobs");
                try
                {
                    Core::Logging::Logger::Get().Submit(
                        Core::Logging::Level::Warn,
                        "Scheduler",
                        "Scheduled job exceeded the configured execution budget",
                        Core::Logging::EventId::SchedulerSlowJob,
                        {
                            Core::Logging::MakeField("elapsed_ms", elapsedMs),
                            Core::Logging::MakeField("budget_ms", threshold.count())
                        });
                }
                catch (...)
                {
                }
            }

            ++executed;
        }

        Core::Metrics::SetGauge("scheduler.pending", static_cast<double>(Pending()));
        Core::Metrics::Increment("scheduler.executed", static_cast<std::int64_t>(executed));
        return executed;
    }

    void GameScheduler::Clear() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        std::queue<Job> empty;
        m_Jobs.swap(empty);
        Core::Metrics::SetGauge("scheduler.pending", 0.0);
    }

    std::size_t GameScheduler::Pending() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Jobs.size();
    }

    void GameScheduler::SetSlowJobThreshold(std::chrono::milliseconds threshold) noexcept
    {
        m_SlowJobThresholdMs.store(std::max<std::int64_t>(0, threshold.count()), std::memory_order_release);
    }

    std::chrono::milliseconds GameScheduler::SlowJobThreshold() const noexcept
    {
        return std::chrono::milliseconds{m_SlowJobThresholdMs.load(std::memory_order_acquire)};
    }
}
