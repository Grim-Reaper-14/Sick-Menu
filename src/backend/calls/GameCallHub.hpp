#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>

namespace Sick::Backend::Calls
{
    enum class CallRequirement
    {
        GameThread,
        NativeBackend,
        ScriptBackend,
    };

    struct GameCallTickStats
    {
        std::size_t executed{};
        std::size_t deferred{};
    };

    class GameCallHub final
    {
    public:
        using Job = std::function<void()>;

        [[nodiscard]] bool QueueGame(Job job);
        [[nodiscard]] bool QueueNative(Job job);
        [[nodiscard]] bool QueueScript(Job job);

        GameCallTickStats Tick(
            std::size_t maxJobs,
            std::uint64_t maxMicros,
            bool nativeReady,
            bool scriptReady) noexcept;
        void Reset() noexcept;

        [[nodiscard]] std::size_t Pending() const noexcept;

    private:
        struct PendingCall
        {
            CallRequirement requirement{};
            Job job;
        };

        static constexpr std::size_t MaxPending = 512;
        [[nodiscard]] bool Queue(CallRequirement requirement, Job job);

        mutable std::mutex m_Mutex;
        std::queue<PendingCall> m_Calls;
    };
}
