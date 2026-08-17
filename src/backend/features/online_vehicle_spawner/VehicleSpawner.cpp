#include "VehicleSpawner.hpp"

#include "backend/tasking/GameFiberScheduler.hpp"
#include "game/scripts/ScriptTypes.hpp"

namespace Sick::Backend::Features::OnlineVehicleSpawner
{
    Game::Hash VehicleSpawner::HashModelName(std::string_view modelName) noexcept
    {
        return Game::Scripts::Joaat(modelName);
    }

    bool VehicleSpawner::TryQueue(Game::Hash modelHash) noexcept
    {
        bool expected{};
        if (!m_Busy.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return false;

        m_ModelHash.store(modelHash, std::memory_order_release);
        m_State.store(VehicleSpawnerState::Queued, std::memory_order_release);
        return true;
    }

    void VehicleSpawner::QueueFailed() noexcept
    {
        Finish(VehicleSpawnerState::Failed);
    }

    void VehicleSpawner::Reject(Game::Hash modelHash, VehicleSpawnerState state) noexcept
    {
        if (m_Busy.load(std::memory_order_acquire))
            return;
        m_ModelHash.store(modelHash, std::memory_order_release);
        m_State.store(state, std::memory_order_release);
    }

    void VehicleSpawner::Spawn(Game::Hash modelHash, bool enterVehicle) noexcept
    {
        m_State.store(VehicleSpawnerState::Loading, std::memory_order_release);

        if (!m_Service.IsVehicleModel(modelHash))
        {
            Finish(VehicleSpawnerState::InvalidModel);
            return;
        }

        m_Service.RequestModel(modelHash);
        bool loaded{};
        for (std::uint32_t attempt = 0; attempt < ModelLoadYieldLimit; ++attempt)
        {
            if (m_Service.IsModelLoaded(modelHash))
            {
                loaded = true;
                break;
            }
            Tasking::GameFiberScheduler::YieldCurrent();
        }

        if (!loaded)
        {
            m_Service.ReleaseModel(modelHash);
            Finish(VehicleSpawnerState::TimedOut);
            return;
        }

        const auto vehicle = m_Service.SpawnAtPlayer(modelHash, enterVehicle);
        m_Service.ReleaseModel(modelHash);
        Finish(vehicle != 0 ? VehicleSpawnerState::Spawned : VehicleSpawnerState::Failed);
    }

    void VehicleSpawner::Finish(VehicleSpawnerState state) noexcept
    {
        m_State.store(state, std::memory_order_release);
        m_Busy.store(false, std::memory_order_release);
    }

    void VehicleSpawner::Reset() noexcept
    {
        m_Busy.store(false, std::memory_order_release);
        m_ModelHash.store(0, std::memory_order_release);
        m_State.store(VehicleSpawnerState::Idle, std::memory_order_release);
    }

    VehicleSpawnerSnapshot VehicleSpawner::Snapshot() const noexcept
    {
        return {
            .state = m_State.load(std::memory_order_acquire),
            .modelHash = m_ModelHash.load(std::memory_order_acquire),
            .busy = m_Busy.load(std::memory_order_acquire),
        };
    }
}
