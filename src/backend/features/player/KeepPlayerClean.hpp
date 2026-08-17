#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct KeepPlayerClean final
    {
        static void Apply(Game::PlayerService& service, Game::Ped ped) noexcept
        {
            service.Clean(ped);
        }
    };
}
