#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>

namespace sick::core
{
    struct BuildVersion final
    {
        std::uint16_t major{};
        std::uint16_t minor{};
        std::uint16_t patch{};
        std::uint16_t revision{};

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] std::string to_string() const;
    };

    class BuildInfo final
    {
    public:
        explicit BuildInfo(HMODULE module) noexcept;

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] const BuildVersion& version() const noexcept;
        [[nodiscard]] const std::wstring& path() const noexcept;

    private:
        std::wstring m_path;
        BuildVersion m_version;
    };
}
