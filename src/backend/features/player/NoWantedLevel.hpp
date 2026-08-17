#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct NoWantedLevel final
    {
        static void Apply(Game::PlayerService& service, Game::Player player) noexcept
        {
            if (service.WantedLevel(player) != 0)
                service.ClearWantedLevel(player);
        }
    };
}
