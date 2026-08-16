#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Sick::Backend::Tasking
{
    class ThreadPool final
    {
    public:
        using Task = std::function<void()>;

        bool Start(std::size_t workerCount = 2, std::size_t maxPending = 1024);
        void Stop() noexcept;
        [[nodiscard]] bool Submit(Task task);
        [[nodiscard]] std::size_t Pending() const noexcept;
        [[nodiscard]] bool Running() const noexcept;

    private:
        void Worker() noexcept;

        mutable std::mutex m_Mutex;
        std::condition_variable m_Condition;
        std::queue<Task> m_Tasks;
        std::vector<std::thread> m_Workers;
        std::size_t m_MaxPending{1024};
        bool m_Stopping{true};
    };
}
