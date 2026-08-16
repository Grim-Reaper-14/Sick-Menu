#pragma once

#include <cstdint>
#include <string_view>

namespace Sick::Game::Scripts
{
    using ScriptHash = std::uint32_t;

    [[nodiscard]] constexpr char ToLowerAscii(char value) noexcept
    {
        return value >= 'A' && value <= 'Z'
            ? static_cast<char>(value + ('a' - 'A'))
            : value;
    }

    [[nodiscard]] constexpr ScriptHash Joaat(std::string_view value) noexcept
    {
        ScriptHash hash{};

        for (const char character : value)
        {
            hash += static_cast<std::uint8_t>(ToLowerAscii(character));
            hash += hash << 10;
            hash ^= hash >> 6;
        }

        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
        return hash;
    }
}
