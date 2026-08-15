#include "ScriptGlobal.hpp"

namespace Sick::Game::Enhanced
{
    void ScriptGlobal::BindResolver(ResolverFn resolver) noexcept
    {
        s_Resolver.store(resolver, std::memory_order_release);
    }

    void ScriptGlobal::ResetResolver() noexcept
    {
        s_Resolver.store(nullptr, std::memory_order_release);
    }

    bool ScriptGlobal::ResolverReady() noexcept
    {
        return s_Resolver.load(std::memory_order_acquire) != nullptr;
    }

    bool ScriptGlobal::CanAccess() const noexcept
    {
        return Address() != nullptr;
    }

    void* ScriptGlobal::Address() const noexcept
    {
        const auto resolver = s_Resolver.load(std::memory_order_acquire);
        return resolver ? resolver(m_Index) : nullptr;
    }
}
