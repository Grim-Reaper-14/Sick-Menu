#include "game/natives/NativeBackend.hpp"

#include <cassert>
#include <cstdint>

namespace
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;

    int g_ResolveCalls = 0;
    Entity g_LastEntity = 0;
    bool g_LastInvincible = false;

    void PlayerPedHandler(NativeCallContext* context)
    {
        context->SetResult<Ped>(1337);
    }

    void PlayerIdHandler(NativeCallContext* context)
    {
        context->SetResult<Player>(7);
    }

    void SetInvincibleHandler(NativeCallContext* context)
    {
        assert(context->ArgumentCount() == 3);
        g_LastEntity = context->GetArgument<Entity>(0);
        g_LastInvincible = context->GetArgument<bool>(1);
        assert(!context->GetArgument<bool>(2));
    }

    void EntityCoordsHandler(NativeCallContext* context)
    {
        assert(context->ArgumentCount() == 2);
        assert(context->GetArgument<Entity>(0) == 42);
        assert(!context->GetArgument<bool>(1));
        context->SetResult<ScriptVector>({1.0F, 2.0F, 3.0F});
    }

    NativeHandler ResolveForTest(NativeHash hash)
    {
        ++g_ResolveCalls;

        switch (hash)
        {
        case Hashes::PLAYER_PED_ID:
            return &PlayerPedHandler;
        case Hashes::PLAYER_ID:
            return &PlayerIdHandler;
        case Hashes::SET_ENTITY_INVINCIBLE:
            return &SetInvincibleHandler;
        case Hashes::GET_ENTITY_COORDS:
            return &EntityCoordsHandler;
        default:
            return nullptr;
        }
    }
}

int main()
{
    auto& resolver = Sick::Game::Natives::NativeResolver::Get();
    resolver.Reset();
    resolver.SetResolver(&ResolveForTest);

    assert(Sick::Game::Natives::PLAYER::PLAYER_PED_ID() == 1337);
    assert(Sick::Game::Natives::PLAYER::PLAYER_PED_ID() == 1337);
    assert(g_ResolveCalls == 1);

    assert(Sick::Game::Natives::PLAYER::PLAYER_ID() == 7);
    assert(g_ResolveCalls == 2);

    Sick::Game::Natives::ENTITY::SET_ENTITY_INVINCIBLE(42, true);
    assert(g_LastEntity == 42);
    assert(g_LastInvincible);
    assert(g_ResolveCalls == 3);

    const auto coords = Sick::Game::Natives::ENTITY::GET_ENTITY_COORDS(42, false);
    assert(coords.x == 1.0F);
    assert(coords.y == 2.0F);
    assert(coords.z == 3.0F);
    assert(g_ResolveCalls == 4);

    constexpr Sick::Game::NativeHash missing = 0x1111222233334444ULL;
    const auto missingResult = Sick::Game::Natives::NativeInvoker::TryCall<std::int32_t>(missing);
    assert(!missingResult.has_value());
    assert(g_ResolveCalls == 5);

    resolver.RegisterOverride(Sick::Game::Natives::Hashes::PLAYER_ID, &PlayerPedHandler);
    assert(Sick::Game::Natives::PLAYER::PLAYER_ID() == 1337);

    resolver.Reset();
    return 0;
}
