#include "NativeBootstrap.hpp"
#include "NativeCrossmap.hpp"
#include "game/natives/NativeHandlerTable.hpp"
#include "game/natives/generated/NativeHashes.hpp"

namespace Sick::Game::Enhanced
{
    NativeBootstrap& NativeBootstrap::Get() noexcept
    {
        static NativeBootstrap bootstrap;
        return bootstrap;
    }

    bool NativeBootstrap::Initialize(ProviderFn provider, HashMapperFn mapper) noexcept
    {
        Shutdown();

        const auto build = BuildManager::Current();
        if (!provider || build == UnknownBuild)
            return false;

        auto& handlers = Natives::NativeHandlerTable::Get();
        std::size_t resolved{};

        for (std::size_t i = 0; i < Natives::NativeCount; ++i)
        {
            const auto index = static_cast<Natives::NativeIndex>(i);
            const auto originalHash = Natives::Generated::HashFor(index);
            const auto mappedHash = mapper
                ? mapper(originalHash, build)
                : NativeCrossmap::Get().Translate(originalHash, build);
            const auto handler = provider(mappedHash, build);

            handlers.Set(index, handler);
            if (handler)
                ++resolved;
        }

        {
            std::scoped_lock lock(m_Mutex);
            m_ResolvedCount = resolved;
            m_Ready = resolved != 0;
        }

        return resolved != 0;
    }

    void NativeBootstrap::Shutdown() noexcept
    {
        Natives::NativeHandlerTable::Get().Clear();

        std::scoped_lock lock(m_Mutex);
        m_Ready = false;
        m_ResolvedCount = 0;
    }

    bool NativeBootstrap::Ready() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Ready;
    }

    std::size_t NativeBootstrap::ResolvedCount() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_ResolvedCount;
    }

    std::size_t NativeBootstrap::MissingCount() const noexcept
    {
        const auto resolved = ResolvedCount();
        return resolved < Natives::NativeCount ? Natives::NativeCount - resolved : 0;
    }

    Natives::NativeHandler NativeBootstrap::Resolve(NativeHash hash) const noexcept
    {
        auto index = Natives::Generated::IndexForHash(hash);
        if (index == Natives::NativeIndex::Count)
        {
            const auto original = NativeCrossmap::Get().Reverse(hash, BuildManager::Current());
            if (!original)
                return nullptr;

            index = Natives::Generated::IndexForHash(*original);
            if (index == Natives::NativeIndex::Count)
                return nullptr;
        }

        return Natives::NativeHandlerTable::Get().Get(index);
    }
}
