#pragma once

#include "Module.hpp"
#include "Pattern.hpp"
#include "PointerCalculator.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Sick::Memory
{
    struct ScanDiagnostic
    {
        std::string pattern;
        std::string module;
        bool required{};
        bool requireUnique{true};
        bool found{};
        bool ambiguous{};
        std::size_t matchCount{};
        std::uintptr_t address{};
    };

    struct ScanSummary
    {
        bool success{true};
        std::vector<ScanDiagnostic> diagnostics;
    };

    class PatternScanner final
    {
    public:
        using MatchCallback = std::function<void(PointerCalculator)>;

        explicit PatternScanner(Module module);

        void Add(
            Pattern pattern,
            MatchCallback callback,
            bool required = true,
            bool requireUnique = true);

        [[nodiscard]] ScanSummary Scan() const;
        [[nodiscard]] std::optional<PointerCalculator> FindFirst(const Pattern& pattern) const noexcept;
        [[nodiscard]] std::vector<PointerCalculator> FindAll(const Pattern& pattern) const;

    private:
        struct Request
        {
            Pattern pattern;
            MatchCallback callback;
            bool required{};
            bool requireUnique{true};
        };

        Module m_Module;
        std::vector<Request> m_Requests;
    };
}
