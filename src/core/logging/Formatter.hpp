#pragma once

#include "LogRecord.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace Sick::Core::Logging
{
    [[nodiscard]] inline std::string FormatWallTime(
        std::chrono::system_clock::time_point value)
    {
        const auto time = std::chrono::system_clock::to_time_t(value);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            value.time_since_epoch()) % 1000;

        std::ostringstream stream;
        stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
               << '.' << std::setw(3) << std::setfill('0') << milliseconds.count();
        return stream.str();
    }

    [[nodiscard]] inline std::string FormatText(const LogRecord& record)
    {
        std::ostringstream stream;
        stream << FormatWallTime(record.wallTime)
               << " [" << ToString(record.level) << "]"
               << " [" << record.category << "]"
               << " [tid=" << record.threadId;

        if (!record.threadName.empty())
            stream << ':' << record.threadName;

        stream << ']';

        if (record.correlationId != 0)
            stream << " [corr=" << record.correlationId << ']';
        if (record.spanId != 0)
            stream << " [span=" << record.spanId << ']';
        if (record.event != EventId::None)
            stream << " [event=" << static_cast<std::uint64_t>(record.event) << ']';

        stream << ' ' << record.message;

        for (const auto& field : record.fields)
            stream << ' ' << field.key << '=' << field.value;

        if (record.source.line() != 0)
        {
            stream << " (" << record.source.file_name() << ':' << record.source.line();
            if (record.source.function_name()[0] != '\0')
                stream << " " << record.source.function_name();
            stream << ')';
        }

        return stream.str();
    }

    [[nodiscard]] inline std::string FormatJson(const LogRecord& record)
    {
        std::ostringstream stream;
        stream << '{'
               << "\"sequence\":" << record.sequence << ','
               << "\"time\":\"" << Detail::EscapeJson(FormatWallTime(record.wallTime)) << "\"," 
               << "\"level\":\"" << ToString(record.level) << "\"," 
               << "\"category\":\"" << Detail::EscapeJson(record.category) << "\"," 
               << "\"message\":\"" << Detail::EscapeJson(record.message) << "\"," 
               << "\"event\":" << static_cast<std::uint64_t>(record.event) << ','
               << "\"thread_id\":" << record.threadId << ','
               << "\"correlation_id\":" << record.correlationId << ','
               << "\"span_id\":" << record.spanId << ','
               << "\"source\":{"
               << "\"file\":\"" << Detail::EscapeJson(record.source.file_name()) << "\"," 
               << "\"line\":" << record.source.line() << ','
               << "\"function\":\"" << Detail::EscapeJson(record.source.function_name()) << "\"},"
               << "\"fields\":{";

        for (std::size_t i = 0; i < record.fields.size(); ++i)
        {
            if (i != 0)
                stream << ',';

            stream << '"' << Detail::EscapeJson(record.fields[i].key) << "\":\""
                   << Detail::EscapeJson(record.fields[i].value) << '"';
        }

        stream << "}}";
        return stream.str();
    }
}
