#include "Reaper.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace
{
    constexpr Reaper::NativeHash MappedPlayerPedId = 0xF00DBAAD00000002ULL;

    int g_ProviderCalls = 0;
    Reaper::Entity g_LastEntity = 0;
    bool g_LastInvincible = false;
    std::array<std::int64_t, 64> g_Globals{};

    void PlayerPedHandler(Reaper::Native::Context* context)
    {
        context->SetResult<Reaper::Ped>(321);
    }

    void PlayerIdHandler(Reaper::Native::Context* context)
    {
        context->SetResult<Reaper::Player>(8);
    }

    void EntityExistsHandler(Reaper::Native::Context* context)
    {
        context->SetResult<bool>(context->GetArgument<Reaper::Entity>(0) == 321);
    }

    void InvincibleHandler(Reaper::Native::Context* context)
    {
        g_LastEntity = context->GetArgument<Reaper::Entity>(0);
        g_LastInvincible = context->GetArgument<bool>(1);
    }

    Reaper::Native::Handler ProvideNative(Reaper::NativeHash hash, Reaper::Enhanced::BuildId build)
    {
        assert(build == 9001);
        ++g_ProviderCalls;

        switch (hash)
        {
        case MappedPlayerPedId:
            return &PlayerPedHandler;
        case Reaper::Native::Hashes::PLAYER_ID:
            return &PlayerIdHandler;
        case Reaper::Native::Hashes::DOES_ENTITY_EXIST:
            return &EntityExistsHandler;
        case Reaper::Native::Hashes::SET_ENTITY_INVINCIBLE:
            return &InvincibleHandler;
        default:
            return nullptr;
        }
    }

    void* ResolveGlobal(std::size_t index)
    {
        return index < g_Globals.size() ? &g_Globals[index] : nullptr;
    }
}

int main()
{
    auto& crossmap = Reaper::Native::Crossmap::Get();
    crossmap.Clear();
    assert(crossmap.Register(9001, Reaper::Native::Hashes::PLAYER_PED_ID, MappedPlayerPedId));

    assert(Reaper::Enhanced::Game::InitializeIndexed(9001, &ProvideNative));
    assert(Reaper::Enhanced::Game::Ready());
    assert(g_ProviderCalls == static_cast<int>(Sick::Game::Natives::NativeCount));
    assert(Reaper::Native::HandlerTable::Get().ResolvedCount() == Sick::Game::Natives::NativeCount);

    const Reaper::Ped ped = Reaper::PLAYER::PLAYER_PED_ID();
    assert(ped == 321);
    assert(Reaper::PLAYER::PLAYER_ID() == 8);
    assert(Reaper::ENTITY::DOES_ENTITY_EXIST(ped));

    Reaper::ENTITY::SET_ENTITY_INVINCIBLE(ped, true);
    assert(g_LastEntity == 321);
    assert(g_LastInvincible);

    const auto rawPed = Reaper::Native::Invoker::Call<Reaper::Ped>(Reaper::Native::Hashes::PLAYER_PED_ID);
    assert(rawPed == 321);
    assert(Reaper::Native::Invoker::Call<Reaper::Ped>(MappedPlayerPedId) == 321);
    assert(g_ProviderCalls == static_cast<int>(Sick::Game::Natives::NativeCount));

    Reaper::Enhanced::Game::BindScriptGlobalResolver(&ResolveGlobal);
    Reaper::ScriptGlobal root{10};
    assert(root.CanAccess());
    root.As<std::int64_t&>() = 44;
    assert(g_Globals[10] == 44);

    auto child = root.At(2);
    child.As<std::int64_t&>() = 55;
    assert(child.Index() == 12);
    assert(g_Globals[12] == 55);

    auto arrayItem = root.At(3, 4);
    assert(arrayItem.Index() == 23);
    arrayItem.As<std::int64_t&>() = 66;
    assert(g_Globals[23] == 66);

    const auto stats = Reaper::Native::System::Stats();
    assert(stats.calls >= 5);
    assert(stats.failed == 0);

    Reaper::Enhanced::Game::Shutdown();
    assert(!Reaper::Enhanced::Game::Ready());
    assert(!Reaper::ScriptGlobal::ResolverReady());
    crossmap.Clear();
    return 0;
}
