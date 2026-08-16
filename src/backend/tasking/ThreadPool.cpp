#include "ThreadPool.hpp"

#include <algorithm>
#include <utility>

namespace Sick::Backend::Tasking
{
    bool ThreadPool::Start(std::size_t workerCount, std::size_t maxPending)
    {
        workerCount = std::max<std::size_t>(workerCount, 1);
        maxPending = std::max<std::size_t>(maxPending, 1);

        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Stopping)
                return true;
            m_MaxPending = maxPending;
            m_Stopping = false;
        }

        try
        {
            m_Workers.reserve(workerCount);
            for (std::size_t index = 0; index < workerCount; ++index)
                m_Workers.emplace_back([this]() { Worker(); });
        }
        catch (...)
        {
            Stop();
            return false;
        }

        return true;
    }

    void ThreadPool::Stop() noexcept
    {
        {
            std::scoped_lock lock(m_Mutex);
            m_Stopping = true;
        }
        m_Condition.notify_all();

        for (auto& worker : m_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        m_Workers.clear();

        std::scoped_lock lock(m_Mutex);
        std::queue<Task> empty;
        m_Tasks.swap(empty);
    }

    bool ThreadPool::Submit(Task task)
    {
        if (!task)
            return false;

        {
            std::scoped_lock lock(m_Mutex);
            if (m_Stopping || m_Tasks.size() >= m_MaxPending)
                return false;
            m_Tasks.push(std::move(task));
        }
        m_Condition.notify_one();
        return true;
    }

    std::size_t ThreadPool::Pending() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Tasks.size();
    }

    bool ThreadPool::Running() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return !m_Stopping && !m_Workers.empty();
    }

    void ThreadPool::Worker() noexcept
    {
        for (;;)
        {
            Task task;
            {
                std::unique_lock lock(m_Mutex);
                m_Condition.wait(lock, [this]() {
                    return m_Stopping || !m_Tasks.empty();
                });

                if (m_Stopping && m_Tasks.empty())
                    return;

                task = std::move(m_Tasks.front());
                m_Tasks.pop();
            }

            try
            {
                task();
            }
            catch (...)
            {
            }
        }
    }
}
