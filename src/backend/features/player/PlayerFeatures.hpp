#pragma once

#include "backend/BackendTypes.hpp"
#include "game/services/PlayerService.hpp"

#include <atomic>
#include <cstdint>

namespace Sick::Backend::Features
{
    // Coordinates Player feature modules that share state or have interactions.
    // The individual gameplay operations live beside this file (GodMode.hpp,
    // Waterproof.hpp, etc.) so a menu entry does not grow this class forever.
    class PlayerFeatures final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void SetInfiniteOxygen(bool enabled) noexcept;
        void SetNoRagdoll(bool enabled) noexcept;
        void SetSuperJump(bool enabled) noexcept;
        void SetSeatBelt(bool enabled) noexcept;
        void SetNoWantedLevel(bool enabled) noexcept;
        void SetWantedLevel(int level) noexcept;
        void SetFastRun(bool enabled) noexcept;
        void SetFastSwim(bool enabled) noexcept;
        void SetKeepPlayerClean(bool enabled) noexcept;
        void SetAqualung(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetWaterproof(bool enabled) noexcept;

        void Tick() noexcept;
        void Reset() noexcept;
        [[nodiscard]] PlayerFeatureSnapshot Snapshot() const noexcept;

    private:
        static constexpr std::uint32_t RefreshTicks = 30;

        void RestorePedState(Game::Ped ped) noexcept;
        void ClearPedAppliedState() noexcept;

        Game::PlayerService m_Player;

        std::atomic_bool m_GodModeRequested{};
        std::atomic_bool m_InfiniteOxygenRequested{};
        std::atomic_bool m_NoRagdollRequested{};
        std::atomic_bool m_SuperJumpRequested{};
        std::atomic_bool m_SeatBeltRequested{};
        std::atomic_bool m_NoWantedLevelRequested{};
        std::atomic_int m_WantedLevelRequested{};
        std::atomic_bool m_FastRunRequested{};
        std::atomic_bool m_FastSwimRequested{};
        std::atomic_bool m_KeepPlayerCleanRequested{};
        std::atomic_bool m_AqualungRequested{};
        std::atomic_bool m_NoGravityRequested{};
        std::atomic_bool m_WaterproofRequested{};

        std::atomic_bool m_GodModeActive{};
        std::atomic_bool m_InfiniteOxygenActive{};
        std::atomic_bool m_NoRagdollActive{};
        std::atomic_bool m_SuperJumpActive{};
        std::atomic_bool m_SeatBeltActive{};
        std::atomic_bool m_NoWantedLevelActive{};
        std::atomic_bool m_FastRunActive{};
        std::atomic_bool m_FastSwimActive{};
        std::atomic_bool m_KeepPlayerCleanActive{};
        std::atomic_bool m_AqualungActive{};
        std::atomic_bool m_NoGravityActive{};
        std::atomic_bool m_WaterproofActive{};

        bool m_GodModeApplied{};
        bool m_OxygenApplied{};
        bool m_NoRagdollApplied{};
        bool m_SeatBeltApplied{};
        bool m_AqualungApplied{};
        bool m_NoGravityApplied{};
        bool m_WaterproofApplied{};
        bool m_NoWantedLevelApplied{};
        bool m_FastRunApplied{};
        bool m_FastSwimApplied{};
        int m_WantedLevelApplied{};
        std::uint32_t m_RefreshTicks{};
        Game::Ped m_LastPed{};
    };
}
