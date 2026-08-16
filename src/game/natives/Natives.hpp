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
    }
}
