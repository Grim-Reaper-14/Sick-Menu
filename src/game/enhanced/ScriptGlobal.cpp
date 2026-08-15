#include "ScriptGlobal.hpp"
#include "core/diagnostics/Metrics.hpp"
#include "core/logging/Logger.hpp"
#include "core/logging/RateLimiter.hpp"

#include <chrono>

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
        void* address = resolver ? resolver(m_Index) : nullptr;
        if (address)
            return address;

        Core::Metrics::Increment("script_global.resolve_failures");

        try
        {
            const auto key = Core::Logging::Detail::Format("script_global.{}", m_Index);
            if (Core::Logging::RateLimiter::Global().Allow(key, std::chrono::seconds{1}))
            {
                Core::Logging::Logger::Get().Submit(
                    Core::Logging::Level::Warn,
                    "ScriptGlobal",
                    resolver
                        ? "Script global resolver returned a null address"
                        : "Script global resolver is not bound",
                    Core::Logging::EventId::ScriptGlobalResolutionFailure,
                    {
                        Core::Logging::MakeField("index", m_Index),
                        Core::Logging::MakeField("resolver_bound", resolver != nullptr)
                    });
            }
        }
        catch (...)
        {
        }

        return nullptr;
    }
}
