#pragma once

#include "NativeInvoker.hpp"
#include "NativeTypes.hpp"
#include "generated/NativeHashes.hpp"
#include "generated/NativeIndex.hpp"

namespace Sick::Game::Natives
{
    namespace Hashes
    {
        inline constexpr NativeHash PLAYER_PED_ID = Generated::HashFor(NativeIndex::PLAYER_PED_ID);
        inline constexpr NativeHash PLAYER_ID = Generated::HashFor(NativeIndex::PLAYER_ID);
        inline constexpr NativeHash DOES_ENTITY_EXIST = Generated::HashFor(NativeIndex::DOES_ENTITY_EXIST);
        inline constexpr NativeHash SET_ENTITY_INVINCIBLE = Generated::HashFor(NativeIndex::SET_ENTITY_INVINCIBLE);
        inline constexpr NativeHash GET_PLAYER_WANTED_LEVEL = Generated::HashFor(NativeIndex::GET_PLAYER_WANTED_LEVEL);
        inline constexpr NativeHash SET_PLAYER_WANTED_LEVEL = Generated::HashFor(NativeIndex::SET_PLAYER_WANTED_LEVEL);
        inline constexpr NativeHash SET_PLAYER_WANTED_LEVEL_NOW = Generated::HashFor(NativeIndex::SET_PLAYER_WANTED_LEVEL_NOW);
        inline constexpr NativeHash CLEAR_PLAYER_WANTED_LEVEL = Generated::HashFor(NativeIndex::CLEAR_PLAYER_WANTED_LEVEL);
        inline constexpr NativeHash SET_SUPER_JUMP_THIS_FRAME = Generated::HashFor(NativeIndex::SET_SUPER_JUMP_THIS_FRAME);
        inline constexpr NativeHash SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER = Generated::HashFor(NativeIndex::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER);
        inline constexpr NativeHash SET_SWIM_MULTIPLIER_FOR_PLAYER = Generated::HashFor(NativeIndex::SET_SWIM_MULTIPLIER_FOR_PLAYER);
        inline constexpr NativeHash SET_PED_MAX_TIME_UNDERWATER = Generated::HashFor(NativeIndex::SET_PED_MAX_TIME_UNDERWATER);
        inline constexpr NativeHash SET_PED_CAN_RAGDOLL = Generated::HashFor(NativeIndex::SET_PED_CAN_RAGDOLL);
        inline constexpr NativeHash SET_PED_CONFIG_FLAG = Generated::HashFor(NativeIndex::SET_PED_CONFIG_FLAG);
        inline constexpr NativeHash CLEAR_PED_ENV_DIRT = Generated::HashFor(NativeIndex::CLEAR_PED_ENV_DIRT);
        inline constexpr NativeHash RESET_PED_VISIBLE_DAMAGE = Generated::HashFor(NativeIndex::RESET_PED_VISIBLE_DAMAGE);
        inline constexpr NativeHash CLEAR_PED_BLOOD_DAMAGE = Generated::HashFor(NativeIndex::CLEAR_PED_BLOOD_DAMAGE);
        inline constexpr NativeHash SET_ENABLE_SCUBA = Generated::HashFor(NativeIndex::SET_ENABLE_SCUBA);
        inline constexpr NativeHash SET_PED_DIES_IN_WATER = Generated::HashFor(NativeIndex::SET_PED_DIES_IN_WATER);
        inline constexpr NativeHash SET_ENTITY_HAS_GRAVITY = Generated::HashFor(NativeIndex::SET_ENTITY_HAS_GRAVITY);
        inline constexpr NativeHash GET_VEHICLE_PED_IS_IN = Generated::HashFor(NativeIndex::GET_VEHICLE_PED_IS_IN);
        inline constexpr NativeHash SET_ENTITY_COLLISION = Generated::HashFor(NativeIndex::SET_ENTITY_COLLISION);
        inline constexpr NativeHash SET_VEHICLE_FIXED = Generated::HashFor(NativeIndex::SET_VEHICLE_FIXED);
        inline constexpr NativeHash SET_VEHICLE_DEFORMATION_FIXED = Generated::HashFor(NativeIndex::SET_VEHICLE_DEFORMATION_FIXED);
        inline constexpr NativeHash SET_VEHICLE_DIRT_LEVEL = Generated::HashFor(NativeIndex::SET_VEHICLE_DIRT_LEVEL);
        inline constexpr NativeHash SET_VEHICLE_ENGINE_ON = Generated::HashFor(NativeIndex::SET_VEHICLE_ENGINE_ON);
        inline constexpr NativeHash SET_VEHICLE_ON_GROUND_PROPERLY = Generated::HashFor(NativeIndex::SET_VEHICLE_ON_GROUND_PROPERLY);
        inline constexpr NativeHash SET_VEHICLE_ENGINE_HEALTH = Generated::HashFor(NativeIndex::SET_VEHICLE_ENGINE_HEALTH);
        inline constexpr NativeHash SET_VEHICLE_BODY_HEALTH = Generated::HashFor(NativeIndex::SET_VEHICLE_BODY_HEALTH);
        inline constexpr NativeHash SET_VEHICLE_PETROL_TANK_HEALTH = Generated::HashFor(NativeIndex::SET_VEHICLE_PETROL_TANK_HEALTH);
    }

    namespace PLAYER
    {
        [[nodiscard]] inline Ped PLAYER_PED_ID() noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::PLAYER_PED_ID, Ped, false>();
        }

        [[nodiscard]] inline Player PLAYER_ID() noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::PLAYER_ID, Player, false>();
        }

        [[nodiscard]] inline int GET_PLAYER_WANTED_LEVEL(Player player) noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::GET_PLAYER_WANTED_LEVEL, int, false>(player);
        }

        inline void SET_PLAYER_WANTED_LEVEL(Player player, int level, bool disableNoMission) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PLAYER_WANTED_LEVEL, void, false>(player, level, disableNoMission);
        }

        inline void SET_PLAYER_WANTED_LEVEL_NOW(Player player, bool p1) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PLAYER_WANTED_LEVEL_NOW, void, false>(player, p1);
        }

        inline void CLEAR_PLAYER_WANTED_LEVEL(Player player) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::CLEAR_PLAYER_WANTED_LEVEL, void, false>(player);
        }

        inline void SET_SUPER_JUMP_THIS_FRAME(Player player) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_SUPER_JUMP_THIS_FRAME, void, false>(player);
        }

        inline void SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(Player player, float multiplier) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER, void, false>(player, multiplier);
        }

        inline void SET_SWIM_MULTIPLIER_FOR_PLAYER(Player player, float multiplier) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_SWIM_MULTIPLIER_FOR_PLAYER, void, false>(player, multiplier);
        }
    }

    namespace PED
    {
        inline void SET_PED_MAX_TIME_UNDERWATER(Ped ped, float seconds) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PED_MAX_TIME_UNDERWATER, void, false>(ped, seconds);
        }

        inline void SET_PED_CAN_RAGDOLL(Ped ped, bool enabled) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PED_CAN_RAGDOLL, void, false>(ped, enabled);
        }

        inline void SET_PED_CONFIG_FLAG(Ped ped, int flag, bool value) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PED_CONFIG_FLAG, void, false>(ped, flag, value);
        }

        inline void CLEAR_PED_ENV_DIRT(Ped ped) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::CLEAR_PED_ENV_DIRT, void, false>(ped);
        }

        inline void RESET_PED_VISIBLE_DAMAGE(Ped ped) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::RESET_PED_VISIBLE_DAMAGE, void, false>(ped);
        }

        inline void CLEAR_PED_BLOOD_DAMAGE(Ped ped) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::CLEAR_PED_BLOOD_DAMAGE, void, false>(ped);
        }

        inline void SET_ENABLE_SCUBA(Ped ped, bool enabled) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_ENABLE_SCUBA, void, false>(ped, enabled);
        }

        inline void SET_PED_DIES_IN_WATER(Ped ped, bool enabled) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_PED_DIES_IN_WATER, void, false>(ped, enabled);
        }

        [[nodiscard]] inline Vehicle GET_VEHICLE_PED_IS_IN(Ped ped, bool includeEntering) noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::GET_VEHICLE_PED_IS_IN, Vehicle, false>(ped, includeEntering);
        }
    }

    namespace ENTITY
    {
        [[nodiscard]] inline bool DOES_ENTITY_EXIST(Entity entity) noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::DOES_ENTITY_EXIST, bool, false>(entity);
        }

        inline void SET_ENTITY_INVINCIBLE(
            Entity entity,
            bool toggle,
            bool dontResetOnCleanup = false) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_ENTITY_INVINCIBLE, void, false>(
                entity,
                toggle,
                dontResetOnCleanup);
        }

        inline void SET_ENTITY_HAS_GRAVITY(Entity entity, bool enabled) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_ENTITY_HAS_GRAVITY, void, false>(entity, enabled);
        }

        inline void SET_ENTITY_COLLISION(Entity entity, bool enabled, bool keepPhysics) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_ENTITY_COLLISION, void, false>(entity, enabled, keepPhysics);
        }
    }

    namespace VEHICLE
    {
        inline void SET_VEHICLE_FIXED(Vehicle vehicle) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_FIXED, void, false>(vehicle);
        }

        inline void SET_VEHICLE_DEFORMATION_FIXED(Vehicle vehicle) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_DEFORMATION_FIXED, void, false>(vehicle);
        }

        inline void SET_VEHICLE_DIRT_LEVEL(Vehicle vehicle, float level) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_DIRT_LEVEL, void, false>(vehicle, level);
        }

        inline void SET_VEHICLE_ENGINE_ON(
            Vehicle vehicle,
            bool enabled,
            bool instantly,
            bool disableAutoStart) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_ENGINE_ON, void, false>(
                vehicle, enabled, instantly, disableAutoStart);
        }

        [[nodiscard]] inline bool SET_VEHICLE_ON_GROUND_PROPERLY(Vehicle vehicle, float p1 = 5.0F) noexcept
        {
            return NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_ON_GROUND_PROPERLY, bool, false>(vehicle, p1);
        }

        inline void SET_VEHICLE_ENGINE_HEALTH(Vehicle vehicle, float health) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_ENGINE_HEALTH, void, false>(vehicle, health);
        }

        inline void SET_VEHICLE_BODY_HEALTH(Vehicle vehicle, float health) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_BODY_HEALTH, void, false>(vehicle, health);
        }

        inline void SET_VEHICLE_PETROL_TANK_HEALTH(Vehicle vehicle, float health) noexcept
        {
            NativeInvoker::Invoke<NativeIndex::SET_VEHICLE_PETROL_TANK_HEALTH, void, false>(vehicle, health);
        }
    }
}
