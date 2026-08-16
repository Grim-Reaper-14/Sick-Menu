#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Sick::Memory
{
    inline constexpr std::string_view MainModuleName = "<main>";

    class Module final
    {
    public:
        Module() = default;
        Module(std::string name, std::uintptr_t base, std::size_t size);

        [[nodiscard]] static std::optional<Module> FromLoaded(std::string_view name = {});

        [[nodiscard]] const std::string& Name() const noexcept;
        [[nodiscard]] std::uintptr_t Base() const noexcept;
        [[nodiscard]] std::size_t Size() const noexcept;
        [[nodiscard]] std::uintptr_t End() const noexcept;
        [[nodiscard]] bool Valid() const noexcept;
        [[nodiscard]] bool Contains(std::uintptr_t address, std::size_t length = 1) const noexcept;
        [[nodiscard]] std::span<const std::byte> Bytes() const noexcept;

    private:
        std::string m_Name;
        std::uintptr_t m_Base{};
        std::size_t m_Size{};
    };
}
