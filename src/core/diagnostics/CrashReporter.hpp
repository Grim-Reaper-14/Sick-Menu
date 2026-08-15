#pragma once

#include <cstddef>
#include <filesystem>
#include <string_view>

namespace Sick::Core::Crash
{
    struct Config
    {
        std::filesystem::path directory{"logs/crashes"};
        std::size_t recentEvents{256};
        bool writeMiniDump{true};
    };

    bool Install(Config config = {});
    void Uninstall() noexcept;
    void Capture(std::string_view reason) noexcept;
    [[nodiscard]] bool Installed() noexcept;
}
