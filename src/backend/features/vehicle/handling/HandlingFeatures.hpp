#pragma once

#include "backend/BackendTypes.hpp"
#include "game/services/VehicleService.hpp"
#include "shared/HandlingTypes.hpp"

#include <array>
#include <atomic>
#include <cstdint>

namespace Sick::Backend::Features
{
    class HandlingFeatures final
    {
    public:
        HandlingFeatures() noexcept;

        void SetEditorActive(bool active) noexcept;
        void SetValue(Handling::Field field, float value) noexcept;
        void ApplyValues(const Handling::Values& values) noexcept;
        void RequestRestoreOriginal() noexcept;

        void Tick() noexcept;
        void Reset() noexcept;
        [[nodiscard]] HandlingFeatureSnapshot Snapshot() const noexcept;
        [[nodiscard]] Handling::Values Values() const noexcept;

    private:
        static constexpr std::uint32_t VehiclePollTicks = 15;
        static constexpr std::uint32_t AvailabilityPollTicks = 30;
        static constexpr std::size_t MaxWritesPerTick = 8;

        void PublishAvailability(bool available) noexcept;
        void PublishAttached(bool attached) noexcept;
        bool AttachVehicle(Game::Vehicle vehicle) noexcept;
        void QueueOriginalRestore() noexcept;
        void ApplyDirty(Game::Vehicle vehicle) noexcept;

        Game::VehicleService m_Vehicle;
        std::array<std::atomic<float>, Handling::FieldCount> m_Values{};
        Handling::Values m_Original{};
        std::atomic_uint64_t m_DirtyMask{};
        std::atomic_bool m_EditorActive{};
        std::atomic_bool m_ForcePoll{};
        std::atomic_bool m_RestoreRequested{};
        std::atomic_bool m_BackendAvailable{};
        std::atomic_bool m_VehicleAttached{};
        std::atomic_uint64_t m_Revision{};
        Game::Vehicle m_AttachedVehicle{};
        bool m_HasOriginal{};
        std::uint32_t m_PollTicks{};
        std::uint32_t m_AvailabilityTicks{};
    };
}
