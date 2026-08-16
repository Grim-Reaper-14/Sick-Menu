#include "ScriptFunctionCatalog.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace
{
    using Sick::Game::Scripts::Joaat;
    using Sick::Game::Scripts::KnownScriptFunction;
    using Sick::Game::Scripts::ScriptFunctionSpec;

    constexpr std::array g_Catalog{
        ScriptFunctionSpec{KnownScriptFunction::ApplyMpsvData, "ApplyMPSVData", "freemode", Joaat("freemode"), "5D ? ? ? 38 2A 71", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::SendToClouds, "SendToClouds", "shop_controller", Joaat("shop_controller"), "2D 00 02 00 00 72 5D ? ? ? 72", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponKills, "GetWeaponKills", "mp_weapons", Joaat("mp_weapons"), "5D ? ? ? 39 0F 38 00", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponDeaths, "GetWeaponDeaths", "mp_weapons", Joaat("mp_weapons"), "5D ? ? ? 39 10", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponKdRatio, "GetWeaponKDRatio", "mp_weapons", Joaat("mp_weapons"), "5D ? ? ? 39 12", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponHeadshots, "GetWeaponHeadshots", "mp_weapons", Joaat("mp_weapons"), "5D ? ? ? 39 11", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponAccuracy, "GetWeaponAccuracy", "mp_weapons", Joaat("mp_weapons"), "2D 01 09 00 00", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponNameLabel, "GetWeaponNameLabel", "mp_weapons", Joaat("mp_weapons"), "2D 02 2B 00 00", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::GetWeaponDescriptionLabel, "GetWeaponDescLabel", "mp_weapons", Joaat("mp_weapons"), "2D 02 A0 00 00", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::DoTeamSwap, "DoTeamSwap", "fm_mission_controller", Joaat("fm_mission_controller"), "2D 02 04 00 00 38 00 50", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::GiveVehicleReward, "GiveVehicleReward", "am_mp_vehicle_reward", Joaat("am_mp_vehicle_reward"), "2D 0C 1E 00 00", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::IsVehicleValidForPersonalVehicle, "IsVehicleValidForPV", "freemode", Joaat("freemode"), "5D ? ? ? 2A 06 56 13 00 38 00", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetFmmcVariationCount, "GetNumFMMCVariations", "freemode", Joaat("freemode"), "5D ? ? ? 01 72 02 39 04", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::SetFmContentServerState, "SetFMContentScriptServerState", "<runtime content script>", 0, "5D ? ? ? 55 2E 00 5D", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::SetFmContentClientState, "SetFMContentScriptClientState", "<runtime content script>", 0, "5D ? ? ? 55 08 00 74", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::GetCollectibleCoordinates, "GetCollectibleCoords", "freemode", Joaat("freemode"), "5D ? ? ? 7D 2C 10", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::InitializeStreetDealerData, "InitStreetDealerData", "fm_street_dealer", Joaat("fm_street_dealer"), "2D 00 07 00 00 61", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::RunStreetDealerMenu, "RunStreetDealerMenu", "fm_street_dealer", Joaat("fm_street_dealer"), "2D 01 03 00 00 5D ? ? ? 2A", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::CompleteStandardTimeTrial, "BeatStandardTimeTrial", "freemode", Joaat("freemode"), "2D 01 19 00 00 38", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::CompleteRcTimeTrial, "BeatRCTimeTrial", "freemode", Joaat("freemode"), "2D 01 17 00 00 38 00 40", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::EndBicycleTimeTrial, "OnBTTEnd", "fm_content_bicycle_time_trial", Joaat("fm_content_bicycle_time_trial"), "64 ? ? ? 5D ? ? ? 75 77", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::EndFreemodeDelivery, "OnFreemodeDeliveryEnd", "freemode", Joaat("freemode"), "2D 0C 2B 00", 0, false},
        ScriptFunctionSpec{KnownScriptFunction::GetStreetDealerCoordinates, "GetStreetDealerCoords", "freemode", Joaat("freemode"), "5D ? ? ? 5D ? ? ? 5D ? ? ? 18 1F", 1, true},
        ScriptFunctionSpec{KnownScriptFunction::RunGunLockerMenu, "RunGunLockerMenu", "am_mp_auto_shop", Joaat("am_mp_auto_shop"), "2D 06 08 00 00 38 03 5D ? ? ? 57 03 00", 0, false},
    };

    static_assert(g_Catalog.size() == static_cast<std::size_t>(KnownScriptFunction::Count));
}

namespace Sick::Game::Scripts
{
    ScriptPointer ScriptFunctionSpec::Pointer() const
    {
        auto pointer = ScriptPointer{std::string{name}, signature};
        if (offset > 0)
            pointer = pointer.Add(offset);
        else if (offset < 0)
            pointer = pointer.Sub(-offset);

        return rip ? pointer.Rip() : pointer;
    }

    std::span<const ScriptFunctionSpec> ScriptFunctionCatalog::All() noexcept
    {
        return g_Catalog;
    }

    const ScriptFunctionSpec* ScriptFunctionCatalog::Find(KnownScriptFunction id) noexcept
    {
        const auto index = static_cast<std::size_t>(id);
        return index < g_Catalog.size() ? &g_Catalog[index] : nullptr;
    }

    const ScriptFunctionSpec* ScriptFunctionCatalog::Find(std::string_view name) noexcept
    {
        const auto found = std::ranges::find_if(g_Catalog, [name](const ScriptFunctionSpec& spec) {
            return spec.name == name;
        });
        return found != g_Catalog.end() ? &*found : nullptr;
    }
}
