#include "VehicleFeatures.hpp"

#include "AutoRepair.hpp"
#include "CleanVehicle.hpp"
#include "EngineAlwaysOn.hpp"
#include "KeepVehicleClean.hpp"
#include "NoCollision.hpp"
#include "NoGravity.hpp"
#include "PutOnGround.hpp"
#include "RepairVehicle.hpp"
#include "VehicleGodMode.hpp"

namespace Sick::Backend::Features
{
    namespace
    {
        bool StoreChanged(std::atomic_bool& target, bool enabled) noexcept
        {
            return target.exchange(enabled, std::memory_order_acq_rel) != enabled;
        }
    }

    void VehicleFeatures::BumpRevision() noexcept
    {
        m_StateRevision.fetch_add(1, std::memory_order_release);
    }

    void VehicleFeatures::SetGodMode(bool enabled) noexcept { if (StoreChanged(m_GodModeRequested, enabled)) BumpRevision(); }
    void VehicleFeatures::SetAutoRepair(bool enabled) noexcept { if (StoreChanged(m_AutoRepairRequested, enabled)) BumpRevision(); }
    void VehicleFeatures::SetKeepClean(bool enabled) noexcept { if (StoreChanged(m_KeepCleanRequested, enabled)) BumpRevision(); }
    void VehicleFeatures::SetEngineAlwaysOn(bool enabled) noexcept { if (StoreChanged(m_EngineAlwaysOnRequested, enabled)) BumpRevision(); }
    void VehicleFeatures::SetNoGravity(bool enabled) noexcept { if (StoreChanged(m_NoGravityRequested, enabled)) BumpRevision(); }
    void VehicleFeatures::SetNoCollision(bool enabled) noexcept { if (StoreChanged(m_NoCollisionRequested, enabled)) BumpRevision(); }

    void VehicleFeatures::RequestRepair() noexcept { m_RepairRequests.fetch_add(1, std::memory_order_release); }
    void VehicleFeatures::RequestClean() noexcept { m_CleanRequests.fetch_add(1, std::memory_order_release); }
    void VehicleFeatures::RequestPutOnGround() noexcept { m_GroundRequests.fetch_add(1, std::memory_order_release); }

    void VehicleFeatures::RestoreVehicleState(Game::Vehicle vehicle) noexcept
    {
        if (vehicle == 0 || !m_Vehicle.Exists(vehicle))
            return;
        if (m_GodModeApplied)
            VehicleFeature::VehicleGodMode::Apply(m_Vehicle, vehicle, false);
        if (m_NoGravityApplied)
            VehicleFeature::NoGravity::Apply(m_Vehicle, vehicle, false);
        if (m_NoCollisionApplied)
            VehicleFeature::NoCollision::Apply(m_Vehicle, vehicle, false);
        // Engine Always On releases ownership when disabled rather than forcing
        // the engine off, which would be more invasive than the original game state.
    }

    void VehicleFeatures::ClearAppliedState() noexcept
    {
        m_GodModeApplied = false;
        m_AutoRepairApplied = false;
        m_KeepCleanApplied = false;
        m_EngineAlwaysOnApplied = false;
        m_NoGravityApplied = false;
        m_NoCollisionApplied = false;
    }

    void VehicleFeatures::SetActiveState(bool vehicleAvailable) noexcept
    {
        const bool godMode = m_GodModeRequested.load(std::memory_order_acquire);
        const bool autoRepair = m_AutoRepairRequested.load(std::memory_order_acquire);
        const bool keepClean = m_KeepCleanRequested.load(std::memory_order_acquire);
        const bool engineAlwaysOn = m_EngineAlwaysOnRequested.load(std::memory_order_acquire);
        const bool noGravity = m_NoGravityRequested.load(std::memory_order_acquire);
        const bool noCollision = m_NoCollisionRequested.load(std::memory_order_acquire);
        m_GodModeActive.store(vehicleAvailable && godMode && m_GodModeApplied, std::memory_order_release);
        m_AutoRepairActive.store(vehicleAvailable && autoRepair && m_AutoRepairApplied, std::memory_order_release);
        m_KeepCleanActive.store(vehicleAvailable && keepClean && m_KeepCleanApplied, std::memory_order_release);
        m_EngineAlwaysOnActive.store(vehicleAvailable && engineAlwaysOn && m_EngineAlwaysOnApplied, std::memory_order_release);
        m_NoGravityActive.store(vehicleAvailable && noGravity && m_NoGravityApplied, std::memory_order_release);
        m_NoCollisionActive.store(vehicleAvailable && noCollision && m_NoCollisionApplied, std::memory_order_release);
    }

    void VehicleFeatures::Tick() noexcept
    {
        const bool godMode = m_GodModeRequested.load(std::memory_order_acquire);
        const bool autoRepair = m_AutoRepairRequested.load(std::memory_order_acquire);
        const bool keepClean = m_KeepCleanRequested.load(std::memory_order_acquire);
        const bool engineAlwaysOn = m_EngineAlwaysOnRequested.load(std::memory_order_acquire);
        const bool noGravity = m_NoGravityRequested.load(std::memory_order_acquire);
        const bool noCollision = m_NoCollisionRequested.load(std::memory_order_acquire);
        const bool persistent = godMode || autoRepair || keepClean || engineAlwaysOn || noGravity || noCollision;

        const auto revision = m_StateRevision.load(std::memory_order_acquire);
        const auto repairRequests = m_RepairRequests.load(std::memory_order_acquire);
        const auto cleanRequests = m_CleanRequests.load(std::memory_order_acquire);
        const auto groundRequests = m_GroundRequests.load(std::memory_order_acquire);
        const bool revisionChanged = revision != m_SeenStateRevision;
        const bool repairPending = repairRequests != m_SeenRepairRequests;
        const bool cleanPending = cleanRequests != m_SeenCleanRequests;
        const bool groundPending = groundRequests != m_SeenGroundRequests;
        const bool actionPending = repairPending || cleanPending || groundPending;

        if (!persistent && !actionPending && !revisionChanged && m_LastVehicle == 0)
            return;

        bool pollDue = revisionChanged || actionPending;
        if (persistent || m_LastVehicle != 0)
        {
            if (++m_PollTicks >= VehiclePollTicks)
            {
                m_PollTicks = 0;
                pollDue = true;
            }
        }

        bool refreshDue = false;
        bool maintenanceDue = false;
        if (persistent)
        {
            if (++m_RefreshTicks >= RefreshTicks)
            {
                m_RefreshTicks = 0;
                refreshDue = true;
                pollDue = true;
            }
            if (++m_MaintenanceTicks >= MaintenanceTicks)
            {
                m_MaintenanceTicks = 0;
                maintenanceDue = true;
                pollDue = true;
            }
        }
        else
        {
            m_RefreshTicks = 0;
            m_MaintenanceTicks = 0;
        }

        if (!pollDue)
            return;

        const auto vehicle = m_Vehicle.CurrentVehicle();
        const bool vehicleChanged = vehicle != m_LastVehicle;
        if (vehicleChanged)
        {
            RestoreVehicleState(m_LastVehicle);
            ClearAppliedState();
            m_LastVehicle = vehicle;
        }

        const bool available = vehicle != 0 && m_Vehicle.Exists(vehicle);
        if (!available)
        {
            ClearAppliedState();
            m_LastVehicle = 0;
            SetActiveState(false);
        }
        else
        {
            if (godMode != m_GodModeApplied || (refreshDue && godMode))
            {
                VehicleFeature::VehicleGodMode::Apply(m_Vehicle, vehicle, godMode);
                m_GodModeApplied = godMode;
            }

            if (engineAlwaysOn)
            {
                if (!m_EngineAlwaysOnApplied || refreshDue)
                    VehicleFeature::EngineAlwaysOn::Apply(m_Vehicle, vehicle);
                m_EngineAlwaysOnApplied = true;
            }
            else
            {
                m_EngineAlwaysOnApplied = false;
            }

            if (noGravity != m_NoGravityApplied || (refreshDue && noGravity))
            {
                VehicleFeature::NoGravity::Apply(m_Vehicle, vehicle, noGravity);
                m_NoGravityApplied = noGravity;
            }

            if (noCollision != m_NoCollisionApplied || (refreshDue && noCollision))
            {
                VehicleFeature::NoCollision::Apply(m_Vehicle, vehicle, noCollision);
                m_NoCollisionApplied = noCollision;
            }

            if (autoRepair && (!m_AutoRepairApplied || maintenanceDue))
                VehicleFeature::AutoRepair::Apply(m_Vehicle, vehicle);
            m_AutoRepairApplied = autoRepair;

            if (keepClean && (!m_KeepCleanApplied || maintenanceDue))
                VehicleFeature::KeepVehicleClean::Apply(m_Vehicle, vehicle);
            m_KeepCleanApplied = keepClean;

            if (repairPending)
                VehicleFeature::RepairVehicle::Apply(m_Vehicle, vehicle);
            if (cleanPending)
                VehicleFeature::CleanVehicle::Apply(m_Vehicle, vehicle);
            if (groundPending)
                static_cast<void>(VehicleFeature::PutOnGround::Apply(m_Vehicle, vehicle));

            SetActiveState(true);
            if (!persistent)
            {
                ClearAppliedState();
                m_LastVehicle = 0;
                SetActiveState(false);
            }
        }

        m_SeenStateRevision = revision;
        m_SeenRepairRequests = repairRequests;
        m_SeenCleanRequests = cleanRequests;
        m_SeenGroundRequests = groundRequests;
    }

    void VehicleFeatures::Reset() noexcept
    {
        m_GodModeRequested.store(false, std::memory_order_release);
        m_AutoRepairRequested.store(false, std::memory_order_release);
        m_KeepCleanRequested.store(false, std::memory_order_release);
        m_EngineAlwaysOnRequested.store(false, std::memory_order_release);
        m_NoGravityRequested.store(false, std::memory_order_release);
        m_NoCollisionRequested.store(false, std::memory_order_release);
        m_GodModeActive.store(false, std::memory_order_release);
        m_AutoRepairActive.store(false, std::memory_order_release);
        m_KeepCleanActive.store(false, std::memory_order_release);
        m_EngineAlwaysOnActive.store(false, std::memory_order_release);
        m_NoGravityActive.store(false, std::memory_order_release);
        m_NoCollisionActive.store(false, std::memory_order_release);
        ClearAppliedState();
        m_StateRevision.store(0, std::memory_order_release);
        m_RepairRequests.store(0, std::memory_order_release);
        m_CleanRequests.store(0, std::memory_order_release);
        m_GroundRequests.store(0, std::memory_order_release);
        m_SeenStateRevision = 0;
        m_SeenRepairRequests = 0;
        m_SeenCleanRequests = 0;
        m_SeenGroundRequests = 0;
        m_PollTicks = 0;
        m_RefreshTicks = 0;
        m_MaintenanceTicks = 0;
        m_LastVehicle = 0;
    }

    VehicleFeatureSnapshot VehicleFeatures::Snapshot() const noexcept
    {
        return {
            .godMode = {.requested = m_GodModeRequested.load(std::memory_order_acquire), .active = m_GodModeActive.load(std::memory_order_acquire)},
            .autoRepair = {.requested = m_AutoRepairRequested.load(std::memory_order_acquire), .active = m_AutoRepairActive.load(std::memory_order_acquire)},
            .keepClean = {.requested = m_KeepCleanRequested.load(std::memory_order_acquire), .active = m_KeepCleanActive.load(std::memory_order_acquire)},
            .engineAlwaysOn = {.requested = m_EngineAlwaysOnRequested.load(std::memory_order_acquire), .active = m_EngineAlwaysOnActive.load(std::memory_order_acquire)},
            .noGravity = {.requested = m_NoGravityRequested.load(std::memory_order_acquire), .active = m_NoGravityActive.load(std::memory_order_acquire)},
            .noCollision = {.requested = m_NoCollisionRequested.load(std::memory_order_acquire), .active = m_NoCollisionActive.load(std::memory_order_acquire)},
        };
    }
}
