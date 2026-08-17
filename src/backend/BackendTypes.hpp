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

    struct LoggerSnapshot
    {
        std::uint64_t accepted{};
        std::uint64_t written{};
        std::uint64_t dropped{};
        std::uint64_t sinkFailures{};
        std::uint64_t rotations{};
        std::size_t pending{};
        std::size_t peakPending{};
        std::uint64_t lastDrainMicros{};
    };

    struct FileSystemSnapshot
    {
        std::uint64_t reads{};
        std::uint64_t writes{};
        std::uint64_t bytesRead{};
        std::uint64_t bytesWritten{};
        std::uint64_t failures{};
        std::uint64_t rejectedPaths{};
        std::uint64_t rejectedAffinity{};
    };

    struct IoServiceSnapshot
    {
        std::uint64_t accepted{};
        std::uint64_t completed{};
        std::uint64_t cancelled{};
        std::uint64_t rejected{};
        std::size_t pending{};
        std::size_t peakPending{};
    };

    struct BackgroundSnapshot
    {
        LoggerSnapshot logger;
        FileSystemSnapshot filesystem;
        IoServiceSnapshot io;
    };

    struct BackendSnapshot
    {
        bool initialized{};
        bool nativeReady{};
        bool scriptReady{};
        PlayerFeatureSnapshot player;
        BackendQueueSnapshot queues;
        BackendPerformanceSnapshot performance;
        BackgroundSnapshot background;
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
