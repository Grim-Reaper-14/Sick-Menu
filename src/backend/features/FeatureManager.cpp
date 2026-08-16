#include "FeatureManager.hpp"

namespace Sick::Backend::Features
{
    void FeatureManager::SetGodMode(bool enabled) noexcept
    {
        m_Player.SetGodMode(enabled);
    }

    void FeatureManager::ApplyProfile(const FeatureProfile& profile) noexcept
    {
        if (profile.version != FeatureProfile::CurrentVersion)
            return;
        m_Player.SetGodMode(profile.player.godMode);
    }

    void FeatureManager::Tick(bool nativeReady) noexcept
    {
        if (nativeReady)
            m_Player.Tick();
    }

    void FeatureManager::Reset() noexcept
    {
        m_Player.Reset();
    }

    PlayerFeatureSnapshot FeatureManager::PlayerSnapshot() const noexcept
    {
        return m_Player.Snapshot();
    }

    FeatureProfile FeatureManager::Profile() const noexcept
    {
        const auto player = m_Player.Snapshot();
        return {
            .version = FeatureProfile::CurrentVersion,
            .player = {
                .godMode = player.godMode.requested,
            },
        };
    }
}
