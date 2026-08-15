#pragma once

#include "NativeContext.hpp"
#include "generated/NativeIndex.hpp"

#include <array>
#include <atomic>
#include <cstddef>

namespace Sick::Game::Natives
{
    class NativeHandlerTable final
    {
    public:
        static NativeHandlerTable& Get() noexcept;

        void Set(NativeIndex index, NativeHandler handler) noexcept;
        [[nodiscard]] NativeHandler Get(NativeIndex index) const noexcept;

        void Clear() noexcept;
        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] std::size_t ResolvedCount() const noexcept;

    private:
        NativeHandlerTable() noexcept;

        std::array<std::atomic<NativeHandler>, NativeCount> m_Handlers{};
    };
}
