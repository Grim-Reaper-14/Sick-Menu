#include "HealthMonitor.hpp"
#include "core/logging/Logger.hpp"

namespace Sick::Core::Health
{
    Monitor& Monitor::Get() noexcept
    {
        static Monitor monitor;
        return monitor;
    }

    Monitor::~Monitor()
    {
        Stop();
    }

    void Monitor::Start(std::chrono::milliseconds interval)
    {
        bool expected = false;
        if (!m_Running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return;

        m_Interval = interval.count() > 0 ? interval : std::chrono::milliseconds{500};

        try
        {
            m_Worker = std::thread(&Monitor::WorkerLoop, this);
        }
        catch (...)
        {
            m_Running.store(false, std::memory_order_release);
            throw;
        }
    }

    void Monitor::Stop() noexcept
    {
        const bool wasRunning = m_Running.exchange(false, std::memory_order_acq_rel);
        if (wasRunning)
            m_Cv.notify_all();

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
    }

    void Monitor::Register(std::string_view subsystem, std::chrono::milliseconds timeout)
    {
        std::scoped_lock lock(m_Mutex);
        m_Entries[std::string{subsystem}] = {
            std::chrono::steady_clock::now(),
            timeout.count() > 0 ? timeout : std::chrono::milliseconds{5000},
            false
        };
    }

    void Monitor::Remove(std::string_view subsystem) noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            m_Entries.erase(std::string{subsystem});
        }
        catch (...)
        {
        }
    }

    void Monitor::Heartbeat(std::string_view subsystem) noexcept
    {
        try
        {
            bool recovered = false;

            {
                std::scoped_lock lock(m_Mutex);
                auto [it, inserted] = m_Entries.try_emplace(
                    std::string{subsystem},
                    Entry{std::chrono::steady_clock::now(), std::chrono::milliseconds{5000}, false});

                if (!inserted)
                {
                    recovered = it->second.staleReported;
                    it->second.last = std::chrono::steady_clock::now();
                    it->second.staleReported = false;
                }
            }

            if (recovered)
            {
                Logging::Logger::Get().Submit(
                    Logging::Level::Info,
                    "Health",
                    Logging::Detail::Format("{} heartbeat recovered", subsystem),
                    Logging::EventId::HealthRecovered,
                    {Logging::MakeField("subsystem", subsystem)});
            }
        }
        catch (...)
        {
        }
    }

    std::vector<Issue> Monitor::CheckNow()
    {
        const auto now = std::chrono::steady_clock::now();
        std::vector<Issue> issues;
        std::vector<Issue> newlyStale;

        {
            std::scoped_lock lock(m_Mutex);
            for (auto& [name, entry] : m_Entries)
            {
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.last);
                if (elapsed <= entry.timeout)
                    continue;

                const Issue issue{name, elapsed - entry.timeout};
                issues.push_back(issue);

                if (!entry.staleReported)
                {
                    entry.staleReported = true;
                    newlyStale.push_back(issue);
                }
            }
        }

        for (const auto& issue : newlyStale)
        {
            Logging::Logger::Get().Submit(
                Logging::Level::Error,
                "Health",
                Logging::Detail::Format("{} heartbeat stalled", issue.subsystem),
                Logging::EventId::HealthStall,
                {
                    Logging::MakeField("subsystem", issue.subsystem),
                    Logging::MakeField("overdue_ms", issue.overdue.count())
                });
        }

        return issues;
    }

    void Monitor::WorkerLoop() noexcept
    {
        Logging::SetThreadName("ReaperHealth");

        while (m_Running.load(std::memory_order_acquire))
        {
            try
            {
                CheckNow();
            }
            catch (...)
            {
            }

            std::unique_lock lock(m_Mutex);
            m_Cv.wait_for(lock, m_Interval, [this]
            {
                return !m_Running.load(std::memory_order_acquire);
            });
        }
    }
}
