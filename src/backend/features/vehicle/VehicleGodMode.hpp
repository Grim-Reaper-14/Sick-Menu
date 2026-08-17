#pragma once

#include "game/services/VehicleService.hpp"

namespace Sick::Backend::Features::VehicleFeature
{
    struct VehicleGodMode final
    {
        static void Apply(Game::VehicleService& service, Game::Vehicle vehicle, bool enabled) noexcept
        {
            service.SetInvincible(vehicle, enabled);
        }
    };
}
