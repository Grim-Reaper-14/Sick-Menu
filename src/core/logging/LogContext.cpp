#include "LogContext.hpp"

namespace
{
    struct ThreadContext
    {
        std::uint64_t correlationId{};
        std::uint64_t spanId{};
        std::string threadName;
        std::vector<Sick::Core::Logging::Field> fields;
    };

    thread_local ThreadContext g_Context;
}

namespace Sick::Core::Logging
{
    void SetThreadName(std::string name) noexcept
    {
        try
        {
            g_Context.threadName = std::move(name);
        }
        catch (...)
        {
        }
    }

    ContextSnapshot CaptureContext()
    {
        return {
            g_Context.correlationId,
            g_Context.spanId,
            g_Context.threadName,
            g_Context.fields
        };
    }

    std::uint64_t CurrentCorrelation() noexcept
    {
        return g_Context.correlationId;
    }

    std::uint64_t CurrentSpan() noexcept
    {
        return g_Context.spanId;
    }

    void SetCorrelation(std::uint64_t value) noexcept
    {
        g_Context.correlationId = value;
    }

    void SetSpan(std::uint64_t value) noexcept
    {
        g_Context.spanId = value;
    }

    ScopedCorrelation::ScopedCorrelation(std::uint64_t value) noexcept :
        m_Previous(g_Context.correlationId)
    {
        g_Context.correlationId = value;
    }

    ScopedCorrelation::~ScopedCorrelation()
    {
        g_Context.correlationId = m_Previous;
    }

    ScopedContext::ScopedContext(std::initializer_list<Field> fields) :
        m_PreviousSize(g_Context.fields.size())
    {
        g_Context.fields.insert(g_Context.fields.end(), fields.begin(), fields.end());
    }

    ScopedContext::~ScopedContext()
    {
        g_Context.fields.resize(m_PreviousSize);
    }
}
