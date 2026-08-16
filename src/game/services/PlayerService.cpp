#include "PlayerService.hpp"
#include "game/natives/Natives.hpp"

namespace Sick::Game
{
    Ped PlayerService::LocalPed() const noexcept
    {
        return Natives::PLAYER::PLAYER_PED_ID();
    }

    bool PlayerService::Exists(Ped ped) const noexcept
    {
        return ped != 0 && Natives::ENTITY::DOES_ENTITY_EXIST(ped);
    }

    void PlayerService::SetInvincible(Ped ped, bool enabled) const noexcept
    {
        if (ped != 0)
            Natives::ENTITY::SET_ENTITY_INVINCIBLE(ped, enabled);
    }
}
