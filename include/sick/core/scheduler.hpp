#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace sick::core
{
    class Scheduler final
    {
    public:
        using Task = std::function<void()>;

        void add(Task task);
        void tick();
        void clear() noexcept;

    private:
        std::mutex m_mutex;
        std::vector<Task> m_tasks;
    };
}
