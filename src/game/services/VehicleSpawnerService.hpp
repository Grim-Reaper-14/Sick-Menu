#pragma once

#include "game/natives/NativeTypes.hpp"

namespace Sick::Game
{
    class VehicleSpawnerService final
    {
    public:
        [[nodiscard]] bool IsVehicleModel(Hash modelHash) const noexcept;
        void RequestModel(Hash modelHash) const noexcept;
        [[nodiscard]] bool IsModelLoaded(Hash modelHash) const noexcept;
        void ReleaseModel(Hash modelHash) const noexcept;
        [[nodiscard]] Vehicle SpawnAtPlayer(Hash modelHash, bool enterVehicle) const noexcept;
    };
}
