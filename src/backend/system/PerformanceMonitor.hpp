#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace Sick::Backend::System
{
    struct PerformanceSnapshot
    {
        std::uint64_t lastTickMicros{};
        std::uint64_t maxTickMicros{};
        std::uint64_t overBudgetTicks{};
        std::size_t lastJobs{};
        std::size_t lastFiberResumes{};
    };

    class PerformanceMonitor final
    {
    public:
        void Record(
            std::uint64_t elapsedMicros,
            std::uint64_t budgetMicros,
            std::size_t jobs,
            std::size_t fiberResumes) noexcept;
        [[nodiscard]] PerformanceSnapshot Snapshot() const noexcept;
        void Reset() noexcept;

    private:
        std::atomic_uint64_t m_LastTickMicros{};
        std::atomic_uint64_t m_MaxTickMicros{};
        std::atomic_uint64_t m_OverBudgetTicks{};
        std::atomic_size_t m_LastJobs{};
        std::atomic_size_t m_LastFiberResumes{};
    };
}
