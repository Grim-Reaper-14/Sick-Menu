#pragma once

#include "backend/BackendTypes.hpp"
#include "PlayerFeatures.hpp"

namespace Sick::Backend::Features
{
    class FeatureManager final
    {
    public:
        void SetGodMode(bool enabled) noexcept;
        void ApplyProfile(const FeatureProfile& profile) noexcept;
        void Tick(bool nativeReady) noexcept;
        void Reset() noexcept;

        [[nodiscard]] PlayerFeatureSnapshot PlayerSnapshot() const noexcept;
        [[nodiscard]] FeatureProfile Profile() const noexcept;

    private:
        PlayerFeatures m_Player;
    };
}
