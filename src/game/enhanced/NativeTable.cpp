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
        LookupFn lookup{};
        HashMapperFn mapper{};

        {
            std::scoped_lock lock(m_Mutex);
            lookup = m_Lookup;
            mapper = m_Mapper;
        }

        const auto build = BuildManager::Current();
        if (!lookup || build == UnknownBuild)
            return nullptr;

        const auto mappedHash = mapper ? mapper(hash, build) : hash;
        return lookup(mappedHash, build);
    }

    void NativeTable::Reset() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Lookup = nullptr;
        m_Mapper = nullptr;
    }
}
