#pragma once

#include "game/services/VehicleService.hpp"

namespace Sick::Backend::Features::VehicleFeature
{
    struct NoGravity final
    {
        static void Apply(Game::VehicleService& service, Game::Vehicle vehicle, bool enabled) noexcept
        {
            service.SetGravity(vehicle, !enabled);
        }
    };
}
