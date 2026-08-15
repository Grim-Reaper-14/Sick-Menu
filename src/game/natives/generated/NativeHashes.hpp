#pragma once

#include "../NativeTypes.hpp"
#include "NativeIndex.hpp"

#include <array>

namespace Sick::Game::Natives::Generated
{
    inline constexpr std::array<NativeHash, NativeCount> NativeHashes{
        0xD80958FC74E988A6ULL,
        0x4F8644AF03D0E0D6ULL,
        0x7239B21A38F536BAULL,
        0x3882114BDE571AD4ULL,
    };

    [[nodiscard]] constexpr NativeHash HashFor(NativeIndex index) noexcept
    {
        const auto offset = ToNativeOffset(index);
        return offset < NativeHashes.size() ? NativeHashes[offset] : NativeHash{};
    }

    [[nodiscard]] constexpr NativeIndex IndexForHash(NativeHash hash) noexcept
    {
        for (std::size_t i = 0; i < NativeHashes.size(); ++i)
        {
            if (NativeHashes[i] == hash)
                return static_cast<NativeIndex>(i);
        }

        return NativeIndex::Count;
    }
}
