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
        };
    }
}
