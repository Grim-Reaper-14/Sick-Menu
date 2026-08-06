#include "sick/memory/pattern_scanner.hpp"

#include <charconv>
#include <cctype>

namespace sick::memory
{
    Pattern::Pattern(const std::string_view signature)
    {
        std::size_t index = 0;

        while (index < signature.size())
        {
            while (index < signature.size() && std::isspace(static_cast<unsigned char>(signature[index])) != 0)
                ++index;

            if (index >= signature.size())
                break;

            if (signature[index] == '?')
            {
                m_bytes.emplace_back(std::nullopt);
                ++index;

                if (index < signature.size() && signature[index] == '?')
                    ++index;

                continue;
            }

            if (index + 2 > signature.size())
            {
                m_bytes.clear();
                return;
            }

            unsigned int value{};
            const char* begin = signature.data() + index;
            const char* end = begin + 2;
            const auto result = std::from_chars(begin, end, value, 16);

            if (result.ec != std::errc{} || result.ptr != end || value > 0xFF)
            {
                m_bytes.clear();
                return;
            }

            m_bytes.emplace_back(static_cast<std::uint8_t>(value));
            index += 2;
        }
    }

    bool Pattern::valid() const noexcept
    {
        return !m_bytes.empty();
    }

    std::size_t Pattern::size() const noexcept
    {
        return m_bytes.size();
    }

    bool Pattern::matches(const std::uint8_t* address) const noexcept
    {
        if (address == nullptr)
            return false;

        for (std::size_t i = 0; i < m_bytes.size(); ++i)
        {
            if (m_bytes[i].has_value() && address[i] != m_bytes[i].value())
                return false;
        }

        return true;
    }

    PatternScanner::PatternScanner(const Module& module) noexcept
        : m_module(module)
    {
    }

    std::uintptr_t PatternScanner::find(const Pattern& pattern) const noexcept
    {
        if (!m_module.valid() || !pattern.valid() || pattern.size() > m_module.size())
            return 0;

        const auto* bytes = reinterpret_cast<const std::uint8_t*>(m_module.base());
        const std::size_t last = m_module.size() - pattern.size();

        for (std::size_t offset = 0; offset <= last; ++offset)
        {
            if (pattern.matches(bytes + offset))
                return m_module.base() + offset;
        }

        return 0;
    }
}
