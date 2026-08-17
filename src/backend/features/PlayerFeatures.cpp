#include "PlayerFeatures.hpp"

#include <algorithm>

namespace Sick::Backend::Features
{
    void PlayerFeatures::SetGodMode(bool enabled) noexcept { m_GodModeRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetInfiniteOxygen(bool enabled) noexcept { m_InfiniteOxygenRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetNoRagdoll(bool enabled) noexcept { m_NoRagdollRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetSuperJump(bool enabled) noexcept { m_SuperJumpRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetSeatBelt(bool enabled) noexcept { m_SeatBeltRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetNoWantedLevel(bool enabled) noexcept { m_NoWantedLevelRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetWantedLevel(int level) noexcept { m_WantedLevelRequested.store(std::clamp(level, 0, 5), std::memory_order_release); }
    void PlayerFeatures::SetFastRun(bool enabled) noexcept { m_FastRunRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetFastSwim(bool enabled) noexcept { m_FastSwimRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetKeepPlayerClean(bool enabled) noexcept { m_KeepPlayerCleanRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetAqualung(bool enabled) noexcept { m_AqualungRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetNoGravity(bool enabled) noexcept { m_NoGravityRequested.store(enabled, std::memory_order_release); }
    void PlayerFeatures::SetWaterproof(bool enabled) noexcept { m_WaterproofRequested.store(enabled, std::memory_order_release); }

    void PlayerFeatures::RestorePedState(Game::Ped ped) noexcept
    {
        if (ped == 0 || !m_Player.Exists(ped))
            return;

        if (m_GodModeApplied)
            m_Player.SetInvincible(ped, false);
        if (m_NoRagdollApplied)
            m_Player.SetCanRagdoll(ped, true);
        if (m_SeatBeltApplied)
            m_Player.SetSeatBelt(ped, false);
        if (m_WaterproofApplied)
            m_Player.SetWaterproof(ped, false);
        if (m_AqualungApplied)
            m_Player.SetAqualung(ped, false);
        if (m_OxygenApplied)
            m_Player.SetMaxUnderwaterTime(ped, DefaultUnderwaterSeconds);
        if (m_NoGravityApplied)
            m_Player.SetGravity(ped, true);
    }

    void PlayerFeatures::ClearPedAppliedState() noexcept
    {
        m_GodModeApplied = false;
        m_OxygenApplied = false;
        m_NoRagdollApplied = false;
        m_SeatBeltApplied = false;
        m_AqualungApplied = false;
        m_NoGravityApplied = false;
        m_WaterproofApplied = false;
    }

    void PlayerFeatures::Tick() noexcept
    {
        const bool godMode = m_GodModeRequested.load(std::memory_order_acquire);
        const bool infiniteOxygen = m_InfiniteOxygenRequested.load(std::memory_order_acquire);
        const bool noRagdoll = m_NoRagdollRequested.load(std::memory_order_acquire);
        const bool superJump = m_SuperJumpRequested.load(std::memory_order_acquire);
        const bool seatBelt = m_SeatBeltRequested.load(std::memory_order_acquire);
        const bool noWantedLevel = m_NoWantedLevelRequested.load(std::memory_order_acquire);
        const int wantedLevel = std::clamp(m_WantedLevelRequested.load(std::memory_order_acquire), 0, 5);
        const bool fastRun = m_FastRunRequested.load(std::memory_order_acquire);
        const bool fastSwim = m_FastSwimRequested.load(std::memory_order_acquire);
        const bool keepPlayerClean = m_KeepPlayerCleanRequested.load(std::memory_order_acquire);
        const bool aqualung = m_AqualungRequested.load(std::memory_order_acquire);
        const bool noGravity = m_NoGravityRequested.load(std::memory_order_acquire);
        const bool waterproof = m_WaterproofRequested.load(std::memory_order_acquire);

        const bool effectiveOxygen = infiniteOxygen || aqualung || waterproof;
        const bool effectiveAqualung = aqualung && !waterproof;
        const bool pedStateChanged =
            godMode != m_GodModeApplied ||
            effectiveOxygen != m_OxygenApplied ||
            noRagdoll != m_NoRagdollApplied ||
            seatBelt != m_SeatBeltApplied ||
            effectiveAqualung != m_AqualungApplied ||
            noGravity != m_NoGravityApplied ||
            waterproof != m_WaterproofApplied;
        const bool pedStatePresent =
            godMode || infiniteOxygen || noRagdoll || seatBelt || keepPlayerClean ||
            aqualung || noGravity || waterproof || m_GodModeApplied || m_OxygenApplied ||
            m_NoRagdollApplied || m_SeatBeltApplied || m_AqualungApplied ||
            m_NoGravityApplied || m_WaterproofApplied;

        bool refresh = false;
        if (pedStatePresent)
        {
            if (++m_RefreshTicks >= RefreshTicks)
            {
                m_RefreshTicks = 0;
                refresh = true;
            }
        }
        else
        {
            m_RefreshTicks = 0;
        }

        const bool cleanDue = keepPlayerClean &&
            (refresh || !m_KeepPlayerCleanActive.load(std::memory_order_acquire));
        const bool pedNeedsWork = pedStateChanged || refresh || waterproof || cleanDue;

        if (pedNeedsWork)
        {
            const auto ped = m_Player.LocalPed();
            const bool pedChanged = ped != m_LastPed;
            if (pedChanged)
            {
                RestorePedState(m_LastPed);
                ClearPedAppliedState();
                m_LastPed = ped;
            }

            if (ped == 0 || !m_Player.Exists(ped))
            {
                ClearPedAppliedState();
                m_LastPed = 0;
                m_GodModeActive.store(false, std::memory_order_release);
                m_InfiniteOxygenActive.store(false, std::memory_order_release);
                m_NoRagdollActive.store(false, std::memory_order_release);
                m_SeatBeltActive.store(false, std::memory_order_release);
                m_KeepPlayerCleanActive.store(false, std::memory_order_release);
                m_AqualungActive.store(false, std::memory_order_release);
                m_NoGravityActive.store(false, std::memory_order_release);
                m_WaterproofActive.store(false, std::memory_order_release);
            }
            else
            {
                if (godMode != m_GodModeApplied || (refresh && godMode))
                {
                    m_Player.SetInvincible(ped, godMode);
                    m_GodModeApplied = godMode;
                }

                if (effectiveOxygen != m_OxygenApplied || (refresh && effectiveOxygen))
                {
                    m_Player.SetMaxUnderwaterTime(
                        ped,
                        effectiveOxygen ? UnlimitedUnderwaterSeconds : DefaultUnderwaterSeconds);
                    m_OxygenApplied = effectiveOxygen;
                }

                if (noRagdoll != m_NoRagdollApplied || (refresh && noRagdoll))
                {
                    m_Player.SetCanRagdoll(ped, !noRagdoll);
                    m_NoRagdollApplied = noRagdoll;
                }

                if (seatBelt != m_SeatBeltApplied || (refresh && seatBelt))
                {
                    m_Player.SetSeatBelt(ped, seatBelt);
                    m_SeatBeltApplied = seatBelt;
                }

                if (waterproof || waterproof != m_WaterproofApplied)
                {
                    // Waterproof is frame-maintained because GTA can restore its
                    // swimming/scuba flags while the ped is submerged.
                    m_Player.SetWaterproof(ped, waterproof);
                    m_WaterproofApplied = waterproof;
                }

                if (effectiveAqualung != m_AqualungApplied || (refresh && effectiveAqualung))
                {
                    m_Player.SetAqualung(ped, effectiveAqualung);
                    m_AqualungApplied = effectiveAqualung;
                }

                if (waterproof || noGravity != m_NoGravityApplied || (refresh && noGravity))
                {
                    // Waterproof wants normal gravity so the ped settles on the sea floor.
                    // Explicit No Gravity takes precedence when both toggles are enabled.
                    m_Player.SetGravity(ped, !noGravity);
                    m_NoGravityApplied = noGravity;
                }

                if (keepPlayerClean && (cleanDue || pedChanged))
                    m_Player.Clean(ped);

                m_GodModeActive.store(godMode && m_GodModeApplied, std::memory_order_release);
                m_NoRagdollActive.store(noRagdoll && m_NoRagdollApplied, std::memory_order_release);
                m_SeatBeltActive.store(seatBelt && m_SeatBeltApplied, std::memory_order_release);
                m_KeepPlayerCleanActive.store(keepPlayerClean, std::memory_order_release);
                m_AqualungActive.store(effectiveAqualung && m_AqualungApplied, std::memory_order_release);
                m_NoGravityActive.store(noGravity && m_NoGravityApplied, std::memory_order_release);
                m_WaterproofActive.store(waterproof && m_WaterproofApplied, std::memory_order_release);
            }
        }
        else if (!keepPlayerClean)
        {
            m_KeepPlayerCleanActive.store(false, std::memory_order_release);
        }

        // Infinite Oxygen can become satisfied by Aqualung/Waterproof without a
        // new native call, so derive its active state from the shared oxygen state.
        m_InfiniteOxygenActive.store(
            infiniteOxygen && m_OxygenApplied,
            std::memory_order_release);

        if (!superJump)
            m_SuperJumpActive.store(false, std::memory_order_release);
        if (!noWantedLevel)
            m_NoWantedLevelActive.store(false, std::memory_order_release);

        const bool wantedStateChanged = noWantedLevel != m_NoWantedLevelApplied;
        const bool wantedValueChanged = wantedLevel != m_WantedLevelApplied;
        const bool playerNeedsWork =
            superJump || noWantedLevel ||
            fastRun != m_FastRunApplied ||
            fastSwim != m_FastSwimApplied ||
            wantedStateChanged ||
            (!noWantedLevel && wantedValueChanged);

        if (!playerNeedsWork)
            return;

        const auto player = m_Player.LocalPlayer();

        if (superJump)
        {
            m_Player.SetSuperJump(player);
            m_SuperJumpActive.store(true, std::memory_order_release);
        }

        if (fastRun != m_FastRunApplied)
        {
            m_Player.SetRunMultiplier(
                player,
                fastRun ? FastMovementMultiplier : DefaultMovementMultiplier);
            m_FastRunApplied = fastRun;
            m_FastRunActive.store(fastRun, std::memory_order_release);
        }

        if (fastSwim != m_FastSwimApplied)
        {
            m_Player.SetSwimMultiplier(
                player,
                fastSwim ? FastMovementMultiplier : DefaultMovementMultiplier);
            m_FastSwimApplied = fastSwim;
            m_FastSwimActive.store(fastSwim, std::memory_order_release);
        }

        if (noWantedLevel)
        {
            if (m_Player.WantedLevel(player) != 0)
                m_Player.ClearWantedLevel(player);
            m_NoWantedLevelActive.store(true, std::memory_order_release);
        }
        else if (wantedStateChanged || wantedValueChanged)
        {
            m_Player.SetWantedLevel(player, wantedLevel);
            m_WantedLevelApplied = wantedLevel;
        }

        m_NoWantedLevelApplied = noWantedLevel;
    }

    void PlayerFeatures::Reset() noexcept
    {
        m_GodModeRequested.store(false, std::memory_order_release);
        m_InfiniteOxygenRequested.store(false, std::memory_order_release);
        m_NoRagdollRequested.store(false, std::memory_order_release);
        m_SuperJumpRequested.store(false, std::memory_order_release);
        m_SeatBeltRequested.store(false, std::memory_order_release);
        m_NoWantedLevelRequested.store(false, std::memory_order_release);
        m_WantedLevelRequested.store(0, std::memory_order_release);
        m_FastRunRequested.store(false, std::memory_order_release);
        m_FastSwimRequested.store(false, std::memory_order_release);
        m_KeepPlayerCleanRequested.store(false, std::memory_order_release);
        m_AqualungRequested.store(false, std::memory_order_release);
        m_NoGravityRequested.store(false, std::memory_order_release);
        m_WaterproofRequested.store(false, std::memory_order_release);

        m_GodModeActive.store(false, std::memory_order_release);
        m_InfiniteOxygenActive.store(false, std::memory_order_release);
        m_NoRagdollActive.store(false, std::memory_order_release);
        m_SuperJumpActive.store(false, std::memory_order_release);
        m_SeatBeltActive.store(false, std::memory_order_release);
        m_NoWantedLevelActive.store(false, std::memory_order_release);
        m_FastRunActive.store(false, std::memory_order_release);
        m_FastSwimActive.store(false, std::memory_order_release);
        m_KeepPlayerCleanActive.store(false, std::memory_order_release);
        m_AqualungActive.store(false, std::memory_order_release);
        m_NoGravityActive.store(false, std::memory_order_release);
        m_WaterproofActive.store(false, std::memory_order_release);

        ClearPedAppliedState();
        m_NoWantedLevelApplied = false;
        m_FastRunApplied = false;
        m_FastSwimApplied = false;
        m_WantedLevelApplied = 0;
        m_RefreshTicks = 0;
        m_LastPed = 0;
    }

    PlayerFeatureSnapshot PlayerFeatures::Snapshot() const noexcept
    {
        return {
            .godMode = {
                .requested = m_GodModeRequested.load(std::memory_order_acquire),
                .active = m_GodModeActive.load(std::memory_order_acquire),
            },
            .infiniteOxygen = {
                .requested = m_InfiniteOxygenRequested.load(std::memory_order_acquire),
                .active = m_InfiniteOxygenActive.load(std::memory_order_acquire),
            },
            .noRagdoll = {
                .requested = m_NoRagdollRequested.load(std::memory_order_acquire),
                .active = m_NoRagdollActive.load(std::memory_order_acquire),
            },
            .superJump = {
                .requested = m_SuperJumpRequested.load(std::memory_order_acquire),
                .active = m_SuperJumpActive.load(std::memory_order_acquire),
            },
            .seatBelt = {
                .requested = m_SeatBeltRequested.load(std::memory_order_acquire),
                .active = m_SeatBeltActive.load(std::memory_order_acquire),
            },
            .noWantedLevel = {
                .requested = m_NoWantedLevelRequested.load(std::memory_order_acquire),
                .active = m_NoWantedLevelActive.load(std::memory_order_acquire),
            },
            .wantedLevel = std::clamp(m_WantedLevelRequested.load(std::memory_order_acquire), 0, 5),
            .fastRun = {
                .requested = m_FastRunRequested.load(std::memory_order_acquire),
                .active = m_FastRunActive.load(std::memory_order_acquire),
            },
            .fastSwim = {
                .requested = m_FastSwimRequested.load(std::memory_order_acquire),
                .active = m_FastSwimActive.load(std::memory_order_acquire),
            },
            .keepPlayerClean = {
                .requested = m_KeepPlayerCleanRequested.load(std::memory_order_acquire),
                .active = m_KeepPlayerCleanActive.load(std::memory_order_acquire),
            },
            .aqualung = {
                .requested = m_AqualungRequested.load(std::memory_order_acquire),
                .active = m_AqualungActive.load(std::memory_order_acquire),
            },
            .noGravity = {
                .requested = m_NoGravityRequested.load(std::memory_order_acquire),
                .active = m_NoGravityActive.load(std::memory_order_acquire),
            },
            .waterproof = {
                .requested = m_WaterproofRequested.load(std::memory_order_acquire),
                .active = m_WaterproofActive.load(std::memory_order_acquire),
            },
        };
    }
}
