#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct InfiniteOxygen final
    {
        static constexpr float DefaultSeconds = 10.0F;
        static constexpr float UnlimitedSeconds = 1000000.0F;

        static void Apply(Game::PlayerService& service, Game::Ped ped, bool enabled) noexcept
        {
            service.SetMaxUnderwaterTime(ped, enabled ? UnlimitedSeconds : DefaultSeconds);
        }
    };
}
