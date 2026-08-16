#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>

namespace Sick::Backend::Tasking
{
    // Phase-one cooperative game-thread task contract. A task returns true
    // when complete and false when it wants to resume on a later game tick.
    // The implementation can move to stackful fibers without changing callers.
    class GameFiberScheduler final
    {
    public:
        using Task = std::function<bool()>;

        [[nodiscard]] bool Queue(Task task);
        std::size_t Tick(std::size_t maxResumes, std::uint64_t maxMicros) noexcept;
        void Clear() noexcept;
        [[nodiscard]] std::size_t Pending() const noexcept;

    private:
        static constexpr std::size_t MaxPending = 128;

        mutable std::mutex m_Mutex;
        std::queue<Task> m_Tasks;
    };
}
