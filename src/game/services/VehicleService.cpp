#include "VehicleService.hpp"

#include "game/natives/Natives.hpp"

namespace Sick::Game
{
    Vehicle VehicleService::CurrentVehicle() const noexcept
    {
        const auto ped = Natives::PLAYER::PLAYER_PED_ID();
        return ped != 0 ? Natives::PED::GET_VEHICLE_PED_IS_IN(ped, false) : 0;
    }

    bool VehicleService::Exists(Vehicle vehicle) const noexcept
    {
        return vehicle != 0 && Natives::ENTITY::DOES_ENTITY_EXIST(vehicle);
    }

    void VehicleService::SetInvincible(Vehicle vehicle, bool enabled) const noexcept
    {
        Natives::ENTITY::SET_ENTITY_INVINCIBLE(vehicle, enabled, false);
    }

    void VehicleService::SetGravity(Vehicle vehicle, bool enabled) const noexcept
    {
        Natives::ENTITY::SET_ENTITY_HAS_GRAVITY(vehicle, enabled);
    }

    void VehicleService::SetCollision(Vehicle vehicle, bool enabled) const noexcept
    {
        Natives::ENTITY::SET_ENTITY_COLLISION(vehicle, enabled, true);
    }

    void VehicleService::SetEngineOn(Vehicle vehicle) const noexcept
    {
        Natives::VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, true, false);
    }

    void VehicleService::Repair(Vehicle vehicle) const noexcept
    {
        constexpr float FullHealth = 1000.0F;
        Natives::VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, FullHealth);
        Natives::VEHICLE::SET_VEHICLE_BODY_HEALTH(vehicle, FullHealth);
        Natives::VEHICLE::SET_VEHICLE_PETROL_TANK_HEALTH(vehicle, FullHealth);
        Natives::VEHICLE::SET_VEHICLE_FIXED(vehicle);
        Natives::VEHICLE::SET_VEHICLE_DEFORMATION_FIXED(vehicle);
    }

    void VehicleService::Clean(Vehicle vehicle) const noexcept
    {
        Natives::VEHICLE::SET_VEHICLE_DIRT_LEVEL(vehicle, 0.0F);
    }

    bool VehicleService::PutOnGround(Vehicle vehicle) const noexcept
    {
        return Natives::VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(vehicle, 5.0F);
    }
}
