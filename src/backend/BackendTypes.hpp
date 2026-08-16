#pragma once

#include <cstddef>
#include <cstdint>

namespace Sick::Backend
{
    struct BackendSnapshot
    {
        bool initialized{};
        bool nativeReady{};
        bool scriptReady{};
        bool godModeRequested{};
        bool godModeActive{};
        std::size_t pendingGameCalls{};
        std::size_t pendingFibers{};
        std::uint64_t lastTickMicros{};
        std::uint64_t maxTickMicros{};
        std::uint64_t overBudgetTicks{};
    };
}
