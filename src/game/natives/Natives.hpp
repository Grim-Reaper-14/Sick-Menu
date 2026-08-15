#pragma once

#include "NativeInvoker.hpp"
#include "NativeTypes.hpp"

namespace Sick::Game::Natives
{
    namespace Hashes
    {
        inline constexpr NativeHash PLAYER_PED_ID = 0xD80958FC74E988A6ULL;
        inline constexpr NativeHash PLAYER_ID = 0x4F8644AF03D0E0D6ULL;
        inline constexpr NativeHash SET_ENTITY_INVINCIBLE = 0x3882114BDE571AD4ULL;
    }

    namespace PLAYER
    {
        [[nodiscard]] inline Ped PLAYER_PED_ID() noexcept
        {
            return NativeInvoker::Call<Ped>(Hashes::PLAYER_PED_ID);
        }

        [[nodiscard]] inline Player PLAYER_ID() noexcept
        {
            return NativeInvoker::Call<Player>(Hashes::PLAYER_ID);
        }
    }

    namespace ENTITY
    {
        inline void SET_ENTITY_INVINCIBLE(Entity entity, bool toggle) noexcept
        {
            NativeInvoker::Call<void>(Hashes::SET_ENTITY_INVINCIBLE, entity, toggle);
        }
    }
}
