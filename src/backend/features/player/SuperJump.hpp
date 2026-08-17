#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct SuperJump final
    {
        static void Apply(Game::PlayerService& service, Game::Player player) noexcept
        {
            service.SetSuperJump(player);
        }
    };
}
