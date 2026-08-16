#include "GameCallHub.hpp"

#include "game/scheduler/GameScheduler.hpp"

#include <chrono>
#include <utility>

namespace Sick::Backend::Calls
{
    bool GameCallHub::QueueGame(Job job)
    {
        return Queue(CallRequirement::GameThread, std::move(job));
    }

    bool GameCallHub::QueueNative(Job job)
    {
        return Queue(CallRequirement::NativeBackend, std::move(job));
    }

    bool GameCallHub::QueueScript(Job job)
    {
        return Queue(CallRequirement::ScriptBackend, std::move(job));
    }

    GameCallTickStats GameCallHub::Tick(
        std::size_t maxJobs,
        std::uint64_t maxMicros,
        bool nativeReady,
        bool scriptReady) noexcept
    {
        GameCallTickStats stats{};
        if (maxJobs == 0 || maxMicros == 0)
            return stats;

        const auto start = std::chrono::steady_clock::now();
        const auto withinBudget = [start, maxMicros]() {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count()) < maxMicros;
        };

        std::size_t toInspect{};
        {
            std::scoped_lock lock(m_Mutex);
            toInspect = m_Calls.size();
        }

        while (stats.executed < maxJobs && toInspect > 0 && withinBudget())
        {
            PendingCall call;
            {
                std::scoped_lock lock(m_Mutex);
                if (m_Calls.empty())
                    break;
                call = std::move(m_Calls.front());
                m_Calls.pop();
            }
            --toInspect;

            const bool ready =
                call.requirement == CallRequirement::GameThread ||
                (call.requirement == CallRequirement::NativeBackend && nativeReady) ||
                (call.requirement == CallRequirement::ScriptBackend && scriptReady);

            if (!ready)
            {
                std::scoped_lock lock(m_Mutex);
                m_Calls.push(std::move(call));
                ++stats.deferred;
                continue;
            }

            try
            {
                call.job();
            }
            catch (...)
            {
            }
            ++stats.executed;
        }

        // Legacy scheduler compatibility is also bounded. Sick Menu frontend
        // code no longer submits here directly, but external Reaper users can.
        while (stats.executed < maxJobs && withinBudget())
        {
            try
            {
                const auto executed = Game::GameScheduler::Get().Tick(1);
                if (executed == 0)
                    break;
                stats.executed += executed;
            }
            catch (...)
            {
                break;
            }
        }

        return stats;
    }

    void GameCallHub::Reset() noexcept
    {
        {
            std::scoped_lock lock(m_Mutex);
            std::queue<PendingCall> empty;
            m_Calls.swap(empty);
        }
        Game::GameScheduler::Get().Clear();
    }

    std::size_t GameCallHub::Pending() const noexcept
    {
        std::size_t pending{};
        {
            std::scoped_lock lock(m_Mutex);
            pending = m_Calls.size();
        }
        return pending + Game::GameScheduler::Get().Pending();
    }

    bool GameCallHub::Queue(CallRequirement requirement, Job job)
    {
        if (!job)
            return false;

        std::scoped_lock lock(m_Mutex);
        if (m_Calls.size() >= MaxPending)
            return false;
        m_Calls.push(PendingCall{requirement, std::move(job)});
        return true;
    }
}
