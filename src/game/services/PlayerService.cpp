#include "PlayerService.hpp"
#include "game/natives/Natives.hpp"

#include <atomic>

namespace
{
    std::atomic_bool g_InvincibleRequested{false};
    Sick::Game::Ped g_InvinciblePed{};
}

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
        g_InvincibleRequested.store(enabled, std::memory_order_release);
    }

    void PlayerService::Tick() const noexcept
    {
        const bool requested = g_InvincibleRequested.load(std::memory_order_acquire);
        if (!requested && g_InvinciblePed == 0)
            return;

        const auto ped = LocalPed();

        if (g_InvinciblePed != 0 && (g_InvinciblePed != ped || !requested))
        {
            if (Natives::ENTITY::DOES_ENTITY_EXIST(g_InvinciblePed))
                Natives::ENTITY::SET_ENTITY_INVINCIBLE(g_InvinciblePed, false);
            g_InvinciblePed = 0;
        }

        if (!requested || ped == 0 || !Natives::ENTITY::DOES_ENTITY_EXIST(ped))
            return;

        Natives::ENTITY::SET_ENTITY_INVINCIBLE(ped, true);
        g_InvinciblePed = ped;
    }

    void PlayerService::Reset() const noexcept
    {
        g_InvincibleRequested.store(false, std::memory_order_release);
        g_InvinciblePed = 0;
    }
}
