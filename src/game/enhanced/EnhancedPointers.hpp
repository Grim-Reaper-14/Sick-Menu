#pragma once

#include "core/memory/MemoryManager.hpp"

#include <cstddef>
#include <string_view>

namespace Sick::Game::Enhanced
{
    class EnhancedPointers final
    {
    public:
        inline static constexpr std::string_view ModuleName = "GTA5_Enhanced.exe";

        static bool RegisterPatterns(Memory::MemoryManager& memory);
        static bool Initialize();

        [[nodiscard]] static bool CoreReady(const Memory::MemoryManager& memory) noexcept;
        [[nodiscard]] static bool RendererReady(const Memory::MemoryManager& memory) noexcept;
        [[nodiscard]] static bool EntityBridgeReady(const Memory::MemoryManager& memory) noexcept;

        [[nodiscard]] static void* ResolveScriptGlobal(std::size_t index) noexcept;
        [[nodiscard]] static void* ResolveScriptGlobal(
            const Memory::MemoryManager& memory,
            std::size_t index) noexcept;

    private:
        static bool BindScriptGlobalResolver() noexcept;
    };
}
