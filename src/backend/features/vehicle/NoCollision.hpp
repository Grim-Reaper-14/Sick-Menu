#pragma once

#include "game/services/VehicleService.hpp"

namespace Sick::Backend::Features::VehicleFeature
{
    struct NoCollision final
    {
        static void Apply(Game::VehicleService& service, Game::Vehicle vehicle, bool enabled) noexcept
        {
            service.SetCollision(vehicle, !enabled);
        }
    };
}
