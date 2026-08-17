#include "backend/features/HandlingFeatures.hpp"
#include "game/enhanced/BuildManager.hpp"
#include "game/handling/HandlingBackend.hpp"
#include "game/natives/NativeBackend.hpp"
#include "game/natives/Natives.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace
{
    using namespace Sick::Game;
    using namespace Sick::Game::Natives;

    Sick::Handling::Values g_LiveValues{};
    Sick::Handling::Values g_OriginalValues{};
    std::size_t g_NativeCalls{};
    std::size_t g_ReadCalls{};
    std::size_t g_WriteCalls{};

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

    bool NearlyEqual(float left, float right) noexcept
    {
        return std::fabs(left - right) < 0.0001F;
    }

    void PlayerPedHandler(NativeCallContext* context)
    {
        ++g_NativeCalls;
        context->SetResult<Ped>(99);
    }

    void VehiclePedHandler(NativeCallContext* context)
    {
        ++g_NativeCalls;
        const auto ped = context->GetArgument<Ped>(0);
        const auto lastVehicle = context->GetArgument<bool>(1);
        context->SetResult<Vehicle>(ped == 99 && !lastVehicle ? 77 : 0);
    }

    void EntityExistsHandler(NativeCallContext* context)
    {
        ++g_NativeCalls;
        const auto entity = context->GetArgument<Entity>(0);
        context->SetResult<bool>(entity == 77 || entity == 99);
    }

    NativeHandler Lookup(NativeHash hash)
    {
        switch (hash)
        {
        case Hashes::PLAYER_PED_ID: return &PlayerPedHandler;
        case Hashes::GET_VEHICLE_PED_IS_IN: return &VehiclePedHandler;
        case Hashes::DOES_ENTITY_EXIST: return &EntityExistsHandler;
        default: return nullptr;
        }
    }

    bool ReadHandling(Vehicle vehicle, Sick::Handling::Values& values) noexcept
    {
        ++g_ReadCalls;
        if (vehicle != 77)
            return false;
        values = g_LiveValues;
        return true;
    }

    bool WriteHandling(Vehicle vehicle, Sick::Handling::Field field, float value) noexcept
    {
        ++g_WriteCalls;
        if (vehicle != 77 || Sick::Handling::ToIndex(field) >= Sick::Handling::FieldCount)
            return false;
        g_LiveValues[Sick::Handling::ToIndex(field)] = value;
        return true;
    }
}

int main()
{
    using Sick::Backend::Features::HandlingFeatures;
    using Sick::Game::Enhanced::BuildManager;
    using Sick::Game::Enhanced::UnknownBuild;
    using Sick::Game::Handling::HandlingBackend;

    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
    {
        const auto& spec = Sick::Handling::FieldSpecs[index];
        auto value = std::min(spec.maximum, spec.minimum + std::max(spec.step * 2.0F, 0.1F));
        if (spec.integral)
            value = std::round(value);
        g_LiveValues[index] = value;
        g_OriginalValues[index] = value;
    }

    auto& resolver = NativeResolver::Get();
    resolver.Reset();
    resolver.SetResolver(&Lookup);
    BuildManager::SetBuild(1234);

    auto& handlingBackend = HandlingBackend::Get();
    handlingBackend.Clear();
    handlingBackend.Configure({1234, &ReadHandling, &WriteHandling});

    HandlingFeatures features;
    const auto idleNativeCalls = g_NativeCalls;
    features.Tick();
    CHECK(g_NativeCalls == idleNativeCalls);

    features.SetEditorActive(true);
    features.Tick();
    auto snapshot = features.Snapshot();
    CHECK(snapshot.backendAvailable);
    CHECK(snapshot.vehicleAttached);
    CHECK(g_ReadCalls == 1);
    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
        CHECK(NearlyEqual(snapshot.values[index], g_OriginalValues[index]));

    const auto massIndex = Sick::Handling::ToIndex(Sick::Handling::Field::Mass);
    const auto& massSpec = Sick::Handling::Spec(Sick::Handling::Field::Mass);
    const float changedMass = std::min(massSpec.maximum, g_OriginalValues[massIndex] + massSpec.step);
    features.SetValue(Sick::Handling::Field::Mass, changedMass);
    const auto beforeSingleWrite = g_WriteCalls;
    features.Tick();
    CHECK(g_WriteCalls == beforeSingleWrite + 1);
    CHECK(NearlyEqual(g_LiveValues[massIndex], changedMass));

    const auto beforeRestore = g_WriteCalls;
    features.RequestRestoreOriginal();
    features.Tick();
    auto firstRestoreBatch = g_WriteCalls - beforeRestore;
    CHECK(firstRestoreBatch > 0);
    CHECK(firstRestoreBatch <= 8);

    for (std::size_t tick = 0; tick < 16 && g_WriteCalls - beforeRestore < Sick::Handling::FieldCount; ++tick)
    {
        const auto beforeTick = g_WriteCalls;
        features.Tick();
        const auto writesThisTick = g_WriteCalls - beforeTick;
        CHECK(writesThisTick <= 8);
    }
    CHECK(g_WriteCalls - beforeRestore == Sick::Handling::FieldCount);
    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
        CHECK(NearlyEqual(g_LiveValues[index], g_OriginalValues[index]));

    features.SetEditorActive(false);
    const auto beforeIdleAgain = g_NativeCalls;
    const auto beforeIdleWrites = g_WriteCalls;
    features.Tick();
    CHECK(g_NativeCalls == beforeIdleAgain);
    CHECK(g_WriteCalls == beforeIdleWrites);

    handlingBackend.Clear();
    resolver.Reset();
    BuildManager::SetBuild(UnknownBuild);
    return 0;
}
