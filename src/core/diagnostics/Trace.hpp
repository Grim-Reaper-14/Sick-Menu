#pragma once

#include "core/logging/LogRecord.hpp"

#include <chrono>
#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Sick::Core::Trace
{
    class Span final
    {
    public:
        Span(
            std::string_view category,
            std::string_view name,
            std::chrono::milliseconds budget = {},
            std::source_location source = std::source_location::current());
        ~Span() noexcept;

        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;

        template <typename T>
        Span& Field(std::string_view key, T&& value)
        {
            m_Fields.push_back(Logging::MakeField(key, std::forward<T>(value)));
            return *this;
        }

        [[nodiscard]] std::uint64_t Id() const noexcept { return m_Id; }

    private:
        std::string m_Category;
        std::string m_Name;
        std::chrono::milliseconds m_Budget{};
        std::chrono::steady_clock::time_point m_Started{};
        std::source_location m_Source{};
        std::vector<Logging::Field> m_Fields;
        std::uint64_t m_Id{};
        std::uint64_t m_PreviousSpan{};
    };
}

#define REAPER_DETAIL_JOIN_INNER(a, b) a##b
#define REAPER_DETAIL_JOIN(a, b) REAPER_DETAIL_JOIN_INNER(a, b)
#define REAPER_TRACE_SCOPE(category, name) \
    ::Sick::Core::Trace::Span REAPER_DETAIL_JOIN(_reaperTraceSpan_, __LINE__){category, name, {}, std::source_location::current()}
#define REAPER_WATCH_SCOPE(category, name, budget) \
    ::Sick::Core::Trace::Span REAPER_DETAIL_JOIN(_reaperWatchSpan_, __LINE__){category, name, std::chrono::duration_cast<std::chrono::milliseconds>(budget), std::source_location::current()}
