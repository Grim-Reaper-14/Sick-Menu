#pragma once

#include "../NativeTypes.hpp"
#include "NativeIndex.hpp"

#include <array>

namespace Sick::Game::Natives::Generated
{
    // GTA V Enhanced (Gen9) native hashes translated from the canonical hashes.
    // Mapping values were validated against YimMenuV2 enhanced/src/game/gta/invoker/crossmap.txt.
    // Keep this array in NativeIndex order so the runtime can
    // hand the translated values to InitNativeTables and cache the handlers by
    // the stable Sick-Menu index.
    inline constexpr std::array<NativeHash, NativeCount> EnhancedNativeHashes{
        0x4A8C381C258A124DULL,
        0x259BE71D8A81D4FAULL,
        0xFC8BFE4B41177C22ULL,
        0x935364B4448CD584ULL,
        0xE7B45027762DEFE7ULL,
        0xE20A252886E4FE1DULL,
        0x42C9A22D6724F283ULL,
        0x3C482AC51A8E85DCULL,
        0x353BF8D85390AA39ULL,
        0xA52E1AE3848A506BULL,
        0x289497A4BA9049E0ULL,
        0x0ACCC8916441860AULL,
        0x9FF00EA9A61211D2ULL,
        0x0428AFDCAA63B06EULL,
        0xD81F5EA29FD2682EULL,
        0x69AE13B08EFD8497ULL,
        0x8EA9C5E0178372E1ULL,
        0x61BBBE1B9F8AC7D0ULL,
        0x34A9A872D3C510BFULL,
        0xBF861D73D95BF415ULL,
        0x6EF03BE64E058E2FULL,
        0x44C48AC14D3C09EDULL,
        0xF698038C13845696ULL,
        0x1D1124C855316790ULL,
        0x9452FE4900245259ULL,
        0xC229299217554C78ULL,
        0x1DE99C193C7EC64BULL,
        0x2AEBE39F6BF7D6BCULL,
        0x3E7E7AD923FD91A7ULL,
        0xDF9DC0584881B7AFULL,
        0xD1A6A821F5AC81DBULL,
        0xCFC0C995455A6204ULL,
        0x4B423FAA24E8ABF0ULL,
        0xE7D342E0F16AAA8FULL,
        0xAD1840C2E6AF7D5EULL,
        0xEC9DAA34BBB4658CULL,
        0x6252BC0DD8A320DBULL,
        0x55098D9E9AD58806ULL,
        0x5779387E956077A6ULL,
        0x73CAFD2038E812B3ULL,
        0x1F2AA07F00B3217AULL,
        0xE38E9162A2500646ULL,
        0x6AF0636DDEDCB6DDULL,
        0x2A1F4F37F95BAD08ULL,
        0x487EB21CC7295BA1ULL,
        0x43FEB945EE7F85B8ULL,
        0x816562BADFDEC83EULL,
        0xD133EF7430EDCD09ULL,
        0x2036F561ADD12E33ULL,
        0xF40DD601A65F7F19ULL,
        0x6089CDF6A57F326CULL,
        0x2AA720E4287BF269ULL,
        0x8E0A582209A62695ULL,
        0xB5BA80F839791C0FULL,
        0xE41033B25D003A07ULL,
        0x7EE3A3C5E4A40CC9ULL,
        0x1262D55792428154ULL,
        0xEB9DC3C7D8596C46ULL,
    };

    static_assert(EnhancedNativeHashes.size() == NativeCount);

    [[nodiscard]] constexpr NativeHash EnhancedHashFor(NativeIndex index) noexcept
    {
        const auto offset = ToNativeOffset(index);
        return offset < EnhancedNativeHashes.size() ? EnhancedNativeHashes[offset] : NativeHash{};
    }
}
