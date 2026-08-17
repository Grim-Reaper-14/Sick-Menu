#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct GodMode final
    {
        static void Apply(Game::PlayerService& service, Game::Ped ped, bool enabled) noexcept
        {
            service.SetInvincible(ped, enabled);
        }
    };
}
