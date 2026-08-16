#pragma once

#include "BuildManager.hpp"
#include "game/natives/NativeTypes.hpp"

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <span>
#include <unordered_map>

namespace Sick::Game::Enhanced
{
    struct NativeCrossmapEntry
    {
        NativeHash original{};
        NativeHash mapped{};
    };

    class NativeCrossmap final
    {
    public:
        static NativeCrossmap& Get() noexcept;

        [[nodiscard]] bool Register(BuildId build, NativeHash original, NativeHash mapped);
        [[nodiscard]] bool Load(BuildId build, std::span<const NativeCrossmapEntry> entries);
        [[nodiscard]] bool Remove(BuildId build, NativeHash original) noexcept;

        void Clear() noexcept;
        void Clear(BuildId build) noexcept;

        [[nodiscard]] NativeHash Translate(NativeHash original, BuildId build) const noexcept;
        [[nodiscard]] std::optional<NativeHash> Reverse(NativeHash mapped, BuildId build) const noexcept;
        [[nodiscard]] bool Contains(BuildId build, NativeHash original) const noexcept;
        [[nodiscard]] std::size_t Size(BuildId build) const noexcept;

    private:
        struct BuildCrossmap
        {
            std::unordered_map<NativeHash, NativeHash> forward;
            std::unordered_map<NativeHash, NativeHash> reverse;
        };

        NativeCrossmap() = default;

        mutable std::shared_mutex m_Mutex;
        std::unordered_map<BuildId, BuildCrossmap> m_Builds;
    };
}
