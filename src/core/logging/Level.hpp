#pragma once

#include <cstdint>
#include <string_view>

namespace Sick::Core::Logging
{
    enum class Level : std::uint8_t
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    [[nodiscard]] constexpr std::string_view ToString(Level level) noexcept
    {
        switch (level)
        {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Critical: return "CRITICAL";
        case Level::Off: return "OFF";
        }

        return "UNKNOWN";
    }

    [[nodiscard]] constexpr bool Enabled(Level value, Level minimum) noexcept
    {
        return value >= minimum && minimum != Level::Off;
    }
}
