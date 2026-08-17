#include "FeatureManager.hpp"

namespace Sick::Backend::Features
{
    void FeatureManager::SetGodMode(bool enabled) noexcept { m_Player.SetGodMode(enabled); }
    void FeatureManager::SetInfiniteOxygen(bool enabled) noexcept { m_Player.SetInfiniteOxygen(enabled); }
    void FeatureManager::SetNoRagdoll(bool enabled) noexcept { m_Player.SetNoRagdoll(enabled); }
    void FeatureManager::SetSuperJump(bool enabled) noexcept { m_Player.SetSuperJump(enabled); }
    void FeatureManager::SetSeatBelt(bool enabled) noexcept { m_Player.SetSeatBelt(enabled); }
    void FeatureManager::SetNoWantedLevel(bool enabled) noexcept { m_Player.SetNoWantedLevel(enabled); }
    void FeatureManager::SetWantedLevel(int level) noexcept { m_Player.SetWantedLevel(level); }
    void FeatureManager::SetFastRun(bool enabled) noexcept { m_Player.SetFastRun(enabled); }
    void FeatureManager::SetFastSwim(bool enabled) noexcept { m_Player.SetFastSwim(enabled); }
    void FeatureManager::SetKeepPlayerClean(bool enabled) noexcept { m_Player.SetKeepPlayerClean(enabled); }
    void FeatureManager::SetAqualung(bool enabled) noexcept { m_Player.SetAqualung(enabled); }
    void FeatureManager::SetNoGravity(bool enabled) noexcept { m_Player.SetNoGravity(enabled); }
    void FeatureManager::SetWaterproof(bool enabled) noexcept { m_Player.SetWaterproof(enabled); }

    void FeatureManager::SetVehicleGodMode(bool enabled) noexcept { m_Vehicle.SetGodMode(enabled); }
    void FeatureManager::SetVehicleAutoRepair(bool enabled) noexcept { m_Vehicle.SetAutoRepair(enabled); }
    void FeatureManager::SetVehicleKeepClean(bool enabled) noexcept { m_Vehicle.SetKeepClean(enabled); }
    void FeatureManager::SetVehicleEngineAlwaysOn(bool enabled) noexcept { m_Vehicle.SetEngineAlwaysOn(enabled); }
    void FeatureManager::SetVehicleNoGravity(bool enabled) noexcept { m_Vehicle.SetNoGravity(enabled); }
    void FeatureManager::SetVehicleNoCollision(bool enabled) noexcept { m_Vehicle.SetNoCollision(enabled); }
    void FeatureManager::RepairVehicle() noexcept { m_Vehicle.RequestRepair(); }
    void FeatureManager::CleanVehicle() noexcept { m_Vehicle.RequestClean(); }
    void FeatureManager::PutVehicleOnGround() noexcept { m_Vehicle.RequestPutOnGround(); }

    void FeatureManager::SetHandlingEditorActive(bool active) noexcept { m_Handling.SetEditorActive(active); }
    void FeatureManager::SetHandlingValue(Handling::Field field, float value) noexcept { m_Handling.SetValue(field, value); }
    void FeatureManager::ApplyHandlingValues(const Handling::Values& values) noexcept { m_Handling.ApplyValues(values); }
    void FeatureManager::RestoreOriginalHandling() noexcept { m_Handling.RequestRestoreOriginal(); }

    void FeatureManager::ApplyProfile(const FeatureProfile& profile) noexcept
    {
        if (profile.version != FeatureProfile::CurrentVersion)
            return;

        m_Player.SetGodMode(profile.player.godMode);
        m_Player.SetInfiniteOxygen(profile.player.infiniteOxygen);
        m_Player.SetNoRagdoll(profile.player.noRagdoll);
        m_Player.SetSuperJump(profile.player.superJump);
        m_Player.SetSeatBelt(profile.player.seatBelt);
        m_Player.SetNoWantedLevel(profile.player.noWantedLevel);
        m_Player.SetWantedLevel(profile.player.wantedLevel);
        m_Player.SetFastRun(profile.player.fastRun);
        m_Player.SetFastSwim(profile.player.fastSwim);
        m_Player.SetKeepPlayerClean(profile.player.keepPlayerClean);
        m_Player.SetAqualung(profile.player.aqualung);
        m_Player.SetNoGravity(profile.player.noGravity);
        m_Player.SetWaterproof(profile.player.waterproof);

        m_Vehicle.SetGodMode(profile.vehicle.godMode);
        m_Vehicle.SetAutoRepair(profile.vehicle.autoRepair);
        m_Vehicle.SetKeepClean(profile.vehicle.keepClean);
        m_Vehicle.SetEngineAlwaysOn(profile.vehicle.engineAlwaysOn);
        m_Vehicle.SetNoGravity(profile.vehicle.noGravity);
        m_Vehicle.SetNoCollision(profile.vehicle.noCollision);
    }

    void FeatureManager::Tick(bool nativeReady) noexcept
    {
        if (!nativeReady)
            return;
        m_Player.Tick();
        m_Vehicle.Tick();
        m_Handling.Tick();
    }

    void FeatureManager::Reset() noexcept
    {
        m_Player.Reset();
        m_Vehicle.Reset();
        m_Handling.Reset();
    }

    PlayerFeatureSnapshot FeatureManager::PlayerSnapshot() const noexcept { return m_Player.Snapshot(); }
    VehicleFeatureSnapshot FeatureManager::VehicleSnapshot() const noexcept { return m_Vehicle.Snapshot(); }
    HandlingFeatureSnapshot FeatureManager::HandlingSnapshot() const noexcept { return m_Handling.Snapshot(); }
    Handling::Values FeatureManager::HandlingValues() const noexcept { return m_Handling.Values(); }

    FeatureProfile FeatureManager::Profile() const noexcept
    {
        const auto player = m_Player.Snapshot();
        const auto vehicle = m_Vehicle.Snapshot();
        return {
            .version = FeatureProfile::CurrentVersion,
            .player = {
                .godMode = player.godMode.requested,
                .infiniteOxygen = player.infiniteOxygen.requested,
                .noRagdoll = player.noRagdoll.requested,
                .superJump = player.superJump.requested,
                .seatBelt = player.seatBelt.requested,
                .noWantedLevel = player.noWantedLevel.requested,
                .wantedLevel = player.wantedLevel,
                .fastRun = player.fastRun.requested,
                .fastSwim = player.fastSwim.requested,
                .keepPlayerClean = player.keepPlayerClean.requested,
                .aqualung = player.aqualung.requested,
                .noGravity = player.noGravity.requested,
                .waterproof = player.waterproof.requested,
            },
            .vehicle = {
                .godMode = vehicle.godMode.requested,
                .autoRepair = vehicle.autoRepair.requested,
                .keepClean = vehicle.keepClean.requested,
                .engineAlwaysOn = vehicle.engineAlwaysOn.requested,
                .noGravity = vehicle.noGravity.requested,
                .noCollision = vehicle.noCollision.requested,
            },
        };
    }
}
