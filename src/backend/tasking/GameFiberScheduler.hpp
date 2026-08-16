#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::Tasking
{
    // Stackful cooperative scheduler for multi-tick game-thread operations.
    // Tasks may call YieldCurrent() or WaitFor() without blocking GTA's script thread.
    class GameFiberScheduler final
    {
    public:
        using Task = std::function<void()>;

        GameFiberScheduler() = default;
        ~GameFiberScheduler();

        GameFiberScheduler(const GameFiberScheduler&) = delete;
        GameFiberScheduler& operator=(const GameFiberScheduler&) = delete;

        [[nodiscard]] bool Queue(Task task);
        std::size_t Tick(std::size_t maxResumes, std::uint64_t maxMicros) noexcept;
        void Clear() noexcept;
        [[nodiscard]] std::size_t Pending() const noexcept;

        static void YieldCurrent() noexcept;
        static void WaitFor(std::chrono::milliseconds duration) noexcept;
        [[nodiscard]] static bool InFiber() noexcept;

    private:
        struct FiberTask
        {
            GameFiberScheduler* owner{};
            Task task;
            void* handle{};
            std::chrono::steady_clock::time_point wake{};
            bool complete{};
        };

        static constexpr std::size_t MaxPending = 128;
        static constexpr std::size_t MaxActive = 32;
        static constexpr std::size_t FiberStackCommit = 64 * 1024;
        static constexpr std::size_t FiberStackReserve = 256 * 1024;

#if defined(_WIN32)
        static VOID WINAPI FiberEntry(void* parameter) noexcept;
#endif
        static void SuspendCurrentUntil(std::chrono::steady_clock::time_point wake) noexcept;

        mutable std::mutex m_PendingMutex;
        std::queue<Task> m_Pending;
        std::array<std::optional<FiberTask>, MaxActive> m_Fibers;
        std::atomic_size_t m_Active{};
        std::thread::id m_OwnerThread{};
        void* m_RootFiber{};

        static thread_local FiberTask* t_CurrentFiber;
    };
}
