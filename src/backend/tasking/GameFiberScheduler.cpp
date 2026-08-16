#include "GameFiberScheduler.hpp"

#include "TaskAffinity.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace Sick::Backend::Tasking
{
    thread_local GameFiberScheduler::FiberTask* GameFiberScheduler::t_CurrentFiber{};

    GameFiberScheduler::~GameFiberScheduler()
    {
        Clear();
    }

    bool GameFiberScheduler::Queue(Task task)
    {
        if (!task)
            return false;

        std::scoped_lock lock(m_PendingMutex);
        if (m_Pending.size() + m_Active.load(std::memory_order_acquire) >= MaxPending)
            return false;
        m_Pending.push(std::move(task));
        return true;
    }

    std::size_t GameFiberScheduler::Tick(std::size_t maxResumes, std::uint64_t maxMicros) noexcept
    {
        if (maxResumes == 0 || maxMicros == 0 || !HasTaskAffinity(TaskAffinity::Game))
            return 0;

        const auto thread = std::this_thread::get_id();
        if (m_OwnerThread == std::thread::id{})
            m_OwnerThread = thread;
        else if (m_OwnerThread != thread)
            return 0;

        const auto start = std::chrono::steady_clock::now();
        const auto withinBudget = [start, maxMicros]() noexcept {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count()) < maxMicros;
        };

#if defined(_WIN32)
        bool convertedThisTick = false;
        void* rootFiber{};
        if (IsThreadAFiber())
        {
            rootFiber = GetCurrentFiber();
        }
        else
        {
            rootFiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
            convertedThisTick = rootFiber != nullptr;
        }

        if (!rootFiber)
            return 0;

        m_RootFiber = rootFiber;

        const auto active = std::min<std::size_t>(
            m_Active.load(std::memory_order_acquire), MaxActive);
        const auto createLimit = std::min<std::size_t>(maxResumes, MaxActive - active);
        std::size_t created{};
        for (auto& slot : m_Fibers)
        {
            if (created >= createLimit || !withinBudget())
                break;
            if (slot.has_value())
                continue;

            Task task;
            {
                std::scoped_lock lock(m_PendingMutex);
                if (m_Pending.empty())
                    break;
                task = std::move(m_Pending.front());
                m_Pending.pop();
            }

            slot.emplace();
            slot->owner = this;
            slot->task = std::move(task);
            slot->wake = std::chrono::steady_clock::time_point::min();
            slot->handle = CreateFiberEx(
                FiberStackCommit,
                FiberStackReserve,
                FIBER_FLAG_FLOAT_SWITCH,
                &GameFiberScheduler::FiberEntry,
                &*slot);

            if (!slot->handle)
            {
                Task retry = std::move(slot->task);
                slot.reset();
                std::scoped_lock lock(m_PendingMutex);
                m_Pending.push(std::move(retry));
                break;
            }

            m_Active.fetch_add(1, std::memory_order_release);
            ++created;
        }

        const auto now = std::chrono::steady_clock::now();
        std::array<FiberTask*, MaxActive> ready{};
        std::size_t readyCount{};
        for (auto& slot : m_Fibers)
        {
            if (readyCount >= std::min(maxResumes, MaxActive))
                break;
            if (slot && !slot->complete && slot->wake <= now)
                ready[readyCount++] = &*slot;
        }

        std::size_t resumed{};
        for (std::size_t index = 0; index < readyCount; ++index)
        {
            if (!withinBudget())
                break;
            SwitchToFiber(ready[index]->handle);
            ++resumed;
        }

        for (auto& slot : m_Fibers)
        {
            if (!slot || !slot->complete)
                continue;
            if (slot->handle)
                DeleteFiber(slot->handle);
            slot.reset();
            m_Active.fetch_sub(1, std::memory_order_release);
        }

        m_RootFiber = nullptr;
        if (convertedThisTick)
            static_cast<void>(ConvertFiberToThread());
        return resumed;
#else
        std::size_t resumed{};
        while (resumed < maxResumes && withinBudget())
        {
            Task task;
            {
                std::scoped_lock lock(m_PendingMutex);
                if (m_Pending.empty())
                    break;
                task = std::move(m_Pending.front());
                m_Pending.pop();
            }

            try
            {
                task();
            }
            catch (...)
            {
            }
            ++resumed;
        }
        return resumed;
#endif
    }

    void GameFiberScheduler::Clear() noexcept
    {
        {
            std::scoped_lock lock(m_PendingMutex);
            std::queue<Task> empty;
            m_Pending.swap(empty);
        }

#if defined(_WIN32)
        for (auto& slot : m_Fibers)
        {
            if (slot && slot->handle)
                DeleteFiber(slot->handle);
            slot.reset();
        }
#else
        for (auto& slot : m_Fibers)
            slot.reset();
#endif
        m_Active.store(0, std::memory_order_release);
        m_OwnerThread = {};
        m_RootFiber = nullptr;
    }

    std::size_t GameFiberScheduler::Pending() const noexcept
    {
        std::scoped_lock lock(m_PendingMutex);
        return m_Pending.size() + m_Active.load(std::memory_order_acquire);
    }

    void GameFiberScheduler::Yield() noexcept
    {
        SuspendCurrentUntil(std::chrono::steady_clock::now());
    }

    void GameFiberScheduler::WaitFor(std::chrono::milliseconds duration) noexcept
    {
        if (duration < std::chrono::milliseconds::zero())
            duration = std::chrono::milliseconds::zero();
        SuspendCurrentUntil(std::chrono::steady_clock::now() + duration);
    }

    bool GameFiberScheduler::InFiber() noexcept
    {
        return t_CurrentFiber != nullptr;
    }

    void GameFiberScheduler::SuspendCurrentUntil(
        std::chrono::steady_clock::time_point wake) noexcept
    {
#if defined(_WIN32)
        auto* fiber = t_CurrentFiber;
        if (!fiber || !fiber->owner || !fiber->owner->m_RootFiber)
            return;

        fiber->wake = wake;
        t_CurrentFiber = nullptr;
        SwitchToFiber(fiber->owner->m_RootFiber);
        t_CurrentFiber = fiber;
#else
        static_cast<void>(wake);
#endif
    }

#if defined(_WIN32)
    VOID WINAPI GameFiberScheduler::FiberEntry(void* parameter) noexcept
    {
        auto* fiber = static_cast<FiberTask*>(parameter);
        if (!fiber || !fiber->owner)
            ExitThread(0);

        t_CurrentFiber = fiber;
        try
        {
            fiber->task();
        }
        catch (...)
        {
        }

        fiber->complete = true;
        t_CurrentFiber = nullptr;
        SwitchToFiber(fiber->owner->m_RootFiber);

        // A completed fiber is deleted by the root scheduler and must never be
        // resumed. Never return from a Windows fiber entry point: returning
        // would terminate the thread that scheduled it.
        for (;;)
            SwitchToFiber(fiber->owner->m_RootFiber);
    }
#endif
}
