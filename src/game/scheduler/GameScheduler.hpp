#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>

namespace Sick::Game
{
    class GameScheduler final
    {
    public:
        using Job = std::function<void()>;

        static GameScheduler& Get() noexcept;

        void Queue(Job job);
        std::size_t Tick(std::size_t maxJobs = static_cast<std::size_t>(-1));
        void Clear() noexcept;
        [[nodiscard]] std::size_t Pending() const noexcept;

        void SetSlowJobThreshold(std::chrono::milliseconds threshold) noexcept;
        [[nodiscard]] std::chrono::milliseconds SlowJobThreshold() const noexcept;

    private:
        GameScheduler() = default;

        mutable std::mutex m_Mutex;
        std::queue<Job> m_Jobs;
        std::atomic<std::int64_t> m_SlowJobThresholdMs{100};
    };
}
