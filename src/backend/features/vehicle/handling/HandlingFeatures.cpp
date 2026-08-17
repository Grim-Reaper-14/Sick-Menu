#include "HandlingFeatures.hpp"

#include "game/handling/HandlingBackend.hpp"

#include <algorithm>
#include <cmath>

namespace Sick::Backend::Features
{
    HandlingFeatures::HandlingFeatures() noexcept
    {
        for (auto& value : m_Values)
            value.store(0.0F, std::memory_order_relaxed);
    }

    void HandlingFeatures::SetEditorActive(bool active) noexcept
    {
        const bool previous = m_EditorActive.exchange(active, std::memory_order_acq_rel);
        if (active && !previous)
            m_ForcePoll.store(true, std::memory_order_release);
    }

    void HandlingFeatures::SetValue(Handling::Field field, float value) noexcept
    {
        const auto index = Handling::ToIndex(field);
        if (index >= Handling::FieldCount)
            return;

        const auto& spec = Handling::Spec(field);
        value = std::clamp(value, spec.minimum, spec.maximum);
        if (spec.integral)
            value = std::round(value);

        const auto previous = m_Values[index].load(std::memory_order_acquire);
        if (std::fabs(previous - value) < 0.00001F)
            return;

        const auto mask = std::uint64_t{1} << index;
        // Publish intent before the value and again after it. A game-thread
        // claim that lands between those operations cannot lose this update.
        m_DirtyMask.fetch_or(mask, std::memory_order_acq_rel);
        m_Values[index].store(value, std::memory_order_release);
        m_DirtyMask.fetch_or(mask, std::memory_order_acq_rel);
        m_ForcePoll.store(true, std::memory_order_release);
        m_Revision.fetch_add(1, std::memory_order_acq_rel);
    }

    void HandlingFeatures::ApplyValues(const Handling::Values& values) noexcept
    {
        m_DirtyMask.fetch_or(Handling::AllFieldMask, std::memory_order_acq_rel);
        for (std::size_t index = 0; index < Handling::FieldCount; ++index)
        {
            const auto field = static_cast<Handling::Field>(index);
            const auto& spec = Handling::Spec(field);
            auto value = std::clamp(values[index], spec.minimum, spec.maximum);
            if (spec.integral)
                value = std::round(value);
            m_Values[index].store(value, std::memory_order_release);
        }
        m_DirtyMask.fetch_or(Handling::AllFieldMask, std::memory_order_acq_rel);
        m_ForcePoll.store(true, std::memory_order_release);
        m_Revision.fetch_add(1, std::memory_order_acq_rel);
    }

    void HandlingFeatures::RequestRestoreOriginal() noexcept
    {
        m_RestoreRequested.store(true, std::memory_order_release);
        m_ForcePoll.store(true, std::memory_order_release);
    }

    void HandlingFeatures::PublishAvailability(bool available) noexcept
    {
        if (m_BackendAvailable.exchange(available, std::memory_order_acq_rel) != available)
            m_Revision.fetch_add(1, std::memory_order_acq_rel);
    }

    void HandlingFeatures::PublishAttached(bool attached) noexcept
    {
        if (m_VehicleAttached.exchange(attached, std::memory_order_acq_rel) != attached)
            m_Revision.fetch_add(1, std::memory_order_acq_rel);
    }

    bool HandlingFeatures::AttachVehicle(Game::Vehicle vehicle) noexcept
    {
        auto& backend = Game::Handling::HandlingBackend::Get();
        Handling::Values current{};
        if (!backend.Read(vehicle, current))
        {
            m_AttachedVehicle = 0;
            m_HasOriginal = false;
            PublishAttached(false);
            return false;
        }

        m_AttachedVehicle = vehicle;
        m_Original = current;
        m_HasOriginal = true;
        for (std::size_t index = 0; index < Handling::FieldCount; ++index)
        {
            const auto mask = std::uint64_t{1} << index;
            if ((m_DirtyMask.load(std::memory_order_acquire) & mask) == 0)
                m_Values[index].store(current[index], std::memory_order_release);
        }
        PublishAttached(true);
        m_Revision.fetch_add(1, std::memory_order_acq_rel);
        return true;
    }

    void HandlingFeatures::QueueOriginalRestore() noexcept
    {
        if (!m_HasOriginal)
            return;
        m_DirtyMask.fetch_or(Handling::AllFieldMask, std::memory_order_acq_rel);
        for (std::size_t index = 0; index < Handling::FieldCount; ++index)
            m_Values[index].store(m_Original[index], std::memory_order_release);
        m_DirtyMask.fetch_or(Handling::AllFieldMask, std::memory_order_acq_rel);
        m_Revision.fetch_add(1, std::memory_order_acq_rel);
    }

    void HandlingFeatures::ApplyDirty(Game::Vehicle vehicle) noexcept
    {
        auto& backend = Game::Handling::HandlingBackend::Get();
        const auto pending = m_DirtyMask.load(std::memory_order_acquire);
        std::size_t writes{};
        for (std::size_t index = 0; index < Handling::FieldCount && writes < MaxWritesPerTick; ++index)
        {
            const auto mask = std::uint64_t{1} << index;
            if ((pending & mask) == 0)
                continue;

            // Claim the field before reading its desired value. If the frontend
            // changes it concurrently, SetValue() sets this bit again and the
            // newer value is guaranteed another game-thread application pass.
            const auto claimed = m_DirtyMask.fetch_and(~mask, std::memory_order_acq_rel);
            if ((claimed & mask) == 0)
                continue;

            const auto field = static_cast<Handling::Field>(index);
            const float value = m_Values[index].load(std::memory_order_acquire);
            if (!backend.Write(vehicle, field, value))
            {
                m_DirtyMask.fetch_or(mask, std::memory_order_acq_rel);
                continue;
            }

            ++writes;
        }
    }

    void HandlingFeatures::Tick() noexcept
    {
        const bool editorActive = m_EditorActive.load(std::memory_order_acquire);
        const auto dirty = m_DirtyMask.load(std::memory_order_acquire);
        const bool restoreRequested = m_RestoreRequested.load(std::memory_order_acquire);
        const bool forcePoll = m_ForcePoll.exchange(false, std::memory_order_acq_rel);
        if (!editorActive && dirty == 0 && !restoreRequested && !forcePoll)
        {
            m_PollTicks = 0;
            m_AvailabilityTicks = 0;
            return;
        }

        bool checkAvailability = forcePoll;
        if (!checkAvailability && ++m_AvailabilityTicks >= AvailabilityPollTicks)
        {
            m_AvailabilityTicks = 0;
            checkAvailability = true;
        }

        if (checkAvailability)
        {
            const bool available = Game::Handling::HandlingBackend::Get().Available();
            PublishAvailability(available);
            if (!available)
            {
                m_AttachedVehicle = 0;
                m_HasOriginal = false;
                PublishAttached(false);
                m_PollTicks = 0;
                return;
            }
        }
        else if (!m_BackendAvailable.load(std::memory_order_acquire))
        {
            return;
        }

        bool pollNow = forcePoll || m_AttachedVehicle == 0 || dirty != 0 || restoreRequested;
        if (editorActive && !pollNow)
        {
            if (++m_PollTicks >= VehiclePollTicks)
            {
                m_PollTicks = 0;
                pollNow = true;
            }
        }

        Game::Vehicle vehicle = m_AttachedVehicle;
        if (pollNow)
        {
            vehicle = m_Vehicle.CurrentVehicle();
            if (vehicle == 0 || !m_Vehicle.Exists(vehicle))
            {
                m_AttachedVehicle = 0;
                m_HasOriginal = false;
                PublishAttached(false);
                return;
            }

            if (vehicle != m_AttachedVehicle || !m_HasOriginal)
            {
                if (!AttachVehicle(vehicle))
                    return;
            }
        }

        if (m_RestoreRequested.exchange(false, std::memory_order_acq_rel))
            QueueOriginalRestore();

        if (vehicle != 0 && m_DirtyMask.load(std::memory_order_acquire) != 0)
            ApplyDirty(vehicle);
    }

    void HandlingFeatures::Reset() noexcept
    {
        m_DirtyMask.store(0, std::memory_order_release);
        m_EditorActive.store(false, std::memory_order_release);
        m_ForcePoll.store(false, std::memory_order_release);
        m_RestoreRequested.store(false, std::memory_order_release);
        m_BackendAvailable.store(false, std::memory_order_release);
        m_VehicleAttached.store(false, std::memory_order_release);
        m_Revision.store(0, std::memory_order_release);
        m_AttachedVehicle = 0;
        m_HasOriginal = false;
        m_PollTicks = 0;
        m_AvailabilityTicks = 0;
    }

    HandlingFeatureSnapshot HandlingFeatures::Snapshot() const noexcept
    {
        HandlingFeatureSnapshot snapshot{};
        snapshot.backendAvailable = m_BackendAvailable.load(std::memory_order_acquire);
        snapshot.vehicleAttached = m_VehicleAttached.load(std::memory_order_acquire);
        snapshot.revision = m_Revision.load(std::memory_order_acquire);
        for (std::size_t index = 0; index < Handling::FieldCount; ++index)
            snapshot.values[index] = m_Values[index].load(std::memory_order_acquire);
        return snapshot;
    }

    Handling::Values HandlingFeatures::Values() const noexcept
    {
        Handling::Values values{};
        for (std::size_t index = 0; index < Handling::FieldCount; ++index)
            values[index] = m_Values[index].load(std::memory_order_acquire);
        return values;
    }
}
