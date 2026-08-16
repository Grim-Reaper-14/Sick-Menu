#pragma once

#include "game/services/PlayerService.hpp"

#include <atomic>
#include <cstdint>

namespace Sick::Backend::Features
{
    struct PlayerFeatureSnapshot
    {
        bool godModeRequested{};
        bool godModeActive{};
    };

    class PlayerFeatures final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void Tick() noexcept;
        void Reset() noexcept;
        [[nodiscard]] PlayerFeatureSnapshot Snapshot() const noexcept;

    private:
        static constexpr std::uint32_t GodModeRefreshTicks = 30;

        Game::PlayerService m_Player;
        std::atomic_bool m_GodModeRequested{};
        std::atomic_bool m_GodModeActive{};
        std::atomic_uint64_t m_GodModeRevision{};
        std::uint64_t m_SeenGodModeRevision{};
        std::uint32_t m_GodModeRefreshTicks{};
        Game::Ped m_LastGodModePed{};
    };
}
