#include "ScriptPointer.hpp"

#include <limits>
#include <utility>

namespace
{
    [[nodiscard]] constexpr bool IsSpace(char value) noexcept
    {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n';
    }

    [[nodiscard]] constexpr std::optional<std::uint8_t> HexDigit(char value) noexcept
    {
        if (value >= '0' && value <= '9')
            return static_cast<std::uint8_t>(value - '0');
        if (value >= 'a' && value <= 'f')
            return static_cast<std::uint8_t>(value - 'a' + 10);
        if (value >= 'A' && value <= 'F')
            return static_cast<std::uint8_t>(value - 'A' + 10);
        return std::nullopt;
    }
}

namespace Sick::Game::Scripts
{
    ScriptPattern::ScriptPattern(std::string_view signature)
    {
        for (std::size_t index = 0; index < signature.size();)
        {
            if (IsSpace(signature[index]))
            {
                ++index;
                continue;
            }

            if (signature[index] == '?')
            {
                ++index;
                if (index < signature.size() && signature[index] == '?')
                    ++index;

                m_Bytes.emplace_back(std::nullopt);
                continue;
            }

            if (index + 1 >= signature.size())
            {
                m_Bytes.clear();
                return;
            }

            const auto high = HexDigit(signature[index]);
            const auto low = HexDigit(signature[index + 1]);
            if (!high || !low)
            {
                m_Bytes.clear();
                return;
            }

            m_Bytes.emplace_back(static_cast<std::uint8_t>((*high << 4) | *low));
            index += 2;
        }

        m_Valid = !m_Bytes.empty();
    }

    bool ScriptPattern::Valid() const noexcept
    {
        return m_Valid;
    }

    std::size_t ScriptPattern::Size() const noexcept
    {
        return m_Bytes.size();
    }

    std::optional<std::uint32_t> ScriptPattern::Find(const ScriptProgramView& program) const noexcept
    {
        if (!m_Valid || !program.Valid() || m_Bytes.size() > program.CodeSize())
            return std::nullopt;

        const auto lastStart = program.CodeSize() - static_cast<std::uint32_t>(m_Bytes.size());
        for (std::uint32_t start = 0; start <= lastStart; ++start)
        {
            bool matched = true;

            for (std::size_t offset = 0; offset < m_Bytes.size(); ++offset)
            {
                if (!m_Bytes[offset])
                    continue;

                const auto* byte = program.CodeAddress(start + static_cast<std::uint32_t>(offset));
                if (!byte || *byte != *m_Bytes[offset])
                {
                    matched = false;
                    break;
                }
            }

            if (matched)
                return start;
        }

        return std::nullopt;
    }

    ScriptPointer::ScriptPointer(std::string name, std::string_view signature)
        : m_Name(std::move(name)),
          m_Pattern(signature)
    {
    }

    ScriptPointer::ScriptPointer(
        std::string name,
        ScriptPattern pattern,
        std::int64_t offset,
        bool rip)
        : m_Name(std::move(name)),
          m_Pattern(std::move(pattern)),
          m_Offset(offset),
          m_Rip(rip)
    {
    }

    ScriptPointer ScriptPointer::Add(std::int32_t offset) const
    {
        return ScriptPointer(m_Name, m_Pattern, m_Offset + offset, m_Rip);
    }

    ScriptPointer ScriptPointer::Sub(std::int32_t offset) const
    {
        return ScriptPointer(m_Name, m_Pattern, m_Offset - offset, m_Rip);
    }

    ScriptPointer ScriptPointer::Rip() const
    {
        return ScriptPointer(m_Name, m_Pattern, m_Offset, true);
    }

    bool ScriptPointer::Valid() const noexcept
    {
        return !m_Name.empty() && m_Pattern.Valid();
    }

    std::string_view ScriptPointer::Name() const noexcept
    {
        return m_Name;
    }

    std::optional<std::uint32_t> ScriptPointer::Resolve(const ScriptProgramView& program) const noexcept
    {
        const auto match = m_Pattern.Find(program);
        if (!match)
            return std::nullopt;

        const auto adjusted = static_cast<std::int64_t>(*match) + m_Offset;
        if (adjusted < 0 || adjusted > std::numeric_limits<std::uint32_t>::max())
            return std::nullopt;

        const auto address = static_cast<std::uint32_t>(adjusted);
        if (!m_Rip)
            return address < program.CodeSize() ? std::optional<std::uint32_t>{address} : std::nullopt;

        if (address >= program.CodeSize() || program.CodeSize() - address < 3)
            return std::nullopt;

        const auto* byte0 = program.CodeAddress(address);
        const auto* byte1 = program.CodeAddress(address + 1);
        const auto* byte2 = program.CodeAddress(address + 2);
        if (!byte0 || !byte1 || !byte2)
            return std::nullopt;

        const auto target = static_cast<std::uint32_t>(*byte0) |
            (static_cast<std::uint32_t>(*byte1) << 8) |
            (static_cast<std::uint32_t>(*byte2) << 16);

        return target < program.CodeSize() ? std::optional<std::uint32_t>{target} : std::nullopt;
    }
}
