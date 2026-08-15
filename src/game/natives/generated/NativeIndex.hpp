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
        Count
    };

    inline constexpr std::size_t NativeCount = static_cast<std::size_t>(NativeIndex::Count);

    [[nodiscard]] constexpr std::size_t ToNativeOffset(NativeIndex index) noexcept
    {
        return static_cast<std::size_t>(index);
    }
}
