#include "backend/tasking/GameFiberScheduler.hpp"
#include "backend/tasking/TaskAffinity.hpp"
#include "backend/tasking/ThreadPool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace
{
    using namespace std::chrono_literals;
    using Sick::Backend::Tasking::CurrentTaskAffinity;
    using Sick::Backend::Tasking::GameFiberScheduler;
    using Sick::Backend::Tasking::ScopedTaskAffinity;
    using Sick::Backend::Tasking::TaskAffinity;
    using Sick::Backend::Tasking::ThreadPool;

    void TestBackgroundAffinity()
    {
        ThreadPool pool;
        assert(pool.Start(1, 8));

        std::promise<bool> result;
        auto future = result.get_future();
        assert(pool.Submit([&result]() {
            result.set_value(CurrentTaskAffinity() == TaskAffinity::Background);
        }));

        assert(future.wait_for(2s) == std::future_status::ready);
        assert(future.get());
        pool.Stop();
    }

    void TestFiberYieldAndWait()
    {
        GameFiberScheduler scheduler;
        std::vector<int> steps;

        assert(scheduler.Queue([&steps]() {
            assert(CurrentTaskAffinity() == TaskAffinity::Game);
            assert(GameFiberScheduler::InFiber());
            steps.push_back(1);
            GameFiberScheduler::Yield();
            steps.push_back(2);
            GameFiberScheduler::WaitFor(25ms);
            steps.push_back(3);
        }));

        ScopedTaskAffinity gameAffinity{TaskAffinity::Game};
#if defined(_WIN32)
        const bool wasFiber = IsThreadAFiber() != FALSE;
#endif
        assert(scheduler.Tick(1, 50000) == 1);
        assert((steps == std::vector<int>{1}));
        assert(scheduler.Pending() == 1);
#if defined(_WIN32)
        assert(wasFiber || IsThreadAFiber() == FALSE);
#endif

        assert(scheduler.Tick(1, 50000) == 1);
        assert((steps == std::vector<int>{1, 2}));
        assert(scheduler.Pending() == 1);

        assert(scheduler.Tick(1, 50000) == 0);
        std::this_thread::sleep_for(30ms);
        assert(scheduler.Tick(1, 50000) == 1);
        assert((steps == std::vector<int>{1, 2, 3}));
        assert(scheduler.Pending() == 0);
    }

    void TestFiberBudgetsAndLimits()
    {
        GameFiberScheduler scheduler;
        std::atomic_int ran{};
        for (int index = 0; index < 128; ++index)
        {
            assert(scheduler.Queue([&ran]() {
                ran.fetch_add(1, std::memory_order_relaxed);
            }));
        }
        assert(!scheduler.Queue([] {}));

        ScopedTaskAffinity gameAffinity{TaskAffinity::Game};
        assert(scheduler.Tick(2, 50000) == 2);
        assert(ran.load(std::memory_order_relaxed) == 2);
        assert(scheduler.Pending() == 126);
        scheduler.Clear();
        assert(scheduler.Pending() == 0);
    }
}

int main()
{
    TestBackgroundAffinity();
    TestFiberYieldAndWait();
    TestFiberBudgetsAndLimits();
    return 0;
}
