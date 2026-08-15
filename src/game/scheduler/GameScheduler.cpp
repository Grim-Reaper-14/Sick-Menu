#include "GameScheduler.hpp"

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

        std::scoped_lock lock(m_Mutex);
        m_Jobs.push(std::move(job));
    }

    std::size_t GameScheduler::Tick(std::size_t maxJobs)
    {
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

            job();
            ++executed;
        }

        return executed;
    }

    void GameScheduler::Clear() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        std::queue<Job> empty;
        m_Jobs.swap(empty);
    }

    std::size_t GameScheduler::Pending() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Jobs.size();
    }
}
