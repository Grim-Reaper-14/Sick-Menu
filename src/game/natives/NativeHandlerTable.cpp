#include "NativeHandlerTable.hpp"

namespace Sick::Game::Natives
{
    NativeHandlerTable::NativeHandlerTable() noexcept
    {
        Clear();
    }

    NativeHandlerTable& NativeHandlerTable::Get() noexcept
    {
        static NativeHandlerTable table;
        return table;
    }

    void NativeHandlerTable::Set(NativeIndex index, NativeHandler handler) noexcept
    {
        const auto offset = ToNativeOffset(index);
        if (offset >= m_Handlers.size())
            return;

        m_Handlers[offset].store(handler, std::memory_order_release);
    }

    NativeHandler NativeHandlerTable::Get(NativeIndex index) const noexcept
    {
        const auto offset = ToNativeOffset(index);
        if (offset >= m_Handlers.size())
            return nullptr;

        return m_Handlers[offset].load(std::memory_order_acquire);
    }

    void NativeHandlerTable::Clear() noexcept
    {
        for (auto& handler : m_Handlers)
            handler.store(nullptr, std::memory_order_relaxed);
    }

    bool NativeHandlerTable::Ready() const noexcept
    {
        return ResolvedCount() != 0;
    }

    std::size_t NativeHandlerTable::ResolvedCount() const noexcept
    {
        std::size_t resolved{};
        for (const auto& handler : m_Handlers)
        {
            if (handler.load(std::memory_order_relaxed))
                ++resolved;
        }

        return resolved;
    }
}
