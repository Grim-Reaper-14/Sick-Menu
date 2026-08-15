#pragma once

#include "game/enhanced/BuildManager.hpp"
#include "game/enhanced/NativeBootstrap.hpp"
#include "game/enhanced/NativeTable.hpp"
#include "NativeDiagnostics.hpp"
#include "NativeRegistry.hpp"

namespace Sick::Game::Natives
{
    class NativeSystem final
    {
    public:
        static bool Initialize(
            Enhanced::BuildId build,
            Enhanced::NativeTable::LookupFn lookup,
            Enhanced::NativeTable::HashMapperFn mapper = nullptr) noexcept;

        static bool InitializeIndexed(
            Enhanced::BuildId build,
            Enhanced::NativeBootstrap::ProviderFn provider,
            Enhanced::NativeBootstrap::HashMapperFn mapper = nullptr) noexcept;

        static void Shutdown() noexcept;

        [[nodiscard]] static bool Ready() noexcept;
        [[nodiscard]] static NativeStats Stats() noexcept;
    };
}
