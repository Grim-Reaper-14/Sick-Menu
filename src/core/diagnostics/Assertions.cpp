#include "Assertions.hpp"
#include "StackTrace.hpp"

namespace Sick::Core::Diagnostics
{
    void ReportAssertion(
        std::string_view expression,
        std::string_view category,
        std::string message,
        std::source_location source) noexcept
    {
        try
        {
            const auto stack = CaptureStack(1, 32);
            Logging::Logger::Get().Submit(
                Logging::Level::Critical,
                category,
                std::move(message),
                Logging::EventId::AssertionFailed,
                {
                    Logging::MakeField("expression", expression),
                    Logging::MakeField("stack", FormatStack(stack))
                },
                source);
        }
        catch (...)
        {
        }
    }
}
