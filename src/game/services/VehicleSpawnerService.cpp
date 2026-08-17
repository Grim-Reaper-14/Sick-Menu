#include "VehicleSpawnerService.hpp"

#include "game/natives/Natives.hpp"

namespace Sick::Game
{
    bool VehicleSpawnerService::IsVehicleModel(Hash modelHash) const noexcept
    {
        return Natives::STREAMING::IS_MODEL_IN_CDIMAGE(modelHash) &&
            Natives::STREAMING::IS_MODEL_A_VEHICLE(modelHash);
    }

    void VehicleSpawnerService::RequestModel(Hash modelHash) const noexcept
    {
        Natives::STREAMING::REQUEST_MODEL(modelHash);
    }

    bool VehicleSpawnerService::IsModelLoaded(Hash modelHash) const noexcept
    {
        return Natives::STREAMING::HAS_MODEL_LOADED(modelHash);
    }

    void VehicleSpawnerService::ReleaseModel(Hash modelHash) const noexcept
    {
        Natives::STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
    }

    Vehicle VehicleSpawnerService::SpawnAtPlayer(Hash modelHash, bool enterVehicle) const noexcept
    {
        const auto ped = Natives::PLAYER::PLAYER_PED_ID();
        if (ped == 0 || !Natives::ENTITY::DOES_ENTITY_EXIST(ped))
            return 0;

        const auto position = Natives::ENTITY::GET_ENTITY_COORDS(ped, false);
        const auto heading = Natives::ENTITY::GET_ENTITY_HEADING(ped);
        const auto vehicle = Natives::VEHICLE::CREATE_VEHICLE(
            modelHash,
            position.x,
            position.y,
            position.z,
            heading,
            true,
            false);

        if (vehicle != 0 && enterVehicle)
            Natives::PED::SET_PED_INTO_VEHICLE(ped, vehicle, -1);
        return vehicle;
    }
}
