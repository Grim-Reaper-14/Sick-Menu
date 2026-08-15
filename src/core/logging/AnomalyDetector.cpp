#include "AnomalyDetector.hpp"
#include "Formatting.hpp"

namespace Sick::Core::Logging
{
    AnomalyDetector& AnomalyDetector::Get() noexcept
    {
        static AnomalyDetector detector;
        return detector;
    }

    void AnomalyDetector::Configure(
        std::chrono::milliseconds window,
        std::size_t threshold) noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            m_Window = window.count() > 0 ? window : std::chrono::milliseconds{1000};
            m_Threshold = threshold == 0 ? 1 : threshold;
            m_Entries.clear();
        }
        catch (...)
        {
        }
    }

    AnomalyResult AnomalyDetector::Observe(
        Level level,
        EventId event,
        std::string_view category,
        std::string_view message) noexcept
    {
        if (level < Level::Warn)
            return {};

        try
        {
            const auto key = event != EventId::None
                ? Detail::Format("{}#{}", category, static_cast<std::uint64_t>(event))
                : Detail::Format("{}#{}", category, message);

            const auto now = std::chrono::steady_clock::now();
            std::scoped_lock lock(m_Mutex);
            auto& entry = m_Entries[key];

            if (entry.count == 0 || now - entry.windowStart >= m_Window)
            {
                entry.windowStart = now;
                entry.count = 1;
                entry.reported = false;
                return {};
            }

            ++entry.count;
            if (entry.count < m_Threshold)
                return {};

            const bool newlyDetected = !entry.reported;
            entry.reported = true;
            if (newlyDetected)
                m_Detections.fetch_add(1, std::memory_order_relaxed);

            return {true, newlyDetected, entry.count};
        }
        catch (...)
        {
            return {};
        }
    }

    std::uint64_t AnomalyDetector::DetectionCount() const noexcept
    {
        return m_Detections.load(std::memory_order_relaxed);
    }

    void AnomalyDetector::Reset() noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            m_Entries.clear();
            m_Detections.store(0, std::memory_order_relaxed);
        }
        catch (...)
        {
        }
    }
}
