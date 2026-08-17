#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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
        ToggleFeatureSnapshot infiniteOxygen;
        ToggleFeatureSnapshot noRagdoll;
        ToggleFeatureSnapshot superJump;
        ToggleFeatureSnapshot seatBelt;
        ToggleFeatureSnapshot noWantedLevel;
        int wantedLevel{};
        ToggleFeatureSnapshot fastRun;
        ToggleFeatureSnapshot fastSwim;
        ToggleFeatureSnapshot keepPlayerClean;
        ToggleFeatureSnapshot aqualung;
        ToggleFeatureSnapshot noGravity;
        ToggleFeatureSnapshot waterproof;
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
        bool infiniteOxygen{};
        bool noRagdoll{};
        bool superJump{};
        bool seatBelt{};
        bool noWantedLevel{};
        int wantedLevel{};
        bool fastRun{};
        bool fastSwim{};
        bool keepPlayerClean{};
        bool aqualung{};
        bool noGravity{};
        bool waterproof{};
    };

    struct FeatureProfile
    {
        static constexpr std::uint32_t CurrentVersion = 2;

        std::uint32_t version{CurrentVersion};
        PlayerFeatureProfile player;
    };

    struct AssetEntry
    {
        std::string name;
        std::string path;
    };

    struct ThemePalette
    {
        std::uint32_t border{0x23324DFF};
        std::uint32_t header{0x000732FF};
        std::uint32_t headerBand{0x000B46FF};
        std::uint32_t title{0x020409FF};
        std::uint32_t body{0x070D16FF};
        std::uint32_t footer{0x020409FF};
        std::uint32_t selected{0xF6F6F6FF};
        std::uint32_t text{0xF4F4F6FF};
        std::uint32_t selectedText{0x101216FF};
        std::uint32_t disabledText{0x747A84FF};
        std::uint32_t accent{0xE20052FF};
        std::uint32_t inactiveToggle{0x424854FF};
        std::uint32_t logoCyan{0x29D6FFFF};
        std::uint32_t logoMagenta{0xE200C6FF};
        std::uint32_t logoShadow{0x1C0850FF};
    };

    struct ThemeEntry
    {
        AssetEntry asset;
        ThemePalette palette;
    };

    struct AssetCatalogSnapshot
    {
        std::uint64_t generation{};
        std::vector<ThemeEntry> themes;
        std::vector<AssetEntry> images;
        std::vector<AssetEntry> fonts;
        std::vector<AssetEntry> scripts;
    };

    struct MenuPreferences
    {
        float scale{1.25F};
        float left{48.0F};
        float top{28.0F};
        std::string theme{"Default"};
        std::string banner;
        std::string font;
    };
}
