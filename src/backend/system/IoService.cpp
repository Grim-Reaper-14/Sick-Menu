#include "IoService.hpp"

#include "backend/tasking/TaskAffinity.hpp"

#include <algorithm>
#include <utility>

namespace Sick::Backend::System
{
    bool IoService::Start(std::size_t workerCount, std::size_t maxPending)
    {
        workerCount = std::clamp<std::size_t>(workerCount, 1, 4);
        maxPending = std::max<std::size_t>(maxPending, 1);
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Stopping)
                return true;
            m_MaxPending = maxPending;
            m_Stopping = false;
            m_Cancelled.clear();
        }

        try
        {
            m_Workers.reserve(workerCount);
            for (std::size_t index = 0; index < workerCount; ++index)
                m_Workers.emplace_back([this]() { Worker(); });
            return true;
        }
        catch (...)
        {
            Stop();
            return false;
        }
    }

    void IoService::Stop() noexcept
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
        m_Critical.clear();
        m_Normal.clear();
        m_Maintenance.clear();
        m_Cancelled.clear();
    }

    std::optional<IoService::RequestId> IoService::Submit(IoPriority priority, Task task)
    {
        if (!task)
            return std::nullopt;

        const auto id = m_NextRequestId.fetch_add(1, std::memory_order_relaxed);
        std::size_t pending{};
        {
            std::scoped_lock lock(m_Mutex);
            if (m_Stopping || PendingLocked() >= m_MaxPending)
            {
                m_Rejected.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }
            Request request{id, std::move(task)};
            switch (priority)
            {
            case IoPriority::Critical: m_Critical.push_back(std::move(request)); break;
            case IoPriority::Maintenance: m_Maintenance.push_back(std::move(request)); break;
            case IoPriority::Normal:
            default: m_Normal.push_back(std::move(request)); break;
            }
            pending = PendingLocked();
        }
        m_Accepted.fetch_add(1, std::memory_order_relaxed);
        RecordPeak(pending);
        m_Condition.notify_one();
        return id;
    }

    bool IoService::Cancel(RequestId requestId) noexcept
    {
        if (requestId == 0)
            return false;
        std::scoped_lock lock(m_Mutex);
        if (m_Stopping)
            return false;
        const auto contains = [requestId](const std::deque<Request>& queue) {
            return std::any_of(queue.begin(), queue.end(), [requestId](const Request& request) {
                return request.id == requestId;
            });
        };
        if (!contains(m_Critical) && !contains(m_Normal) && !contains(m_Maintenance))
            return false;
        return m_Cancelled.insert(requestId).second;
    }

    bool IoService::Running() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return !m_Stopping && !m_Workers.empty();
    }

    IoServiceSnapshot IoService::Snapshot() const noexcept
    {
        std::size_t pending{};
        {
            std::scoped_lock lock(m_Mutex);
            pending = PendingLocked();
        }
        return {
            .accepted = m_Accepted.load(std::memory_order_acquire),
            .completed = m_Completed.load(std::memory_order_acquire),
            .cancelled = m_CancelledCount.load(std::memory_order_acquire),
            .rejected = m_Rejected.load(std::memory_order_acquire),
            .pending = pending,
            .peakPending = m_PeakPending.load(std::memory_order_acquire),
        };
    }

    std::size_t IoService::PendingLocked() const noexcept
    {
        return m_Critical.size() + m_Normal.size() + m_Maintenance.size();
    }

    bool IoService::PopLocked(Request& request) noexcept
    {
        auto pop = [&request](std::deque<Request>& queue) {
            if (queue.empty())
                return false;
            request = std::move(queue.front());
            queue.pop_front();
            return true;
        };
        return pop(m_Critical) || pop(m_Normal) || pop(m_Maintenance);
    }

    void IoService::Worker() noexcept
    {
        Tasking::ScopedTaskAffinity affinity{Tasking::TaskAffinity::Background};
        for (;;)
        {
            Request request;
            bool cancelled{};
            {
                std::unique_lock lock(m_Mutex);
                m_Condition.wait(lock, [this]() { return m_Stopping || PendingLocked() != 0; });
                if (m_Stopping && PendingLocked() == 0)
                    return;
                if (!PopLocked(request))
                    continue;
                const auto it = m_Cancelled.find(request.id);
                if (it != m_Cancelled.end())
                {
                    m_Cancelled.erase(it);
                    cancelled = true;
                }
            }

            if (cancelled)
            {
                m_CancelledCount.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            try
            {
                request.task();
            }
            catch (...)
            {
            }
            m_Completed.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void IoService::RecordPeak(std::size_t pending) noexcept
    {
        auto peak = m_PeakPending.load(std::memory_order_relaxed);
        while (pending > peak &&
               !m_PeakPending.compare_exchange_weak(peak, pending, std::memory_order_relaxed))
        {
        }
    }
}
