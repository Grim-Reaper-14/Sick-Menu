#include "PlayerService.hpp"
#include "game/natives/Natives.hpp"

#include <algorithm>

namespace Sick::Game
{
    namespace
    {
        constexpr int PedConfigDrownsInWater = 3;
        constexpr int PedConfigWillFlyThroughWindscreen = 32;
        constexpr int PedConfigIsSwimming = 65;
        constexpr int PedConfigIsScuba = 135;
    }

    Ped PlayerService::LocalPed() const noexcept
    {
        return Natives::PLAYER::PLAYER_PED_ID();
    }

    Player PlayerService::LocalPlayer() const noexcept
    {
        return Natives::PLAYER::PLAYER_ID();
    }

    bool PlayerService::Exists(Ped ped) const noexcept
    {
        return ped != 0 && Natives::ENTITY::DOES_ENTITY_EXIST(ped);
    }

    void PlayerService::SetInvincible(Ped ped, bool enabled) const noexcept
    {
        if (ped != 0)
            Natives::ENTITY::SET_ENTITY_INVINCIBLE(ped, enabled);
    }

    void PlayerService::SetMaxUnderwaterTime(Ped ped, float seconds) const noexcept
    {
        if (ped != 0)
            Natives::PED::SET_PED_MAX_TIME_UNDERWATER(ped, seconds);
    }

    void PlayerService::SetCanRagdoll(Ped ped, bool enabled) const noexcept
    {
        if (ped != 0)
            Natives::PED::SET_PED_CAN_RAGDOLL(ped, enabled);
    }

    void PlayerService::SetSuperJump(Player player) const noexcept
    {
        Natives::PLAYER::SET_SUPER_JUMP_THIS_FRAME(player);
    }

    void PlayerService::SetSeatBelt(Ped ped, bool enabled) const noexcept
    {
        if (ped != 0)
            Natives::PED::SET_PED_CONFIG_FLAG(ped, PedConfigWillFlyThroughWindscreen, !enabled);
    }

    int PlayerService::WantedLevel(Player player) const noexcept
    {
        return Natives::PLAYER::GET_PLAYER_WANTED_LEVEL(player);
    }

    void PlayerService::SetWantedLevel(Player player, int level) const noexcept
    {
        level = std::clamp(level, 0, 5);
        if (level == 0)
        {
            ClearWantedLevel(player);
            return;
        }

        Natives::PLAYER::SET_PLAYER_WANTED_LEVEL(player, level, false);
        Natives::PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, false);
    }

    void PlayerService::ClearWantedLevel(Player player) const noexcept
    {
        Natives::PLAYER::CLEAR_PLAYER_WANTED_LEVEL(player);
    }

    void PlayerService::SetRunMultiplier(Player player, float multiplier) const noexcept
    {
        Natives::PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, multiplier);
    }

    void PlayerService::SetSwimMultiplier(Player player, float multiplier) const noexcept
    {
        Natives::PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(player, multiplier);
    }

    void PlayerService::Clean(Ped ped) const noexcept
    {
        if (ped == 0)
            return;
        Natives::PED::CLEAR_PED_ENV_DIRT(ped);
        Natives::PED::CLEAR_PED_BLOOD_DAMAGE(ped);
        Natives::PED::RESET_PED_VISIBLE_DAMAGE(ped);
    }

    void PlayerService::SetAqualung(Ped ped, bool enabled) const noexcept
    {
        if (ped == 0)
            return;
        Natives::PED::SET_PED_CONFIG_FLAG(ped, PedConfigIsScuba, enabled);
        Natives::PED::SET_ENABLE_SCUBA(ped, enabled);
    }

    void PlayerService::SetGravity(Ped ped, bool enabled) const noexcept
    {
        if (ped != 0)
            Natives::ENTITY::SET_ENTITY_HAS_GRAVITY(ped, enabled);
    }

    void PlayerService::SetWaterproof(Ped ped, bool enabled) const noexcept
    {
        if (ped == 0)
            return;

        Natives::PED::SET_PED_DIES_IN_WATER(ped, !enabled);
        Natives::PED::SET_PED_CONFIG_FLAG(ped, PedConfigDrownsInWater, !enabled);
        if (enabled)
        {
            // Suppress GTA's swimming/scuba flags while gravity remains enabled.
            // This is what lets the local ped sink to and traverse the sea floor.
            Natives::PED::SET_PED_CONFIG_FLAG(ped, PedConfigIsSwimming, false);
            Natives::PED::SET_PED_CONFIG_FLAG(ped, PedConfigIsScuba, false);
            Natives::PED::SET_ENABLE_SCUBA(ped, false);
        }
    }
}
