#pragma once

#include "ScriptRuntime.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Sick::Game::Scripts
{
    class ScriptPattern final
    {
    public:
        explicit ScriptPattern(std::string_view signature);

        [[nodiscard]] bool Valid() const noexcept;
        [[nodiscard]] std::size_t Size() const noexcept;
        [[nodiscard]] std::optional<std::uint32_t> Find(const ScriptProgramView& program) const noexcept;

    private:
        std::vector<std::optional<std::uint8_t>> m_Bytes;
        bool m_Valid{};
    };

    class ScriptPointer final
    {
    public:
        ScriptPointer(std::string name, std::string_view signature);

        [[nodiscard]] ScriptPointer Add(std::int32_t offset) const;
        [[nodiscard]] ScriptPointer Sub(std::int32_t offset) const;
        [[nodiscard]] ScriptPointer Rip() const;

        [[nodiscard]] bool Valid() const noexcept;
        [[nodiscard]] std::string_view Name() const noexcept;
        [[nodiscard]] std::optional<std::uint32_t> Resolve(const ScriptProgramView& program) const noexcept;

    private:
        ScriptPointer(
            std::string name,
            ScriptPattern pattern,
            std::int64_t offset,
            bool rip);

        std::string m_Name;
        ScriptPattern m_Pattern;
        std::int64_t m_Offset{};
        bool m_Rip{};
    };
}
