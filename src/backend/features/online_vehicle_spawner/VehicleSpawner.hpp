#pragma once

#include "backend/BackendTypes.hpp"
#include "game/natives/NativeTypes.hpp"
#include "game/services/VehicleSpawnerService.hpp"

#include <atomic>
#include <string_view>

namespace Sick::Backend::Features::OnlineVehicleSpawner
{
    class VehicleSpawner final
    {
    public:
        [[nodiscard]] static Game::Hash HashModelName(std::string_view modelName) noexcept;

        [[nodiscard]] bool TryQueue(Game::Hash modelHash) noexcept;
        void QueueFailed() noexcept;
        void Reject(Game::Hash modelHash, VehicleSpawnerState state) noexcept;
        void Spawn(Game::Hash modelHash, bool enterVehicle) noexcept;
        void Reset() noexcept;
        [[nodiscard]] VehicleSpawnerSnapshot Snapshot() const noexcept;

    private:
        static constexpr std::uint32_t ModelLoadYieldLimit = 600;

        void Finish(VehicleSpawnerState state) noexcept;

        Game::VehicleSpawnerService m_Service;
        std::atomic<VehicleSpawnerState> m_State{VehicleSpawnerState::Idle};
        std::atomic_uint32_t m_ModelHash{};
        std::atomic_bool m_Busy{};
    };
}
