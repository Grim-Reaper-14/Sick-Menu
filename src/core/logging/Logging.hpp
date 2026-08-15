#pragma once

#include "AnomalyDetector.hpp"
#include "EventId.hpp"
#include "Formatter.hpp"
#include "Level.hpp"
#include "LogContext.hpp"
#include "LogRecord.hpp"
#include "Logger.hpp"
#include "RateLimiter.hpp"
#include "sinks/ConsoleSink.hpp"
#include "sinks/DebuggerSink.hpp"
#include "sinks/FileSink.hpp"
#include "sinks/MemorySink.hpp"

#include <chrono>

#define REAPER_DETAIL_STRINGIZE_INNER(value) #value
#define REAPER_DETAIL_STRINGIZE(value) REAPER_DETAIL_STRINGIZE_INNER(value)
#define REAPER_DETAIL_RATE_KEY __FILE__ ":" REAPER_DETAIL_STRINGIZE(__LINE__)

#define REAPER_WARN_EVERY(interval, category, format, ...) \
    do \
    { \
        if (::Sick::Core::Logging::RateLimiter::Global().Allow( \
                REAPER_DETAIL_RATE_KEY, \
                std::chrono::duration_cast<std::chrono::milliseconds>(interval))) \
        { \
            REAPER_WARN(category, format __VA_OPT__(,) __VA_ARGS__); \
        } \
    } while (false)

#define REAPER_WARN_ONCE(category, format, ...) \
    do \
    { \
        if (::Sick::Core::Logging::RateLimiter::Global().First(REAPER_DETAIL_RATE_KEY)) \
            REAPER_WARN(category, format __VA_OPT__(,) __VA_ARGS__); \
    } while (false)
