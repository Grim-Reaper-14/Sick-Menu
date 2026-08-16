#include "GameFiberScheduler.hpp"

#include <chrono>
#include <utility>

namespace Sick::Backend::Tasking
{
    bool GameFiberScheduler::Queue(Task task)
    {
        if (!task)
            return false;

        std::scoped_lock lock(m_Mutex);
        if (m_Tasks.size() >= MaxPending)
            return false;
        m_Tasks.push(std::move(task));
        return true;
    }

    std::size_t GameFiberScheduler::Tick(std::size_t maxResumes, std::uint64_t maxMicros) noexcept
    {
        if (maxResumes == 0 || maxMicros == 0)
            return 0;

        const auto start = std::chrono::steady_clock::now();
        const auto initialPending = Pending();
        std::size_t inspected{};
        std::size_t resumed{};

        while (resumed < maxResumes && inspected < initialPending)
        {
            const auto elapsed = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count());
            if (elapsed >= maxMicros)
                break;

            Task task;
            {
                std::scoped_lock lock(m_Mutex);
                if (m_Tasks.empty())
                    break;
                task = std::move(m_Tasks.front());
                m_Tasks.pop();
            }

            bool complete = true;
            try
            {
                complete = task();
            }
            catch (...)
            {
                complete = true;
            }

            if (!complete)
                static_cast<void>(Queue(std::move(task)));

            ++resumed;
            ++inspected;
        }

        return resumed;
    }

    void GameFiberScheduler::Clear() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        std::queue<Task> empty;
        m_Tasks.swap(empty);
    }

    std::size_t GameFiberScheduler::Pending() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Tasks.size();
    }
}
