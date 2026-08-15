#pragma once

#include "game/natives/NativeTypes.hpp"

namespace Sick::Game
{
    class PlayerService final
    {
    public:
        [[nodiscard]] Ped LocalPed() const noexcept;
        [[nodiscard]] bool Exists() const noexcept;
        void SetInvincible(bool enabled) const noexcept;
    };
}
