#include "PatternScanner.hpp"

#include <utility>

namespace Sick::Memory
{
    PatternScanner::PatternScanner(Module module) :
        m_Module(std::move(module))
    {
    }

    void PatternScanner::Add(Pattern pattern, MatchCallback callback, bool required)
    {
        if (pattern.Empty() || !callback)
            return;

        m_Requests.push_back({std::move(pattern), std::move(callback), required});
    }

    ScanSummary PatternScanner::Scan() const
    {
        ScanSummary summary;

        if (!m_Module.Valid())
        {
            summary.success = false;
            return summary;
        }

        summary.diagnostics.reserve(m_Requests.size());

        for (const auto& request : m_Requests)
        {
            const auto match = FindFirst(request.pattern);
            const bool found = match.has_value();

            summary.diagnostics.push_back({
                request.pattern.Name(),
                m_Module.Name(),
                request.required,
                found,
                found ? match->Address() : 0});

            if (!found)
            {
                if (request.required)
                    summary.success = false;
                continue;
            }

            request.callback(*match);
        }

        return summary;
    }

    std::optional<PointerCalculator> PatternScanner::FindFirst(const Pattern& pattern) const noexcept
    {
        if (!m_Module.Valid() || pattern.Empty() || pattern.Size() > m_Module.Size())
            return std::nullopt;

        const auto image = m_Module.Bytes();
        const auto lastOffset = image.size() - pattern.Size();

        for (std::size_t offset = 0; offset <= lastOffset; ++offset)
        {
            if (pattern.Matches(image, offset))
                return PointerCalculator{m_Module.Base() + offset};
        }

        return std::nullopt;
    }
}
