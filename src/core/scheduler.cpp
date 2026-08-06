#include "sick/core/scheduler.hpp"

#include <utility>

namespace sick::core
{
    void Scheduler::add(Task task)
    {
        if (!task)
            return;

        std::scoped_lock lock(m_mutex);
        m_tasks.emplace_back(std::move(task));
    }

    void Scheduler::tick()
    {
        std::vector<Task> tasks;

        {
            std::scoped_lock lock(m_mutex);
            tasks = m_tasks;
        }

        for (auto& task : tasks)
            task();
    }

    void Scheduler::clear() noexcept
    {
        std::scoped_lock lock(m_mutex);
        m_tasks.clear();
    }
}
