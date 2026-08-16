#pragma once

#include "NativeCatalog.hpp"

#include <array>

namespace Sick::Game::Natives::Generated
{
    inline constexpr auto NativeHashes = []
    {
        std::array<NativeHash, NativeCount> hashes{};
        for (std::size_t i = 0; i < NativeCatalog.size(); ++i)
            hashes[i] = NativeCatalog[i].hash;
        return hashes;
    }();

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
