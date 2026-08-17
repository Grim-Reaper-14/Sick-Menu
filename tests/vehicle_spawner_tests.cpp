#include "backend/features/online_vehicle_spawner/VehicleSpawner.hpp"
#include "backend/tasking/GameFiberScheduler.hpp"
#include "backend/tasking/TaskAffinity.hpp"
#include "game/natives/NativeBackend.hpp"

#include <cassert>
#include <cstdint>

namespace
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;
    using Sick::Backend::Features::OnlineVehicleSpawner::VehicleSpawner;
    using Sick::Backend::Tasking::GameFiberScheduler;
    using Sick::Backend::Tasking::ScopedTaskAffinity;
    using Sick::Backend::Tasking::TaskAffinity;

    constexpr Hash AdderHash = 0xB779A091U;

    bool g_ModelInCdimage = true;
    bool g_ModelIsVehicle = true;
    int g_RequestCalls{};
    int g_LoadChecks{};
    int g_ReleaseCalls{};
    bool g_Warped{};
    Vehicle g_CreatedVehicle{};

    void PlayerPedHandler(NativeCallContext* context)
    {
        context->SetResult<Ped>(99);
    }

    void EntityExistsHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Entity>(0) == 99);
        context->SetResult<bool>(true);
    }

    void ModelInCdimageHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Hash>(0) == AdderHash);
        context->SetResult<bool>(g_ModelInCdimage);
    }

    void ModelIsVehicleHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Hash>(0) == AdderHash);
        context->SetResult<bool>(g_ModelIsVehicle);
    }

    void RequestModelHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Hash>(0) == AdderHash);
        ++g_RequestCalls;
    }

    void HasModelLoadedHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Hash>(0) == AdderHash);
        context->SetResult<bool>(++g_LoadChecks >= 2);
    }

    void EntityCoordsHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Entity>(0) == 99);
        assert(!context->GetArgument<bool>(1));
        context->SetResult<ScriptVector>({10.0F, 20.0F, 30.0F});
    }

    void EntityHeadingHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Entity>(0) == 99);
        context->SetResult<float>(90.0F);
    }

    void CreateVehicleHandler(NativeCallContext* context)
    {
        assert(context->ArgumentCount() == 7);
        assert(context->GetArgument<Hash>(0) == AdderHash);
        assert(context->GetArgument<float>(1) == 10.0F);
        assert(context->GetArgument<float>(2) == 20.0F);
        assert(context->GetArgument<float>(3) == 30.0F);
        assert(context->GetArgument<float>(4) == 90.0F);
        assert(context->GetArgument<bool>(5));
        assert(!context->GetArgument<bool>(6));
        g_CreatedVehicle = 123;
        context->SetResult<Vehicle>(g_CreatedVehicle);
    }

    void SetPedIntoVehicleHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Ped>(0) == 99);
        assert(context->GetArgument<Vehicle>(1) == 123);
        assert(context->GetArgument<int>(2) == -1);
        g_Warped = true;
    }

    void ReleaseModelHandler(NativeCallContext* context)
    {
        assert(context->GetArgument<Hash>(0) == AdderHash);
        ++g_ReleaseCalls;
    }

    NativeHandler ResolveForTest(NativeHash hash)
    {
        switch (hash)
        {
        case Hashes::PLAYER_PED_ID: return &PlayerPedHandler;
        case Hashes::DOES_ENTITY_EXIST: return &EntityExistsHandler;
        case Hashes::IS_MODEL_IN_CDIMAGE: return &ModelInCdimageHandler;
        case Hashes::IS_MODEL_A_VEHICLE: return &ModelIsVehicleHandler;
        case Hashes::REQUEST_MODEL: return &RequestModelHandler;
        case Hashes::HAS_MODEL_LOADED: return &HasModelLoadedHandler;
        case Hashes::GET_ENTITY_COORDS: return &EntityCoordsHandler;
        case Hashes::GET_ENTITY_HEADING: return &EntityHeadingHandler;
        case Hashes::CREATE_VEHICLE: return &CreateVehicleHandler;
        case Hashes::SET_PED_INTO_VEHICLE: return &SetPedIntoVehicleHandler;
        case Hashes::SET_MODEL_AS_NO_LONGER_NEEDED: return &ReleaseModelHandler;
        default: return nullptr;
        }
    }

    void Drain(GameFiberScheduler& scheduler)
    {
        ScopedTaskAffinity gameAffinity{TaskAffinity::Game};
        for (int attempt = 0; attempt < 8 && scheduler.Pending() != 0; ++attempt)
            static_cast<void>(scheduler.Tick(1, 50000));
        assert(scheduler.Pending() == 0);
    }
}

int main()
{
    assert(VehicleSpawner::HashModelName("adder") == AdderHash);
    assert(VehicleSpawner::HashModelName("ADDER") == AdderHash);

    auto& resolver = NativeResolver::Get();
    resolver.Reset();
    resolver.SetResolver(&ResolveForTest);

    VehicleSpawner spawner;
    GameFiberScheduler scheduler;

    assert(spawner.TryQueue(AdderHash));
    assert(!spawner.TryQueue(AdderHash));
    assert(scheduler.Queue([&spawner]() { spawner.Spawn(AdderHash, true); }));
    Drain(scheduler);

    const auto spawned = spawner.Snapshot();
    assert(spawned.state == Sick::Backend::VehicleSpawnerState::Spawned);
    assert(spawned.modelHash == AdderHash);
    assert(!spawned.busy);
    assert(g_RequestCalls == 1);
    assert(g_LoadChecks >= 2);
    assert(g_ReleaseCalls == 1);
    assert(g_CreatedVehicle == 123);
    assert(g_Warped);

    g_ModelInCdimage = false;
    const auto requestsBeforeInvalid = g_RequestCalls;
    assert(spawner.TryQueue(AdderHash));
    assert(scheduler.Queue([&spawner]() { spawner.Spawn(AdderHash, false); }));
    Drain(scheduler);

    const auto invalid = spawner.Snapshot();
    assert(invalid.state == Sick::Backend::VehicleSpawnerState::InvalidModel);
    assert(!invalid.busy);
    assert(g_RequestCalls == requestsBeforeInvalid);

    resolver.Reset();
    return 0;
}
