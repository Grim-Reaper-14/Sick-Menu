#include "NativeResolver.hpp"

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
