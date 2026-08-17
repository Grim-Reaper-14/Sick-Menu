#pragma once

#include <cstddef>
#include <cstdint>

namespace Sick::Game::Natives
{
    enum class NativeIndex : std::uint16_t
    {
        PLAYER_PED_ID = 0,
        PLAYER_ID,
        DOES_ENTITY_EXIST,
        SET_ENTITY_INVINCIBLE,
        GET_PLAYER_WANTED_LEVEL,
        SET_PLAYER_WANTED_LEVEL,
        SET_PLAYER_WANTED_LEVEL_NOW,
        CLEAR_PLAYER_WANTED_LEVEL,
        SET_SUPER_JUMP_THIS_FRAME,
        SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER,
        SET_SWIM_MULTIPLIER_FOR_PLAYER,
        SET_PED_MAX_TIME_UNDERWATER,
        SET_PED_CAN_RAGDOLL,
        SET_PED_CONFIG_FLAG,
        CLEAR_PED_ENV_DIRT,
        RESET_PED_VISIBLE_DAMAGE,
        CLEAR_PED_BLOOD_DAMAGE,
        SET_ENABLE_SCUBA,
        SET_PED_DIES_IN_WATER,
        SET_ENTITY_HAS_GRAVITY,
        GET_VEHICLE_PED_IS_IN,
        SET_ENTITY_COLLISION,
        SET_VEHICLE_FIXED,
        SET_VEHICLE_DEFORMATION_FIXED,
        SET_VEHICLE_DIRT_LEVEL,
        SET_VEHICLE_ENGINE_ON,
        SET_VEHICLE_ON_GROUND_PROPERLY,
        SET_VEHICLE_ENGINE_HEALTH,
        SET_VEHICLE_BODY_HEALTH,
        SET_VEHICLE_PETROL_TANK_HEALTH,
        Count
    };

    inline constexpr std::size_t NativeCount = static_cast<std::size_t>(NativeIndex::Count);

    [[nodiscard]] constexpr std::size_t ToNativeOffset(NativeIndex index) noexcept
    {
        return static_cast<std::size_t>(index);
    }
}
