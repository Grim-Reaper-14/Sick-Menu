#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Sick::Runtime
{
    struct ModuleImage
    {
        std::uintptr_t base{};
        std::size_t size{};
        const std::uint8_t* text{};
        std::size_t textSize{};

        [[nodiscard]] bool Contains(std::uintptr_t address, std::size_t length = 1) const noexcept;
    };

    [[nodiscard]] std::optional<ModuleImage> LoadModuleImage(std::wstring_view name) noexcept;
    [[nodiscard]] std::optional<std::uintptr_t> FindUnique(
        const ModuleImage& image,
        std::string_view signature) noexcept;
    [[nodiscard]] std::optional<std::uintptr_t> Rip(
        const ModuleImage& image,
        std::uintptr_t displacementAddress) noexcept;
}
