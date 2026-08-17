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
        0xE28E54788CE8F12DULL,
        0x39FF19C64EF7DA5BULL,
        0xE0A7D1E497FFCD6FULL,
        0xB302540597885499ULL,
        0x57FFF03E423A4C0BULL,
        0x6DB47AA77FD94E09ULL,
        0xA91C6F0FF7D16A13ULL,
        0x6BA428C528D9E522ULL,
        0xB128377056A54E2AULL,
        0x1913FE4CBF41C463ULL,
        0x6585D955A68452A5ULL,
        0x3AC1F7B898F30C05ULL,
        0x8FE22675A5A45817ULL,
        0xF99F62004024D506ULL,
        0x56CEF0AC79073BDEULL,
        0x4A4722448F18EEF5ULL,
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
