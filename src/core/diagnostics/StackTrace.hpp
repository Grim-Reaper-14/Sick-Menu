#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Sick::Core::Diagnostics
{
    [[nodiscard]] std::vector<std::uintptr_t> CaptureStack(
        std::size_t skip = 0,
        std::size_t maxFrames = 64) noexcept;

    [[nodiscard]] std::string FormatStack(const std::vector<std::uintptr_t>& frames);
}
