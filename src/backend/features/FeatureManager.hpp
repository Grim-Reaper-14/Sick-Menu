#pragma once

#include "backend/BackendTypes.hpp"
#include "PlayerFeatures.hpp"

namespace Sick::Backend::Features
{
    class FeatureManager final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void SetInfiniteOxygen(bool enabled) noexcept;
        void SetNoRagdoll(bool enabled) noexcept;
        void SetSuperJump(bool enabled) noexcept;
        void SetSeatBelt(bool enabled) noexcept;
        void SetNoWantedLevel(bool enabled) noexcept;
        void SetWantedLevel(int level) noexcept;
        void SetFastRun(bool enabled) noexcept;
        void SetFastSwim(bool enabled) noexcept;
        void SetKeepPlayerClean(bool enabled) noexcept;
        void SetAqualung(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetWaterproof(bool enabled) noexcept;

        void ApplyProfile(const FeatureProfile& profile) noexcept;
        void Tick(bool nativeReady) noexcept;
        void Reset() noexcept;

        [[nodiscard]] PlayerFeatureSnapshot PlayerSnapshot() const noexcept;
        [[nodiscard]] FeatureProfile Profile() const noexcept;

    private:
        PlayerFeatures m_Player;
    };
}
