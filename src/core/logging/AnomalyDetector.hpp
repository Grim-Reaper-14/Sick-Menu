#pragma once

#include "EventId.hpp"
#include "Level.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Sick::Core::Logging
{
    struct AnomalyResult
    {
        bool burst{};
        bool newlyDetected{};
        std::size_t count{};
    };

    class AnomalyDetector final
    {
    public:
        static AnomalyDetector& Get() noexcept;

        void Configure(
            std::chrono::milliseconds window,
            std::size_t threshold) noexcept;

        [[nodiscard]] AnomalyResult Observe(
            Level level,
            EventId event,
            std::string_view category,
            std::string_view message) noexcept;

        [[nodiscard]] std::uint64_t DetectionCount() const noexcept;
        void Reset() noexcept;

    private:
        struct Entry
        {
            std::chrono::steady_clock::time_point windowStart{};
            std::size_t count{};
            bool reported{};
        };

        std::mutex m_Mutex;
        std::unordered_map<std::string, Entry> m_Entries;
        std::chrono::milliseconds m_Window{1000};
        std::size_t m_Threshold{25};
        std::atomic<std::uint64_t> m_Detections{};
    };
}
