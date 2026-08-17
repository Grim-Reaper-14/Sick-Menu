#pragma once

#include "game/natives/NativeTypes.hpp"

namespace Sick::Game
{
    // Stateless low-level player operations. Feature lifetime and desired state
    // belong to the backend feature layer, not to this service.
    class PlayerService final
    {
    public:
        [[nodiscard]] Ped LocalPed() const noexcept;
        [[nodiscard]] Player LocalPlayer() const noexcept;
        [[nodiscard]] bool Exists(Ped ped) const noexcept;

        void SetInvincible(Ped ped, bool enabled) const noexcept;
        void SetMaxUnderwaterTime(Ped ped, float seconds) const noexcept;
        void SetCanRagdoll(Ped ped, bool enabled) const noexcept;
        void SetSuperJump(Player player) const noexcept;
        void SetSeatBelt(Ped ped, bool enabled) const noexcept;
        [[nodiscard]] int WantedLevel(Player player) const noexcept;
        void SetWantedLevel(Player player, int level) const noexcept;
        void ClearWantedLevel(Player player) const noexcept;
        void SetRunMultiplier(Player player, float multiplier) const noexcept;
        void SetSwimMultiplier(Player player, float multiplier) const noexcept;
        void Clean(Ped ped) const noexcept;
        void SetAqualung(Ped ped, bool enabled) const noexcept;
        void SetGravity(Ped ped, bool enabled) const noexcept;
        void SetWaterproof(Ped ped, bool enabled) const noexcept;
    };
}
