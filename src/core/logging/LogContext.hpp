#pragma once

#include "LogRecord.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace Sick::Core::Logging
{
    struct ContextSnapshot
    {
        std::uint64_t correlationId{};
        std::uint64_t spanId{};
        std::string threadName;
        std::vector<Field> fields;
    };

    void SetThreadName(std::string name) noexcept;
    [[nodiscard]] ContextSnapshot CaptureContext();
    [[nodiscard]] std::uint64_t CurrentCorrelation() noexcept;
    [[nodiscard]] std::uint64_t CurrentSpan() noexcept;
    void SetCorrelation(std::uint64_t value) noexcept;
    void SetSpan(std::uint64_t value) noexcept;

    class ScopedCorrelation final
    {
    public:
        explicit ScopedCorrelation(std::uint64_t value) noexcept;
        ~ScopedCorrelation();

        ScopedCorrelation(const ScopedCorrelation&) = delete;
        ScopedCorrelation& operator=(const ScopedCorrelation&) = delete;

    private:
        std::uint64_t m_Previous{};
    };

    class ScopedContext final
    {
    public:
        explicit ScopedContext(std::initializer_list<Field> fields);
        ~ScopedContext();

        ScopedContext(const ScopedContext&) = delete;
        ScopedContext& operator=(const ScopedContext&) = delete;

    private:
        std::size_t m_PreviousSize{};
    };
}
