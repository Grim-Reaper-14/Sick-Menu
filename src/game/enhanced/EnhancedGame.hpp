#pragma once

#include "BuildManager.hpp"
#include "NativeBootstrap.hpp"
#include "NativeTable.hpp"
#include "ScriptGlobal.hpp"

namespace Sick::Game::Enhanced
{
    class EnhancedGame final
    {
    public:
        static bool Initialize(
            BuildId build,
            NativeTable::LookupFn lookup,
            NativeTable::HashMapperFn mapper = nullptr) noexcept;

        static bool InitializeIndexed(
            BuildId build,
            NativeBootstrap::ProviderFn provider,
            NativeBootstrap::HashMapperFn mapper = nullptr) noexcept;

        static void BindScriptGlobalResolver(ScriptGlobal::ResolverFn resolver) noexcept;
        static void Shutdown() noexcept;
        static void Tick();
        [[nodiscard]] static bool Ready() noexcept;
    };
}
