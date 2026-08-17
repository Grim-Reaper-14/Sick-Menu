#pragma once

#include "backend/BackendTypes.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_set>
#include <vector>

namespace Sick::Backend::System
{
    enum class IoPriority : std::uint8_t
    {
        Critical,
        Normal,
        Maintenance,
    };

    class IoService final
    {
    public:
        using RequestId = std::uint64_t;
        using Task = std::function<void()>;

        bool Start(std::size_t workerCount = 2, std::size_t maxPending = 1024);
        void Stop() noexcept;
        [[nodiscard]] std::optional<RequestId> Submit(IoPriority priority, Task task);
        [[nodiscard]] bool Cancel(RequestId requestId) noexcept;
        [[nodiscard]] bool Running() const noexcept;
        [[nodiscard]] IoServiceSnapshot Snapshot() const noexcept;

    private:
        struct Request
        {
            RequestId id{};
            Task task;
        };

        [[nodiscard]] std::size_t PendingLocked() const noexcept;
        [[nodiscard]] bool PopLocked(Request& request) noexcept;
        void Worker() noexcept;
        void RecordPeak(std::size_t pending) noexcept;

        mutable std::mutex m_Mutex;
        std::condition_variable m_Condition;
        std::deque<Request> m_Critical;
        std::deque<Request> m_Normal;
        std::deque<Request> m_Maintenance;
        std::unordered_set<RequestId> m_Cancelled;
        std::vector<std::thread> m_Workers;
        std::size_t m_MaxPending{1024};
        bool m_Stopping{true};

        std::atomic_uint64_t m_NextRequestId{1};
        std::atomic_uint64_t m_Accepted{};
        std::atomic_uint64_t m_Completed{};
        std::atomic_uint64_t m_CancelledCount{};
        std::atomic_uint64_t m_Rejected{};
        std::atomic_size_t m_PeakPending{};
    };
}
