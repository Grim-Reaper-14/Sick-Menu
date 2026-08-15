#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Sick::Core::Health
{
    struct Issue
    {
        std::string subsystem;
        std::chrono::milliseconds overdue{};
    };

    class Monitor final
    {
    public:
        static Monitor& Get() noexcept;
        ~Monitor();

        Monitor(const Monitor&) = delete;
        Monitor& operator=(const Monitor&) = delete;

        void Start(std::chrono::milliseconds interval = std::chrono::milliseconds{500});
        void Stop() noexcept;

        void Register(std::string_view subsystem, std::chrono::milliseconds timeout);
        void Remove(std::string_view subsystem) noexcept;
        void Heartbeat(std::string_view subsystem) noexcept;
        [[nodiscard]] std::vector<Issue> CheckNow();

    private:
        struct Entry
        {
            std::chrono::steady_clock::time_point last{};
            std::chrono::milliseconds timeout{5000};
            bool staleReported{};
        };

        Monitor() = default;
        void WorkerLoop() noexcept;

        std::mutex m_Mutex;
        std::condition_variable m_Cv;
        std::unordered_map<std::string, Entry> m_Entries;
        std::thread m_Worker;
        std::chrono::milliseconds m_Interval{500};
        std::atomic_bool m_Running{};
    };

    inline void Start(std::chrono::milliseconds interval = std::chrono::milliseconds{500})
    {
        Monitor::Get().Start(interval);
    }

    inline void Stop() noexcept { Monitor::Get().Stop(); }
    inline void Register(std::string_view subsystem, std::chrono::milliseconds timeout)
    {
        Monitor::Get().Register(subsystem, timeout);
    }
    inline void Heartbeat(std::string_view subsystem) noexcept { Monitor::Get().Heartbeat(subsystem); }
    [[nodiscard]] inline std::vector<Issue> CheckNow() { return Monitor::Get().CheckNow(); }
}
