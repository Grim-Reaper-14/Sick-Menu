#pragma once

#include "BuildManager.hpp"
#include "NativeTable.hpp"

namespace Sick::Game::Enhanced
{
    class EnhancedGame final
    {
    public:
        static bool Initialize(
            BuildId build,
            NativeTable::LookupFn lookup,
            NativeTable::HashMapperFn mapper = nullptr) noexcept;

        static void Shutdown() noexcept;
        static void Tick();
        [[nodiscard]] static bool Ready() noexcept;
    };
}
