#include "Trace.hpp"
#include "Metrics.hpp"
#include "core/logging/LogContext.hpp"
#include "core/logging/Logger.hpp"

#include <atomic>

namespace
{
    std::atomic<std::uint64_t> g_NextSpanId{1};
}

namespace Sick::Core::Trace
{
    Span::Span(
        std::string_view category,
        std::string_view name,
        std::chrono::milliseconds budget,
        std::source_location source) :
        m_Category(category),
        m_Name(name),
        m_Budget(budget),
        m_Started(std::chrono::steady_clock::now()),
        m_Source(source),
        m_Id(g_NextSpanId.fetch_add(1, std::memory_order_relaxed)),
        m_PreviousSpan(Logging::CurrentSpan())
    {
        Logging::SetSpan(m_Id);
    }

    Span::~Span() noexcept
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_Started;
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        const auto milliseconds = static_cast<double>(microseconds) / 1000.0;
        Logging::SetSpan(m_PreviousSpan);

        try
        {
            Metrics::Observe("trace." + m_Category + "." + m_Name + ".ms", milliseconds);

            auto fields = std::move(m_Fields);
            fields.push_back(Logging::MakeField("elapsed_ms", milliseconds));
            fields.push_back(Logging::MakeField("span_id", m_Id));

            if (m_Budget.count() > 0 && elapsed > m_Budget)
            {
                fields.push_back(Logging::MakeField("budget_ms", m_Budget.count()));
                Logging::Logger::Get().Submit(
                    Logging::Level::Warn,
                    m_Category,
                    Logging::Detail::Format("{} exceeded its execution budget", m_Name),
                    Logging::EventId::TraceBudgetExceeded,
                    std::move(fields),
                    m_Source);
            }
            else
            {
                Logging::Logger::Get().Submit(
                    Logging::Level::Trace,
                    m_Category,
                    Logging::Detail::Format("{} completed", m_Name),
                    Logging::EventId::None,
                    std::move(fields),
                    m_Source);
            }
        }
        catch (...)
        {
        }
    }
}
