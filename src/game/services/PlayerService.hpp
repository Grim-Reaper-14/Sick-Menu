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
        [[nodiscard]] bool Exists(Ped ped) const noexcept;
        void SetInvincible(Ped ped, bool enabled) const noexcept;
    };
}
