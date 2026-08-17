#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct FastRun final
    {
        static constexpr float DefaultMultiplier = 1.0F;
        static constexpr float FastMultiplier = 1.49F;

        static void Apply(Game::PlayerService& service, Game::Player player, bool enabled) noexcept
        {
            service.SetRunMultiplier(player, enabled ? FastMultiplier : DefaultMultiplier);
        }
    };
}
