#pragma once

#include "game/services/VehicleService.hpp"

namespace Sick::Backend::Features::VehicleFeature
{
    struct PutOnGround final
    {
        [[nodiscard]] static bool Apply(Game::VehicleService& service, Game::Vehicle vehicle) noexcept
        {
            return service.PutOnGround(vehicle);
        }
    };
}
