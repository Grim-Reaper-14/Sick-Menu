#include "Reaper.hpp"
#include "game/natives/generated/EnhancedNativeHashes.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace
{
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

    void NoopHandler(Reaper::Native::Context*)
    {
    }

    Reaper::Native::Handler ProvidePartialNative(Reaper::NativeHash hash, Reaper::Enhanced::BuildId build)
    {
        assert(build == 9001);
        ++g_ProviderCalls;

        switch (hash)
        {
        case Reaper::Native::Hashes::PLAYER_PED_ID:
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

    Reaper::Native::Handler ProvideCompleteNative(Reaper::NativeHash hash, Reaper::Enhanced::BuildId build)
    {
        if (const auto handler = ProvidePartialNative(hash, build))
            return handler;
        return &NoopHandler;
    }

    void* ResolveGlobal(std::size_t index)
    {
        return index < g_Globals.size() ? &g_Globals[index] : nullptr;
    }
}

int main()
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;
    namespace Generated = Sick::Game::Natives::Generated;

    static_assert(Generated::EnhancedNativeHashes.size() == NativeCount);
    for (const auto hash : Generated::EnhancedNativeHashes)
        assert(hash != 0);
    assert(Generated::EnhancedHashFor(NativeIndex::PLAYER_PED_ID) == 0x4A8C381C258A124DULL);
    assert(Generated::EnhancedHashFor(NativeIndex::SET_SUPER_JUMP_THIS_FRAME) == 0x353BF8D85390AA39ULL);
    assert(Generated::EnhancedHashFor(NativeIndex::SET_VEHICLE_ENGINE_ON) == 0xC229299217554C78ULL);
    assert(Generated::EnhancedHashFor(NativeIndex::REQUEST_MODEL) == 0xEC9DAA34BBB4658CULL);
    assert(Generated::EnhancedHashFor(NativeIndex::CREATE_VEHICLE) == 0x5779387E956077A6ULL);
    assert(Generated::EnhancedHashFor(NativeIndex::SET_PED_INTO_VEHICLE) == 0x73CAFD2038E812B3ULL);

    g_ProviderCalls = 0;
    assert(!Reaper::Enhanced::Game::InitializeIndexed(9001, &ProvidePartialNative));
    assert(!Reaper::Enhanced::Game::Ready());
    assert(g_ProviderCalls == static_cast<int>(NativeCount));

    g_ProviderCalls = 0;
    assert(Reaper::Enhanced::Game::InitializeIndexed(9001, &ProvideCompleteNative));
    assert(Reaper::Enhanced::Game::Ready());
    assert(g_ProviderCalls == static_cast<int>(NativeCount));
    assert(Reaper::Native::HandlerTable::Get().ResolvedCount() == NativeCount);

    const Reaper::Ped ped = Reaper::PLAYER::PLAYER_PED_ID();
    assert(ped == 321);
    assert(Reaper::PLAYER::PLAYER_ID() == 8);
    assert(Reaper::ENTITY::DOES_ENTITY_EXIST(ped));

    Reaper::ENTITY::SET_ENTITY_INVINCIBLE(ped, true);
    assert(g_LastEntity == 321);
    assert(g_LastInvincible);

    const auto rawPed = Reaper::Native::Invoker::Call<Reaper::Ped>(Reaper::Native::Hashes::PLAYER_PED_ID);
    assert(rawPed == 321);
    assert(g_ProviderCalls == static_cast<int>(NativeCount));

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
    return 0;
}
