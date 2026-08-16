#pragma once

#include "ScriptFunction.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace Sick::Game::Scripts
{
    enum class KnownScriptFunction : std::uint8_t
    {
        ApplyMpsvData = 0,
        SendToClouds,
        GetWeaponKills,
        GetWeaponDeaths,
        GetWeaponKdRatio,
        GetWeaponHeadshots,
        GetWeaponAccuracy,
        GetWeaponNameLabel,
        GetWeaponDescriptionLabel,
        DoTeamSwap,
        GiveVehicleReward,
        IsVehicleValidForPersonalVehicle,
        GetFmmcVariationCount,
        SetFmContentServerState,
        SetFmContentClientState,
        GetCollectibleCoordinates,
        InitializeStreetDealerData,
        RunStreetDealerMenu,
        CompleteStandardTimeTrial,
        CompleteRcTimeTrial,
        EndBicycleTimeTrial,
        EndFreemodeDelivery,
        GetStreetDealerCoordinates,
        RunGunLockerMenu,
        Count
    };

    struct ScriptFunctionSpec
    {
        KnownScriptFunction id{};
        std::string_view name;
        std::string_view scriptName;
        ScriptHash script{};
        std::string_view signature;
        std::int32_t offset{};
        bool rip{};

        [[nodiscard]] ScriptPointer Pointer() const;

        // scriptOverride is required for definitions whose script is selected at runtime.
        [[nodiscard]] ScriptFunction Bind(ScriptHash scriptOverride = 0) const
        {
            return ScriptFunction{scriptOverride != 0 ? scriptOverride : script, Pointer()};
        }
    };

    namespace ScriptFunctionCatalog
    {
        inline constexpr std::string_view ReferenceGameVersion = "GTA Online 1.73";
        inline constexpr std::string_view ReferenceEnhancedBuild = "1158.13";
        inline constexpr std::string_view ReferenceScriptsCommit =
            "30dd0df8bce87bdc21103555bae380be9fc0a916";
        inline constexpr std::string_view ReferenceYimMenuCommit =
            "36263375cfae21f34c0ff30770a1463d7d44405c";

        [[nodiscard]] std::span<const ScriptFunctionSpec> All() noexcept;
        [[nodiscard]] const ScriptFunctionSpec* Find(KnownScriptFunction id) noexcept;
        [[nodiscard]] const ScriptFunctionSpec* Find(std::string_view name) noexcept;
    }
}
