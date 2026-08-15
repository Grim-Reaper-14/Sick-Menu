#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Sick::Core::Metrics
{
    struct Distribution
    {
        std::uint64_t count{};
        double total{};
        double minimum{};
        double maximum{};
    };

    struct Snapshot
    {
        std::unordered_map<std::string, std::int64_t> counters;
        std::unordered_map<std::string, double> gauges;
        std::unordered_map<std::string, Distribution> distributions;
    };

    void Increment(std::string_view name, std::int64_t amount = 1) noexcept;
    void SetGauge(std::string_view name, double value) noexcept;
    void Observe(std::string_view name, double value) noexcept;
    [[nodiscard]] Snapshot GetSnapshot();
    void Reset() noexcept;
}
