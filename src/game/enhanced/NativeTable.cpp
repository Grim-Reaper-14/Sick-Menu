#include "NativeTable.hpp"

namespace Sick::Game::Enhanced
{
    NativeTable& NativeTable::Get() noexcept
    {
        static NativeTable table;
        return table;
    }

    void NativeTable::Configure(LookupFn lookup, HashMapperFn mapper) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Lookup = lookup;
        m_Mapper = mapper;
    }

    bool NativeTable::Ready() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Lookup != nullptr && BuildManager::Supported();
    }

    Natives::NativeHandler NativeTable::Resolve(NativeHash hash) const noexcept
    {
        std::scoped_lock lock(m_Mutex);

        if (!m_Lookup || !BuildManager::Supported())
            return nullptr;

        const auto build = BuildManager::Current();
        const auto mappedHash = m_Mapper ? m_Mapper(hash, build) : hash;
        return m_Lookup(mappedHash, build);
    }

    void NativeTable::Reset() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Lookup = nullptr;
        m_Mapper = nullptr;
    }
}
