#include "backend/features/VehicleFeatures.hpp"
#include "game/natives/NativeBackend.hpp"

#include <cstddef>
#include <iostream>

namespace
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;

    std::size_t g_HandlerCalls{};
    bool g_Invincible{};
    bool g_Gravity{true};
    bool g_Collision{true};
    bool g_ArgumentsValid{true};
    std::size_t g_EngineOnCalls{};
    std::size_t g_RepairOps{};
    std::size_t g_CleanOps{};
    std::size_t g_GroundOps{};

    bool Check(bool condition, const char* expression, int line)
    {
        if (condition)
            return true;
        std::cerr << "check failed at line " << line << ": " << expression << '\n';
        return false;
    }

#define CHECK(expression) \
    do \
    { \
        if (!Check(static_cast<bool>(expression), #expression, __LINE__)) \
            return 1; \
    } while (false)

    void PlayerPedHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        context->SetResult<Ped>(99);
    }

    void VehiclePedHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        g_ArgumentsValid = g_ArgumentsValid &&
            context->GetArgument<Ped>(0) == 99 &&
            !context->GetArgument<bool>(1);
        context->SetResult<Vehicle>(777);
    }

    void ExistsHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        context->SetResult<bool>(context->GetArgument<Entity>(0) == 777);
    }

    void InvincibleHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        g_ArgumentsValid = g_ArgumentsValid && context->GetArgument<Entity>(0) == 777;
        g_Invincible = context->GetArgument<bool>(1);
    }

    void GravityHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        g_Gravity = context->GetArgument<bool>(1);
    }

    void CollisionHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        g_Collision = context->GetArgument<bool>(1);
        g_ArgumentsValid = g_ArgumentsValid && context->GetArgument<bool>(2);
    }

    void EngineHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        ++g_EngineOnCalls;
        g_ArgumentsValid = g_ArgumentsValid &&
            context->GetArgument<bool>(1) &&
            context->GetArgument<bool>(2);
    }

    void RepairHandler(NativeCallContext*)
    {
        ++g_HandlerCalls;
        ++g_RepairOps;
    }

    void CleanHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        ++g_CleanOps;
        g_ArgumentsValid = g_ArgumentsValid && context->GetArgument<float>(1) == 0.0F;
    }

    void GroundHandler(NativeCallContext* context)
    {
        ++g_HandlerCalls;
        ++g_GroundOps;
        context->SetResult<bool>(true);
    }

    NativeHandler Lookup(NativeHash hash)
    {
        switch (hash)
        {
        case Hashes::PLAYER_PED_ID: return &PlayerPedHandler;
        case Hashes::GET_VEHICLE_PED_IS_IN: return &VehiclePedHandler;
        case Hashes::DOES_ENTITY_EXIST: return &ExistsHandler;
        case Hashes::SET_ENTITY_INVINCIBLE: return &InvincibleHandler;
        case Hashes::SET_ENTITY_HAS_GRAVITY: return &GravityHandler;
        case Hashes::SET_ENTITY_COLLISION: return &CollisionHandler;
        case Hashes::SET_VEHICLE_ENGINE_ON: return &EngineHandler;
        case Hashes::SET_VEHICLE_FIXED:
        case Hashes::SET_VEHICLE_DEFORMATION_FIXED:
        case Hashes::SET_VEHICLE_ENGINE_HEALTH:
        case Hashes::SET_VEHICLE_BODY_HEALTH:
        case Hashes::SET_VEHICLE_PETROL_TANK_HEALTH:
            return &RepairHandler;
        case Hashes::SET_VEHICLE_DIRT_LEVEL: return &CleanHandler;
        case Hashes::SET_VEHICLE_ON_GROUND_PROPERLY: return &GroundHandler;
        default: return nullptr;
        }
    }
}

int main()
{
    auto& resolver = Sick::Game::Natives::NativeResolver::Get();
    resolver.Reset();
    resolver.SetResolver(&Lookup);

    Sick::Backend::Features::VehicleFeatures features;
    const auto beforeIdle = g_HandlerCalls;
    features.Tick();
    CHECK(g_HandlerCalls == beforeIdle);

    features.SetGodMode(true);
    features.SetAutoRepair(true);
    features.SetKeepClean(true);
    features.SetEngineAlwaysOn(true);
    features.SetNoGravity(true);
    features.SetNoCollision(true);
    features.RequestRepair();
    features.RequestClean();
    features.RequestPutOnGround();
    features.Tick();

    const auto active = features.Snapshot();
    CHECK(active.godMode.active);
    CHECK(active.autoRepair.active);
    CHECK(active.keepClean.active);
    CHECK(active.engineAlwaysOn.active);
    CHECK(active.noGravity.active);
    CHECK(active.noCollision.active);
    CHECK(g_Invincible);
    CHECK(!g_Gravity);
    CHECK(!g_Collision);
    CHECK(g_EngineOnCalls >= 1);
    CHECK(g_RepairOps >= 5);
    CHECK(g_CleanOps >= 1);
    CHECK(g_GroundOps == 1);
    CHECK(g_ArgumentsValid);

    const auto afterApply = g_HandlerCalls;
    features.Tick();
    CHECK(g_HandlerCalls == afterApply);

    features.SetGodMode(false);
    features.SetAutoRepair(false);
    features.SetKeepClean(false);
    features.SetEngineAlwaysOn(false);
    features.SetNoGravity(false);
    features.SetNoCollision(false);
    features.Tick();
    CHECK(!g_Invincible);
    CHECK(g_Gravity);
    CHECK(g_Collision);
    CHECK(g_ArgumentsValid);

    const auto afterDisable = g_HandlerCalls;
    features.Tick();
    CHECK(g_HandlerCalls == afterDisable);

    resolver.Reset();
    return 0;
}
