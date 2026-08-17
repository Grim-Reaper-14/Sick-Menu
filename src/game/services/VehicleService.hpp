#pragma once

#include "game/natives/NativeTypes.hpp"

namespace Sick::Game
{
    // Stateless vehicle operations. Persistent state belongs to VehicleFeatures.
    class VehicleService final
    {
    public:
        [[nodiscard]] Vehicle CurrentVehicle() const noexcept;
        [[nodiscard]] bool Exists(Vehicle vehicle) const noexcept;

        void SetInvincible(Vehicle vehicle, bool enabled) const noexcept;
        void SetGravity(Vehicle vehicle, bool enabled) const noexcept;
        void SetCollision(Vehicle vehicle, bool enabled) const noexcept;
        void SetEngineOn(Vehicle vehicle) const noexcept;
        void Repair(Vehicle vehicle) const noexcept;
        void Clean(Vehicle vehicle) const noexcept;
        [[nodiscard]] bool PutOnGround(Vehicle vehicle) const noexcept;
    };
}
