#include "Pattern.hpp"

#include <charconv>
#include <sstream>
#include <utility>

namespace
{
    constexpr std::uint64_t FnvOffset = 14695981039346656037ULL;
    constexpr std::uint64_t FnvPrime = 1099511628211ULL;

    void HashByte(std::uint64_t& hash, std::uint8_t value) noexcept
    {
        hash ^= value;
        hash *= FnvPrime;
    }
}

namespace Sick::Memory
{
    Pattern::Pattern(std::string name, std::vector<Byte> bytes, std::uint64_t hash) :
        m_Name(std::move(name)),
        m_Bytes(std::move(bytes)),
        m_Hash(hash)
    {
    }

    std::optional<Pattern> Pattern::Parse(
        std::string_view name,
        std::string_view idaSignature)
    {
        if (name.empty() || idaSignature.empty())
            return std::nullopt;

        std::vector<Byte> bytes;
        std::istringstream stream{std::string{idaSignature}};
        std::string token;
        std::uint64_t hash = FnvOffset;

        while (stream >> token)
        {
            if (token == "?" || token == "??")
            {
                bytes.emplace_back(std::nullopt);
                HashByte(hash, 0);
                HashByte(hash, 0);
                continue;
            }

            if (token.size() != 2)
                return std::nullopt;

            unsigned int parsed{};
            const auto [end, error] = std::from_chars(
                token.data(),
                token.data() + token.size(),
                parsed,
                16);

            if (error != std::errc{} || end != token.data() + token.size() || parsed > 0xFF)
                return std::nullopt;

            const auto value = static_cast<std::uint8_t>(parsed);
            bytes.emplace_back(value);
            HashByte(hash, 1);
            HashByte(hash, value);
        }

        if (bytes.empty())
            return std::nullopt;

        return Pattern{std::string{name}, std::move(bytes), hash};
    }

    const std::string& Pattern::Name() const noexcept
    {
        return m_Name;
    }

    std::span<const Pattern::Byte> Pattern::Bytes() const noexcept
    {
        return m_Bytes;
    }

    std::size_t Pattern::Size() const noexcept
    {
        return m_Bytes.size();
    }

    std::uint64_t Pattern::Hash() const noexcept
    {
        return m_Hash;
    }

    bool Pattern::Empty() const noexcept
    {
        return m_Bytes.empty();
    }

    bool Pattern::Matches(std::span<const std::byte> image, std::size_t offset) const noexcept
    {
        if (Empty() || offset > image.size() || Size() > image.size() - offset)
            return false;

        for (std::size_t i = 0; i < m_Bytes.size(); ++i)
        {
            if (!m_Bytes[i])
                continue;

            if (std::to_integer<std::uint8_t>(image[offset + i]) != *m_Bytes[i])
                return false;
        }

        return true;
    }
}
