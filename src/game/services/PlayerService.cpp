#include "PlayerService.hpp"
#include "game/natives/Natives.hpp"

namespace Sick::Game
{
    Ped PlayerService::LocalPed() const noexcept
    {
        return Natives::PLAYER::PLAYER_PED_ID();
    }

    bool PlayerService::Exists() const noexcept
    {
        const auto ped = LocalPed();
        return ped != 0 && Natives::ENTITY::DOES_ENTITY_EXIST(ped);
    }

    void PlayerService::SetInvincible(bool enabled) const noexcept
    {
        const auto ped = LocalPed();
        if (ped == 0 || !Natives::ENTITY::DOES_ENTITY_EXIST(ped))
            return;

        Natives::ENTITY::SET_ENTITY_INVINCIBLE(ped, enabled);
    }
}
