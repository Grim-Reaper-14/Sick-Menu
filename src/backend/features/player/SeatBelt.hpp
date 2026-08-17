#pragma once

#include "game/services/PlayerService.hpp"

namespace Sick::Backend::Features::Player
{
    struct SeatBelt final
    {
        static void Apply(Game::PlayerService& service, Game::Ped ped, bool enabled) noexcept
        {
            service.SetSeatBelt(ped, enabled);
        }
    };
}
