#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Sick::Memory
{
    class Pattern final
    {
    public:
        using Byte = std::optional<std::uint8_t>;

        Pattern() = default;

        [[nodiscard]] static std::optional<Pattern> Parse(
            std::string_view name,
            std::string_view idaSignature);

        [[nodiscard]] const std::string& Name() const noexcept;
        [[nodiscard]] std::span<const Byte> Bytes() const noexcept;
        [[nodiscard]] std::size_t Size() const noexcept;
        [[nodiscard]] std::uint64_t Hash() const noexcept;
        [[nodiscard]] bool Empty() const noexcept;
        [[nodiscard]] bool Matches(std::span<const std::byte> image, std::size_t offset) const noexcept;

    private:
        Pattern(std::string name, std::vector<Byte> bytes, std::uint64_t hash);

        std::string m_Name;
        std::vector<Byte> m_Bytes;
        std::uint64_t m_Hash{};
    };
}
