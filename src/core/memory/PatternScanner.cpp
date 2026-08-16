#include "PatternScanner.hpp"

#include <utility>

namespace Sick::Memory
{
    PatternScanner::PatternScanner(Module module) :
        m_Module(std::move(module))
    {
    }

    void PatternScanner::Add(
        Pattern pattern,
        MatchCallback callback,
        bool required,
        bool requireUnique)
    {
        if (pattern.Empty() || !callback)
            return;

        m_Requests.push_back({
            std::move(pattern),
            std::move(callback),
            required,
            requireUnique});
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
            const auto matches = FindAll(request.pattern);
            const bool found = !matches.empty();
            const bool ambiguous = matches.size() > 1;
            const bool accepted = found && (!request.requireUnique || matches.size() == 1);

            ScanDiagnostic diagnostic;
            diagnostic.pattern = request.pattern.Name();
            diagnostic.module = m_Module.Name();
            diagnostic.required = request.required;
            diagnostic.requireUnique = request.requireUnique;
            diagnostic.found = found;
            diagnostic.ambiguous = ambiguous;
            diagnostic.matchCount = matches.size();
            diagnostic.address = found ? matches.front().Address() : 0;
            summary.diagnostics.push_back(std::move(diagnostic));

            if (!accepted)
            {
                if (request.required)
                    summary.success = false;
                continue;
            }

            request.callback(matches.front());
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

    std::vector<PointerCalculator> PatternScanner::FindAll(const Pattern& pattern) const
    {
        std::vector<PointerCalculator> matches;

        if (!m_Module.Valid() || pattern.Empty() || pattern.Size() > m_Module.Size())
            return matches;

        const auto image = m_Module.Bytes();
        const auto lastOffset = image.size() - pattern.Size();

        for (std::size_t offset = 0; offset <= lastOffset; ++offset)
        {
            if (pattern.Matches(image, offset))
                matches.emplace_back(m_Module.Base() + offset);
        }

        return matches;
    }
}
