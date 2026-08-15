#pragma once

#include "EventId.hpp"
#include "Formatting.hpp"
#include "Level.hpp"

#include <chrono>
#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Sick::Core::Logging
{
    struct Field
    {
        std::string key;
        std::string value;
    };

    template <typename T>
    [[nodiscard]] inline Field MakeField(std::string_view key, T&& value)
    {
        return {std::string{key}, Detail::ToText(std::forward<T>(value))};
    }

    struct LogRecord
    {
        std::uint64_t sequence{};
        std::chrono::system_clock::time_point wallTime{};
        std::chrono::steady_clock::time_point monotonicTime{};
        Level level{Level::Info};
        EventId event{EventId::None};
        std::string category;
        std::string message;
        std::vector<Field> fields;
        std::uint64_t threadId{};
        std::string threadName;
        std::uint64_t correlationId{};
        std::uint64_t spanId{};
        std::source_location source{};
    };
}
