#include "Metrics.hpp"

#include <algorithm>
#include <mutex>

namespace
{
    std::mutex g_MetricsMutex;
    Sick::Core::Metrics::Snapshot g_Metrics;
}

namespace Sick::Core::Metrics
{
    void Increment(std::string_view name, std::int64_t amount) noexcept
    {
        try
        {
            std::scoped_lock lock(g_MetricsMutex);
            g_Metrics.counters[std::string{name}] += amount;
        }
        catch (...)
        {
        }
    }

    void SetGauge(std::string_view name, double value) noexcept
    {
        try
        {
            std::scoped_lock lock(g_MetricsMutex);
            g_Metrics.gauges[std::string{name}] = value;
        }
        catch (...)
        {
        }
    }

    void Observe(std::string_view name, double value) noexcept
    {
        try
        {
            std::scoped_lock lock(g_MetricsMutex);
            auto& distribution = g_Metrics.distributions[std::string{name}];

            if (distribution.count == 0)
            {
                distribution.minimum = value;
                distribution.maximum = value;
            }
            else
            {
                distribution.minimum = std::min(distribution.minimum, value);
                distribution.maximum = std::max(distribution.maximum, value);
            }

            ++distribution.count;
            distribution.total += value;
        }
        catch (...)
        {
        }
    }

    Snapshot GetSnapshot()
    {
        std::scoped_lock lock(g_MetricsMutex);
        return g_Metrics;
    }

    void Reset() noexcept
    {
        try
        {
            std::scoped_lock lock(g_MetricsMutex);
            g_Metrics = {};
        }
        catch (...)
        {
        }
    }
}
