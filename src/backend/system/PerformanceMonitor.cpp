#include "PerformanceMonitor.hpp"

#include <algorithm>

namespace Sick::Backend::System
{
    void PerformanceMonitor::Record(
        std::uint64_t elapsedMicros,
        std::uint64_t budgetMicros,
        std::size_t jobs,
        std::size_t fiberResumes) noexcept
    {
        m_LastTickMicros.store(elapsedMicros, std::memory_order_release);
        m_LastJobs.store(jobs, std::memory_order_release);
        m_LastFiberResumes.store(fiberResumes, std::memory_order_release);

        auto maximum = m_MaxTickMicros.load(std::memory_order_relaxed);
        while (elapsedMicros > maximum &&
               !m_MaxTickMicros.compare_exchange_weak(
                   maximum,
                   elapsedMicros,
                   std::memory_order_release,
                   std::memory_order_relaxed))
        {
        }

        if (elapsedMicros > budgetMicros)
            m_OverBudgetTicks.fetch_add(1, std::memory_order_relaxed);
    }

    PerformanceSnapshot PerformanceMonitor::Snapshot() const noexcept
    {
        return {
            .lastTickMicros = m_LastTickMicros.load(std::memory_order_acquire),
            .maxTickMicros = m_MaxTickMicros.load(std::memory_order_acquire),
            .overBudgetTicks = m_OverBudgetTicks.load(std::memory_order_acquire),
            .lastJobs = m_LastJobs.load(std::memory_order_acquire),
            .lastFiberResumes = m_LastFiberResumes.load(std::memory_order_acquire),
        };
    }

    void PerformanceMonitor::Reset() noexcept
    {
        m_LastTickMicros.store(0, std::memory_order_release);
        m_MaxTickMicros.store(0, std::memory_order_release);
        m_OverBudgetTicks.store(0, std::memory_order_release);
        m_LastJobs.store(0, std::memory_order_release);
        m_LastFiberResumes.store(0, std::memory_order_release);
    }
}
