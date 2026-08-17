#pragma once

#include "backend/BackendTypes.hpp"
#include "backend/features/player/PlayerFeatures.hpp"
#include "backend/features/vehicle/VehicleFeatures.hpp"
#include "backend/features/vehicle/handling/HandlingFeatures.hpp"

namespace Sick::Backend::Features
{
    class FeatureManager final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void SetInfiniteOxygen(bool enabled) noexcept;
        void SetNoRagdoll(bool enabled) noexcept;
        void SetSuperJump(bool enabled) noexcept;
        void SetSeatBelt(bool enabled) noexcept;
        void SetNoWantedLevel(bool enabled) noexcept;
        void SetWantedLevel(int level) noexcept;
        void SetFastRun(bool enabled) noexcept;
        void SetFastSwim(bool enabled) noexcept;
        void SetKeepPlayerClean(bool enabled) noexcept;
        void SetAqualung(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetWaterproof(bool enabled) noexcept;

        void SetVehicleGodMode(bool enabled) noexcept;
        void SetVehicleAutoRepair(bool enabled) noexcept;
        void SetVehicleKeepClean(bool enabled) noexcept;
        void SetVehicleEngineAlwaysOn(bool enabled) noexcept;
        void SetVehicleNoGravity(bool enabled) noexcept;
        void SetVehicleNoCollision(bool enabled) noexcept;
        void RepairVehicle() noexcept;
        void CleanVehicle() noexcept;
        void PutVehicleOnGround() noexcept;

        void SetHandlingEditorActive(bool active) noexcept;
        void SetHandlingValue(Handling::Field field, float value) noexcept;
        void ApplyHandlingValues(const Handling::Values& values) noexcept;
        void RestoreOriginalHandling() noexcept;

        void ApplyProfile(const FeatureProfile& profile) noexcept;
        void Tick(bool nativeReady) noexcept;
        void Reset() noexcept;

        [[nodiscard]] PlayerFeatureSnapshot PlayerSnapshot() const noexcept;
        [[nodiscard]] VehicleFeatureSnapshot VehicleSnapshot() const noexcept;
        [[nodiscard]] HandlingFeatureSnapshot HandlingSnapshot() const noexcept;
        [[nodiscard]] Handling::Values HandlingValues() const noexcept;
        [[nodiscard]] FeatureProfile Profile() const noexcept;

    private:
        PlayerFeatures m_Player;
        VehicleFeatures m_Vehicle;
        HandlingFeatures m_Handling;
    };
}
