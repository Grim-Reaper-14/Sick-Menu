#pragma once

#include "backend/BackendTypes.hpp"
#include "game/services/VehicleService.hpp"

#include <atomic>
#include <cstdint>

namespace Sick::Backend::Features
{
    class VehicleFeatures final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void SetAutoRepair(bool enabled) noexcept;
        void SetKeepClean(bool enabled) noexcept;
        void SetEngineAlwaysOn(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetNoCollision(bool enabled) noexcept;

        void RequestRepair() noexcept;
        void RequestClean() noexcept;
        void RequestPutOnGround() noexcept;

        void Tick() noexcept;
        void Reset() noexcept;
        [[nodiscard]] VehicleFeatureSnapshot Snapshot() const noexcept;

    private:
        static constexpr std::uint32_t VehiclePollTicks = 15;
        static constexpr std::uint32_t RefreshTicks = 30;
        static constexpr std::uint32_t MaintenanceTicks = 60;

        void BumpRevision() noexcept;
        void RestoreVehicleState(Game::Vehicle vehicle) noexcept;
        void ClearAppliedState() noexcept;
        void SetActiveState(bool vehicleAvailable) noexcept;

        Game::VehicleService m_Vehicle;

        std::atomic_bool m_GodModeRequested{};
        std::atomic_bool m_AutoRepairRequested{};
        std::atomic_bool m_KeepCleanRequested{};
        std::atomic_bool m_EngineAlwaysOnRequested{};
        std::atomic_bool m_NoGravityRequested{};
        std::atomic_bool m_NoCollisionRequested{};

        std::atomic_bool m_GodModeActive{};
        std::atomic_bool m_AutoRepairActive{};
        std::atomic_bool m_KeepCleanActive{};
        std::atomic_bool m_EngineAlwaysOnActive{};
        std::atomic_bool m_NoGravityActive{};
        std::atomic_bool m_NoCollisionActive{};

        std::atomic_uint64_t m_StateRevision{};
        std::atomic_uint64_t m_RepairRequests{};
        std::atomic_uint64_t m_CleanRequests{};
        std::atomic_uint64_t m_GroundRequests{};

        std::uint64_t m_SeenStateRevision{};
        std::uint64_t m_SeenRepairRequests{};
        std::uint64_t m_SeenCleanRequests{};
        std::uint64_t m_SeenGroundRequests{};
        bool m_GodModeApplied{};
        bool m_AutoRepairApplied{};
        bool m_KeepCleanApplied{};
        bool m_EngineAlwaysOnApplied{};
        bool m_NoGravityApplied{};
        bool m_NoCollisionApplied{};
        std::uint32_t m_PollTicks{};
        std::uint32_t m_RefreshTicks{};
        std::uint32_t m_MaintenanceTicks{};
        Game::Vehicle m_LastVehicle{};
    };
}
