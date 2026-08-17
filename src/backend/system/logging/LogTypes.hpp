#pragma once

#include "backend/tasking/TaskAffinity.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace Sick::Backend::System::Logging
{
    enum class LogLevel : std::uint8_t
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
    };

    struct LogField
    {
        std::string key;
        std::string value;
    };

    struct LogRecord
    {
        std::uint64_t sequence{};
        std::chrono::system_clock::time_point timestamp{};
        std::uint64_t monotonicMicros{};
        std::uint64_t threadId{};
        Tasking::TaskAffinity affinity{Tasking::TaskAffinity::Unspecified};
        LogLevel level{LogLevel::Info};
        std::string channel;
        std::string message;
        std::vector<LogField> fields;
        const char* sourceFile{};
        const char* sourceFunction{};
        std::uint32_t sourceLine{};
    };
}
