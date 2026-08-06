#pragma once

#include "sick/memory/module.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace sick::memory
{
    class Pattern final
    {
    public:
        explicit Pattern(std::string_view signature);

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool matches(const std::uint8_t* address) const noexcept;

    private:
        std::vector<std::optional<std::uint8_t>> m_bytes;
    };

    class PatternScanner final
    {
    public:
        explicit PatternScanner(const Module& module) noexcept;

        [[nodiscard]] std::uintptr_t find(const Pattern& pattern) const noexcept;

    private:
        const Module& m_module;
    };
}
