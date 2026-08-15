#pragma once

#include "core/logging/Logger.hpp"

#include <cassert>
#include <source_location>
#include <string_view>
#include <utility>

namespace Sick::Core::Diagnostics
{
    void ReportAssertion(
        std::string_view expression,
        std::string_view category,
        std::string message,
        std::source_location source = std::source_location::current()) noexcept;

    template <typename... Args>
    void ReportAssertionAt(
        std::string_view expression,
        std::string_view category,
        std::source_location source,
        std::string_view format,
        Args&&... args) noexcept
    {
        try
        {
            ReportAssertion(
                expression,
                category,
                Logging::Detail::Format(format, std::forward<Args>(args)...),
                source);
        }
        catch (...)
        {
            ReportAssertion(expression, category, "Assertion failed", source);
        }
    }
}

#define REAPER_ASSERT(condition, category, format, ...) \
    do \
    { \
        const bool _reaperAssertionResult = static_cast<bool>(condition); \
        if (!_reaperAssertionResult) \
        { \
            ::Sick::Core::Diagnostics::ReportAssertionAt( \
                #condition, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__); \
            assert(_reaperAssertionResult); \
        } \
    } while (false)

#define REAPER_VERIFY(condition, category, format, ...) \
    ([&]() -> bool \
    { \
        const bool _reaperVerifyResult = static_cast<bool>(condition); \
        if (!_reaperVerifyResult) \
        { \
            ::Sick::Core::Diagnostics::ReportAssertionAt( \
                #condition, category, std::source_location::current(), format __VA_OPT__(,) __VA_ARGS__); \
        } \
        return _reaperVerifyResult; \
    }())
