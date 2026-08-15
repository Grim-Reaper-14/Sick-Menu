#pragma once

#include <atomic>
#include <cstdint>
#include <string_view>

namespace Sick::Game::Enhanced
{
    using BuildId = std::uint32_t;
    inline constexpr BuildId UnknownBuild = 0;

    class BuildManager final
    {
    public:
        static void SetBuild(BuildId build) noexcept;
        [[nodiscard]] static BuildId Current() noexcept;
        [[nodiscard]] static bool Supported() noexcept;
        [[nodiscard]] static std::string_view EditionName() noexcept;

    private:
        static inline std::atomic<BuildId> s_Build{UnknownBuild};
    };
}
