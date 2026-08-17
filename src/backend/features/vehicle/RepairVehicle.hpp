#pragma once

#include "game/services/VehicleService.hpp"

namespace Sick::Backend::Features::VehicleFeature
{
    struct RepairVehicle final
    {
        static void Apply(Game::VehicleService& service, Game::Vehicle vehicle) noexcept
        {
            service.Repair(vehicle);
        }
    };
}
