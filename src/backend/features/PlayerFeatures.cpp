#include "PlayerFeatures.hpp"

namespace Sick::Backend::Features
{
    void PlayerFeatures::SetGodMode(bool enabled) noexcept
    {
        const auto previous = m_GodModeRequested.exchange(enabled, std::memory_order_acq_rel);
        if (previous != enabled)
            m_GodModeRevision.fetch_add(1, std::memory_order_release);
    }

    void PlayerFeatures::Tick() noexcept
    {
        const bool requested = m_GodModeRequested.load(std::memory_order_acquire);
        const auto revision = m_GodModeRevision.load(std::memory_order_acquire);
        const bool changed = revision != m_SeenGodModeRevision;

        if (!changed)
        {
            if (!requested && m_LastGodModePed == 0)
            {
                m_GodModeActive.store(false, std::memory_order_release);
                return;
            }

            if (++m_GodModeRefreshTicks < GodModeRefreshTicks)
                return;
        }

        m_SeenGodModeRevision = revision;
        m_GodModeRefreshTicks = 0;

        if (!requested)
        {
            if (m_LastGodModePed != 0 && m_Player.Exists(m_LastGodModePed))
                m_Player.SetInvincible(m_LastGodModePed, false);
            m_LastGodModePed = 0;
            m_GodModeActive.store(false, std::memory_order_release);
            return;
        }

        const auto ped = m_Player.LocalPed();
        if (ped == 0 || !m_Player.Exists(ped))
        {
            m_GodModeActive.store(false, std::memory_order_release);
            return;
        }

        if (m_LastGodModePed != 0 && m_LastGodModePed != ped && m_Player.Exists(m_LastGodModePed))
            m_Player.SetInvincible(m_LastGodModePed, false);

        m_Player.SetInvincible(ped, true);
        m_LastGodModePed = ped;
        m_GodModeActive.store(true, std::memory_order_release);
    }

    void PlayerFeatures::Reset() noexcept
    {
        m_GodModeRequested.store(false, std::memory_order_release);
        m_GodModeActive.store(false, std::memory_order_release);
        m_GodModeRevision.fetch_add(1, std::memory_order_release);
        m_SeenGodModeRevision = m_GodModeRevision.load(std::memory_order_acquire);
        m_GodModeRefreshTicks = 0;
        m_LastGodModePed = 0;
    }

    PlayerFeatureSnapshot PlayerFeatures::Snapshot() const noexcept
    {
        return {
            .godMode = {
                .requested = m_GodModeRequested.load(std::memory_order_acquire),
                .active = m_GodModeActive.load(std::memory_order_acquire),
            },
        };
    }
}
