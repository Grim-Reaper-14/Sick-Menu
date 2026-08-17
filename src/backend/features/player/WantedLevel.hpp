#pragma once

#include "game/services/PlayerService.hpp"

#include <algorithm>

namespace Sick::Backend::Features::Player
{
    struct WantedLevel final
    {
        [[nodiscard]] static int Normalize(int level) noexcept
        {
            return std::clamp(level, 0, 5);
        }

        static void Apply(Game::PlayerService& service, Game::Player player, int level) noexcept
        {
            service.SetWantedLevel(player, Normalize(level));
        }
    };
}
