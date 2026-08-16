#include "game/enhanced/EnhancedGame.hpp"
#include "game/natives/NativeBackend.hpp"
#include "game/scheduler/GameScheduler.hpp"
#include "game/services/PlayerService.hpp"

#include <cassert>

namespace
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;
    using namespace Sick::Game::Enhanced;

    int g_Lookups = 0;
    Entity g_LastEntity = 0;
    bool g_LastInvincible = false;

    void PlayerPedHandler(NativeCallContext* context)
    {
        context->SetResult<Ped>(99);
    }

    void EntityExistsHandler(NativeCallContext* context)
    {
        context->SetResult<bool>(context->GetArgument<Entity>(0) == 99);
    }

    void InvincibleHandler(NativeCallContext* context)
    {
        g_LastEntity = context->GetArgument<Entity>(0);
        g_LastInvincible = context->GetArgument<bool>(1);
    }

    NativeHandler Lookup(NativeHash hash, BuildId build)
    {
        assert(build == 1234);
        ++g_Lookups;

        switch (hash)
        {
        case Hashes::PLAYER_PED_ID:
            return &PlayerPedHandler;
        case Hashes::DOES_ENTITY_EXIST:
            return &EntityExistsHandler;
        case Hashes::SET_ENTITY_INVINCIBLE:
            return &InvincibleHandler;
        default:
            return nullptr;
        }
    }
}

int main()
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;
    using namespace Sick::Game::Enhanced;

    assert(EnhancedGame::Initialize(1234, &Lookup));
    assert(EnhancedGame::Ready());
    assert(BuildManager::Current() == 1234);
    assert(NativeRegistry::Get().Size() == 4);

    const auto playerPed = NativeRegistry::Get().Find("PLAYER_PED_ID");
    assert(playerPed.has_value());
    assert(playerPed->hash == Hashes::PLAYER_PED_ID);

    PlayerService player;
    assert(player.LocalPed() == 99);
    assert(player.Exists());
    player.SetInvincible(true);
    EnhancedGame::Tick();
    assert(g_LastEntity == 99);
    assert(g_LastInvincible);

    bool scheduled = false;
    GameScheduler::Get().Queue([&scheduled]() { scheduled = true; });
    assert(GameScheduler::Get().Pending() == 1);
    EnhancedGame::Tick();
    assert(scheduled);
    assert(GameScheduler::Get().Pending() == 0);

    player.SetInvincible(false);
    EnhancedGame::Tick();
    assert(!g_LastInvincible);

    const auto stats = NativeSystem::Stats();
    assert(stats.calls >= 4);
    assert(stats.failed == 0);
    assert(g_Lookups == 3);

    EnhancedGame::Shutdown();
    assert(!EnhancedGame::Ready());
    return 0;
}
