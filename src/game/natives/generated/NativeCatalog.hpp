#pragma once

#include "../NativeTypes.hpp"
#include "NativeIndex.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace Sick::Game::Natives::Generated
{
    struct NativeDefinition
    {
        NativeIndex index{};
        NativeHash hash{};
        std::string_view name;
        std::string_view nameSpace;
        std::uint8_t argumentCount{};
        bool vectorFixup{};
    };

    inline constexpr std::array<NativeDefinition, NativeCount> NativeCatalog{
        NativeDefinition{NativeIndex::PLAYER_PED_ID, 0xD80958FC74E988A6ULL, "PLAYER_PED_ID", "PLAYER", 0, false},
        NativeDefinition{NativeIndex::PLAYER_ID, 0x4F8644AF03D0E0D6ULL, "PLAYER_ID", "PLAYER", 0, false},
        NativeDefinition{NativeIndex::DOES_ENTITY_EXIST, 0x7239B21A38F536BAULL, "DOES_ENTITY_EXIST", "ENTITY", 1, false},
        NativeDefinition{NativeIndex::SET_ENTITY_INVINCIBLE, 0x3882114BDE571AD4ULL, "SET_ENTITY_INVINCIBLE", "ENTITY", 2, false},
    };

    [[nodiscard]] constexpr const NativeDefinition* DefinitionFor(NativeIndex index) noexcept
    {
        const auto offset = ToNativeOffset(index);
        return offset < NativeCatalog.size() ? &NativeCatalog[offset] : nullptr;
    }
}
