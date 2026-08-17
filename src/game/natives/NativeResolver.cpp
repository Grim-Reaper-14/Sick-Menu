#include "NativeResolver.hpp"

namespace
{
    Sick::Game::NativeHash ToCanonical(Sick::Game::NativeHash hash) noexcept
    {
        switch (hash)
        {
        case 0x1F2AA07F00B3217AULL: return 0xB5AD06DDA85E2E8FULL;
        case 0xE38E9162A2500646ULL: return 0x5B59C12A02157D00ULL;
        case 0x6AF0636DDEDCB6DDULL: return 0x8450270DC5896D39ULL;
        case 0x2A1F4F37F95BAD08ULL: return 0xF5501FF9869DAC7CULL;
        case 0x487EB21CC7295BA1ULL: return 0xE33678A9AE50A01BULL;
        case 0x43FEB945EE7F85B8ULL: return 0xA5277ECCD081FCC1ULL;
        case 0x816562BADFDEC83EULL: return 0x941B1F179D6AE19AULL;
        case 0xD133EF7430EDCD09ULL: return 0x4F1D4BE3A7F24601ULL;
        case 0x2036F561ADD12E33ULL: return 0xBB361D7264AC4FD8ULL;
        case 0xF40DD601A65F7F19ULL: return 0xC0C8E6AAA00F1A58ULL;
        case 0x6089CDF6A57F326CULL: return 0x77B012A683295B6EULL;
        case 0x2AA720E4287BF269ULL: return 0xE62930EC6FAABCA5ULL;
        case 0x8E0A582209A62695ULL: return 0xEAB8A43F6621850FULL;
        case 0xB5BA80F839791C0FULL: return 0x5DA0536AEAD1FF31ULL;
        case 0xE41033B25D003A07ULL: return 0x89D1FDCA3735A1E0ULL;
        case 0x7EE3A3C5E4A40CC9ULL: return 0xD772F6AA66750D2BULL;
        case 0x1262D55792428154ULL: return 0x579FA5568DE0C2A0ULL;
        case 0xEB9DC3C7D8596C46ULL: return 0x439C904840715871ULL;
        default: return 0;
        }
    }
}

namespace Sick::Game::Natives
{
    NativeResolver& NativeResolver::Get() noexcept
    {
        static NativeResolver instance;
        return instance;
    }

    void NativeResolver::SetResolver(ResolverFn resolver) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Resolver = resolver;
        m_Cache.clear();
    }

    NativeResolver::ResolverFn NativeResolver::GetResolver() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Resolver;
    }

    bool NativeResolver::IsReady() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Resolver != nullptr || !m_Overrides.empty();
    }

    NativeHandler NativeResolver::Resolve(NativeHash hash) noexcept
    {
        std::scoped_lock lock(m_Mutex);

        if (const auto overrideIt = m_Overrides.find(hash); overrideIt != m_Overrides.end())
            return overrideIt->second;

        if (const auto cachedIt = m_Cache.find(hash); cachedIt != m_Cache.end())
            return cachedIt->second;

        NativeHandler handler = m_Resolver ? m_Resolver(hash) : nullptr;
        if (!handler)
        {
            const auto canonical = ToCanonical(hash);
            if (canonical != 0 && m_Resolver)
                handler = m_Resolver(canonical);
        }

        m_Cache.emplace(hash, handler);
        return handler;
    }

    void NativeResolver::RegisterOverride(NativeHash hash, NativeHandler handler) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Overrides[hash] = handler;
        m_Cache.erase(hash);
    }

    void NativeResolver::RemoveOverride(NativeHash hash) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Overrides.erase(hash);
        m_Cache.erase(hash);
    }

    void NativeResolver::ClearCache() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Cache.clear();
    }

    void NativeResolver::Reset() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Resolver = nullptr;
        m_Cache.clear();
        m_Overrides.clear();
    }

    std::size_t NativeResolver::CachedHandlerCount() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Cache.size();
    }
}
