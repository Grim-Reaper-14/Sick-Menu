#pragma once

#include "shared/HandlingTypes.hpp"

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

    struct VehicleFeatureSnapshot
    {
        ToggleFeatureSnapshot godMode;
        ToggleFeatureSnapshot autoRepair;
        ToggleFeatureSnapshot keepClean;
        ToggleFeatureSnapshot engineAlwaysOn;
        ToggleFeatureSnapshot noGravity;
        ToggleFeatureSnapshot noCollision;
    };

    enum class VehicleSpawnerState : std::uint8_t
    {
        Idle,
        Queued,
        Loading,
        Spawned,
        NativeUnavailable,
        InvalidModel,
        TimedOut,
        Failed,
    };

    struct VehicleSpawnerSnapshot
    {
        VehicleSpawnerState state{VehicleSpawnerState::Idle};
        std::uint32_t modelHash{};
        bool busy{};
    };

    enum class OnlineSessionType : std::int32_t
    {
        Public = 0,
        SoloPublic = 1,
        ClosedCrew = 2,
        Crew = 3,
        ClosedFriends = 6,
        FindFriend = 9,
        Solo = 10,
        InviteOnly = 11,
        JoinCrew = 12,
        SpectatorTv = 13,
        LeaveOnline = -1,
    };

    enum class VehicleCustomizationCommand : std::int32_t
    {
        SetMod = 0,
        ToggleMod = 1,
        SetWheelType = 2,
        SetPrimaryModColor = 3,
        SetSecondaryModColor = 4,
        SetExtraColours = 5,
        SetInteriorColor = 6,
        SetDashboardColor = 7,
        SetNeonEnabled = 8,
        SetNeonColor = 9,
        SetTireSmokeColor = 10,
        SetXenonColor = 11,
        SetExtra = 12,
        SetBulletproofTires = 13,
        MaxVehicle = 14,
    };

    enum class SessionSwitchState : std::uint8_t
    {
        Idle,
        Queued,
        Switching,
        Complete,
        ScriptUnavailable,
        GlobalUnavailable,
        Failed,
    };

    struct SessionSwitchSnapshot
    {
        SessionSwitchState state{SessionSwitchState::Idle};
        OnlineSessionType target{OnlineSessionType::Public};
        bool busy{};
    };

    enum class PersonalVehicleSaveState : std::uint8_t
    {
        Idle,
        Queued,
        Validating,
        WaitingForGarageSelection,
        Complete,
        NativeUnavailable,
        ScriptUnavailable,
        RewardScriptUnavailable,
        GlobalUnavailable,
        NoVehicle,
        InvalidVehicle,
        AlreadyPersonal,
        Failed,
    };

    struct PersonalVehicleSaveSnapshot
    {
        PersonalVehicleSaveState state{PersonalVehicleSaveState::Idle};
        bool busy{};
    };

    struct HandlingFeatureSnapshot
    {
        bool backendAvailable{};
        bool vehicleAttached{};
        std::uint64_t revision{};
        Handling::Values values{};
    };

    struct HandlingProfileCatalogSnapshot
    {
        std::uint64_t generation{};
        std::vector<std::string> names;
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
        VehicleFeatureSnapshot vehicle;
        VehicleSpawnerSnapshot vehicleSpawner;
        SessionSwitchSnapshot sessionSwitch;
        PersonalVehicleSaveSnapshot personalVehicleSave;
        HandlingFeatureSnapshot handling;
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

    struct VehicleFeatureProfile
    {
        bool godMode{};
        bool autoRepair{};
        bool keepClean{};
        bool engineAlwaysOn{};
        bool noGravity{};
        bool noCollision{};
    };

    struct FeatureProfile
    {
        static constexpr std::uint32_t CurrentVersion = 3;

        std::uint32_t version{CurrentVersion};
        PlayerFeatureProfile player;
        VehicleFeatureProfile vehicle;
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
