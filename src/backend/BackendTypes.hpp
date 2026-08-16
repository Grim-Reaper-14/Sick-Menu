#pragma once

#include <cstddef>
#include <cstdint>

namespace Sick::Backend
{
    struct ToggleFeatureSnapshot
    {
        bool requested{};
        bool active{};
    };

    struct PlayerFeatureSnapshot
    {
        ToggleFeatureSnapshot godMode;
    };

    struct BackendQueueSnapshot
    {
        std::size_t gameCalls{};
        std::size_t fibers{};
        std::size_t background{};
    };

    struct BackendPerformanceSnapshot
    {
        std::uint64_t lastTickMicros{};
        std::uint64_t maxTickMicros{};
        std::uint64_t overBudgetTicks{};
        std::size_t lastJobs{};
        std::size_t lastFiberResumes{};
    };

    struct BackendSnapshot
    {
        bool initialized{};
        bool nativeReady{};
        bool scriptReady{};
        PlayerFeatureSnapshot player;
        BackendQueueSnapshot queues;
        BackendPerformanceSnapshot performance;
    };

    struct PlayerFeatureProfile
    {
        bool godMode{};
    };

    struct FeatureProfile
    {
        static constexpr std::uint32_t CurrentVersion = 1;

        std::uint32_t version{CurrentVersion};
        PlayerFeatureProfile player;
    };
}
