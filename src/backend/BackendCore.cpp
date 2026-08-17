#include "BackendCore.hpp"

#include "backend/system/LoggerApi.hpp"
#include "backend/tasking/TaskAffinity.hpp"
#include "game/enhanced/EnhancedScriptHost.hpp"
#include "game/enhanced/ScriptGlobal.hpp"
#include "game/natives/Natives.hpp"
#include "game/natives/NativeSystem.hpp"
#include "game/scripts/ScriptFunctionCatalog.hpp"
#include "game/scripts/ScriptRuntime.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <utility>

namespace Sick::Backend
{
    namespace
    {
        std::uint64_t ElapsedMicros(std::chrono::steady_clock::time_point start) noexcept
        {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - start).count());
        }

        [[nodiscard]] bool ValidSessionType(Sick::Backend::OnlineSessionType type) noexcept
        {
            using Type = Sick::Backend::OnlineSessionType;
            switch (type)
            {
            case Type::Public:
            case Type::SoloPublic:
            case Type::ClosedCrew:
            case Type::Crew:
            case Type::ClosedFriends:
            case Type::FindFriend:
            case Type::Solo:
            case Type::InviteOnly:
            case Type::JoinCrew:
            case Type::SpectatorTv:
            case Type::LeaveOnline:
                return true;
            }
            return false;
        }

        constexpr auto RewardScript = Sick::Game::Scripts::Joaat("am_mp_vehicle_reward");
        constexpr std::array<Sick::Game::Scripts::ScriptHash, 7> PersonalVehicleBlacklist{
            Sick::Game::Scripts::Joaat("rcbandito"),
            Sick::Game::Scripts::Joaat("minitank"),
            Sick::Game::Scripts::Joaat("thruster"),
            Sick::Game::Scripts::Joaat("terbyte"),
            Sick::Game::Scripts::Joaat("avenger"),
            Sick::Game::Scripts::Joaat("policet3"),
            Sick::Game::Scripts::Joaat("brickade2"),
        };

        [[nodiscard]] bool IsPersonalVehicleBlacklisted(Sick::Game::Scripts::ScriptHash model) noexcept
        {
            return std::ranges::find(PersonalVehicleBlacklist, model) != PersonalVehicleBlacklist.end();
        }

        void ClearVehicleRewardLocals() noexcept
        {
            auto* reward = Sick::Game::Enhanced::EnhancedScriptHost::LocalAddress(RewardScript, 148, 47);
            if (!reward)
                return;
            reward[3] = 0;
            reward[4] = 0;
            reward[5] = 0;
            reward[6] = 0;
        }

        constexpr Sick::Game::Natives::NativeHash SetVehicleModKitHash = 0xB5AD06DDA85E2E8FULL;
        constexpr Sick::Game::Natives::NativeHash GetNumVehicleModsHash = 0x5B59C12A02157D00ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModHash = 0x8450270DC5896D39ULL;
        constexpr Sick::Game::Natives::NativeHash ToggleVehicleModHash = 0xF5501FF9869DAC7CULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleWheelTypeHash = 0xE33678A9AE50A01BULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModColor1Hash = 0xA5277ECCD081FCC1ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModColor2Hash = 0x941B1F179D6AE19AULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleColoursHash = 0x4F1D4BE3A7F24601ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleExtraColoursHash = 0xBB361D7264AC4FD8ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleInteriorColorHash = 0xC0C8E6AAA00F1A58ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleDashboardColorHash = 0x77B012A683295B6EULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleNeonEnabledHash = 0xE62930EC6FAABCA5ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleNeonColourHash = 0xEAB8A43F6621850FULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleTireSmokeColorHash = 0x5DA0536AEAD1FF31ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleXenonColorHash = 0x89D1FDCA3735A1E0ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleExtraHash = 0xD772F6AA66750D2BULL;
        constexpr Sick::Game::Natives::NativeHash DoesExtraExistHash = 0x579FA5568DE0C2A0ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleTyresCanBurstHash = 0x439C904840715871ULL;
    }

    BackendCore& BackendCore::Get() noexcept
    {
        static BackendCore core;
        return core;
    }

    bool BackendCore::Initialize(const std::filesystem::path& moduleDirectory) noexcept
    {
        if (m_Initialized.load(std::memory_order_acquire))
            return true;
        if (!m_Background.Initialize(moduleDirectory))
            return false;

        const auto settings = m_Background.Settings().Snapshot().backend;
        m_MaxGameJobsPerTick = std::clamp<std::size_t>(settings.maxGameJobsPerTick, 1, 64);
        m_MaxFiberResumesPerTick = std::clamp<std::size_t>(settings.maxFiberResumesPerTick, 1, 32);
        m_MaxBackendMicros = std::clamp<std::uint64_t>(settings.maxBackendMicros, 50, 2000);
        m_ExitGtaRequested.store(false, std::memory_order_release);

        m_Initialized.store(true, std::memory_order_release);
        static_cast<void>(m_Background.Configs().Load("default"));
        System::LoggerApi::Get().Info("backend", "BackendCore initialized");
        return true;
    }

    void BackendCore::Shutdown() noexcept
    {
        if (!m_Initialized.exchange(false, std::memory_order_acq_rel))
            return;
        ResetGameState();
        System::LoggerApi::Get().Info("backend", "BackendCore shutting down");
        m_Background.Shutdown();
    }

    void BackendCore::ResetGameState() noexcept
    {
        m_CallHub.Reset();
        m_Features.Reset();
        m_Fibers.Clear();
        m_VehicleSpawner.Reset();
        m_NativeReady.store(false, std::memory_order_release);
        m_ScriptReady.store(false, std::memory_order_release);
        m_SessionSwitchState.store(SessionSwitchState::Idle, std::memory_order_release);
        m_SessionSwitchTarget.store(OnlineSessionType::Public, std::memory_order_release);
        m_PersonalVehicleSaveRequested.store(false, std::memory_order_release);
        m_PersonalVehicleSaveAttempts.store(0, std::memory_order_release);
        m_PersonalVehicleSaveState.store(PersonalVehicleSaveState::Idle, std::memory_order_release);
        m_Performance.Reset();
    }

    void BackendCore::TickGame() noexcept
    {
        Tasking::ScopedTaskAffinity affinity{Tasking::TaskAffinity::Game};
        const auto start = std::chrono::steady_clock::now();
        const bool nativeReady = Game::Natives::NativeSystem::Ready();
        const bool scriptReady = Game::Scripts::ScriptRuntime::Get().Ready();
        m_NativeReady.store(nativeReady, std::memory_order_release);
        m_ScriptReady.store(scriptReady, std::memory_order_release);

        if (const auto profile = m_Background.Configs().TakePendingProfile())
            m_Features.ApplyProfile(*profile);
        if (const auto handling = m_Background.HandlingProfiles().TakePendingValues())
            m_Features.ApplyHandlingValues(*handling);

        m_Features.Tick(nativeReady);
        TickPersonalVehicleSave(nativeReady, scriptReady);

        const auto afterFeatures = ElapsedMicros(start);
        const auto callBudget = afterFeatures < m_MaxBackendMicros
            ? m_MaxBackendMicros - afterFeatures
            : 0;
        const auto callStats = m_CallHub.Tick(
            m_MaxGameJobsPerTick,
            callBudget,
            nativeReady,
            scriptReady);

        const auto afterCalls = ElapsedMicros(start);
        const auto fiberBudget = afterCalls < m_MaxBackendMicros
            ? m_MaxBackendMicros - afterCalls
            : 0;
        const auto fiberResumes = m_Fibers.Tick(
            m_MaxFiberResumesPerTick,
            fiberBudget);

        m_Performance.Record(
            ElapsedMicros(start),
            m_MaxBackendMicros,
            callStats.executed,
            fiberResumes);
    }

    bool BackendCore::QueueGame(Calls::GameCallHub::Job job) { return m_CallHub.QueueGame(std::move(job)); }
    bool BackendCore::QueueNative(Calls::GameCallHub::Job job) { return m_CallHub.QueueNative(std::move(job)); }
    bool BackendCore::QueueScript(Calls::GameCallHub::Job job) { return m_CallHub.QueueScript(std::move(job)); }
    bool BackendCore::QueueFiber(Tasking::GameFiberScheduler::Task task) { return m_Fibers.Queue(std::move(task)); }

    void BackendCore::SetGodMode(bool enabled) noexcept { m_Features.SetGodMode(enabled); }
    void BackendCore::SetInfiniteOxygen(bool enabled) noexcept { m_Features.SetInfiniteOxygen(enabled); }
    void BackendCore::SetNoRagdoll(bool enabled) noexcept { m_Features.SetNoRagdoll(enabled); }
    void BackendCore::SetSuperJump(bool enabled) noexcept { m_Features.SetSuperJump(enabled); }
    void BackendCore::SetSeatBelt(bool enabled) noexcept { m_Features.SetSeatBelt(enabled); }
    void BackendCore::SetNoWantedLevel(bool enabled) noexcept { m_Features.SetNoWantedLevel(enabled); }
    void BackendCore::SetWantedLevel(int level) noexcept { m_Features.SetWantedLevel(level); }
    void BackendCore::SetFastRun(bool enabled) noexcept { m_Features.SetFastRun(enabled); }
    void BackendCore::SetFastSwim(bool enabled) noexcept { m_Features.SetFastSwim(enabled); }
    void BackendCore::SetKeepPlayerClean(bool enabled) noexcept { m_Features.SetKeepPlayerClean(enabled); }
    void BackendCore::SetAqualung(bool enabled) noexcept { m_Features.SetAqualung(enabled); }
    void BackendCore::SetNoGravity(bool enabled) noexcept { m_Features.SetNoGravity(enabled); }
    void BackendCore::SetWaterproof(bool enabled) noexcept { m_Features.SetWaterproof(enabled); }

    void BackendCore::SetVehicleGodMode(bool enabled) noexcept { m_Features.SetVehicleGodMode(enabled); }
    void BackendCore::SetVehicleAutoRepair(bool enabled) noexcept { m_Features.SetVehicleAutoRepair(enabled); }
    void BackendCore::SetVehicleKeepClean(bool enabled) noexcept { m_Features.SetVehicleKeepClean(enabled); }
    void BackendCore::SetVehicleEngineAlwaysOn(bool enabled) noexcept { m_Features.SetVehicleEngineAlwaysOn(enabled); }
    void BackendCore::SetVehicleNoGravity(bool enabled) noexcept { m_Features.SetVehicleNoGravity(enabled); }
    void BackendCore::SetVehicleNoCollision(bool enabled) noexcept { m_Features.SetVehicleNoCollision(enabled); }
    void BackendCore::RepairVehicle() noexcept { m_Features.RepairVehicle(); }
    void BackendCore::CleanVehicle() noexcept { m_Features.CleanVehicle(); }
    void BackendCore::PutVehicleOnGround() noexcept { m_Features.PutVehicleOnGround(); }

    bool BackendCore::CustomizeCurrentVehicle(
        VehicleCustomizationCommand command,
        int a,
        int b,
        int c,
        int d)
    {
        if (!m_NativeReady.load(std::memory_order_acquire))
            return false;

        return QueueNative([command, a, b, c, d] {
            const auto ped = Game::Natives::PLAYER::PLAYER_PED_ID();
            const auto vehicle = Game::Natives::PED::GET_VEHICLE_PED_IS_IN(ped, false);
            if (vehicle == 0 || !Game::Natives::ENTITY::DOES_ENTITY_EXIST(vehicle))
                return;

            using Invoker = Game::Natives::NativeInvoker;
            static_cast<void>(Invoker::TryCallVoid(SetVehicleModKitHash, vehicle, 0));

            switch (command)
            {
            case VehicleCustomizationCommand::SetMod:
            {
                const int slot = std::clamp(a, 0, 49);
                const auto count = Invoker::TryCall<int>(GetNumVehicleModsHash, vehicle, slot);
                if (!count || *count <= 0)
                {
                    static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, -1, false));
                    return;
                }
                const int index = std::clamp(b, -1, *count - 1);
                static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, index, c != 0));
                return;
            }
            case VehicleCustomizationCommand::ToggleMod:
                static_cast<void>(Invoker::TryCallVoid(
                    ToggleVehicleModHash,
                    vehicle,
                    std::clamp(a, 0, 49),
                    b != 0));
                return;
            case VehicleCustomizationCommand::SetWheelType:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleWheelTypeHash, vehicle, std::clamp(a, 0, 12)));
                return;
            case VehicleCustomizationCommand::SetPrimaryModColor:
                if (a >= 6)
                {
                    static_cast<void>(Invoker::TryCallVoid(
                        SetVehicleColoursHash,
                        vehicle,
                        std::clamp(b, 0, 222),
                        std::clamp(c, 0, 222)));
                    return;
                }
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleModColor1Hash,
                    vehicle,
                    std::clamp(a, 0, 5),
                    std::clamp(b, 0, 160),
                    0));
                return;
            case VehicleCustomizationCommand::SetSecondaryModColor:
                if (a >= 6)
                {
                    static_cast<void>(Invoker::TryCallVoid(
                        SetVehicleColoursHash,
                        vehicle,
                        std::clamp(c, 0, 222),
                        std::clamp(b, 0, 222)));
                    return;
                }
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleModColor2Hash,
                    vehicle,
                    std::clamp(a, 0, 5),
                    std::clamp(b, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetExtraColours:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleExtraColoursHash,
                    vehicle,
                    std::clamp(a, 0, 160),
                    std::clamp(b, 0, 222)));
                return;
            case VehicleCustomizationCommand::SetInteriorColor:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleInteriorColorHash, vehicle, std::clamp(a, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetDashboardColor:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleDashboardColorHash, vehicle, std::clamp(a, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetNeonEnabled:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleNeonEnabledHash,
                    vehicle,
                    std::clamp(a, 0, 3),
                    b != 0));
                return;
            case VehicleCustomizationCommand::SetNeonColor:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleNeonColourHash,
                    vehicle,
                    std::clamp(a, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(c, 0, 255)));
                return;
            case VehicleCustomizationCommand::SetTireSmokeColor:
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 20, true));
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleTireSmokeColorHash,
                    vehicle,
                    std::clamp(a, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(c, 0, 255)));
                return;
            case VehicleCustomizationCommand::SetXenonColor:
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 22, true));
                static_cast<void>(Invoker::TryCallVoid(SetVehicleXenonColorHash, vehicle, std::clamp(a, -1, 12)));
                return;
            case VehicleCustomizationCommand::SetExtra:
            {
                const int extra = std::clamp(a, 1, 14);
                const auto exists = Invoker::TryCall<bool>(DoesExtraExistHash, vehicle, extra);
                if (exists && *exists)
                    static_cast<void>(Invoker::TryCallVoid(SetVehicleExtraHash, vehicle, extra, b == 0));
                return;
            }
            case VehicleCustomizationCommand::SetBulletproofTires:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleTyresCanBurstHash, vehicle, a == 0));
                return;
            case VehicleCustomizationCommand::MaxVehicle:
                for (int slot = 0; slot < 50; ++slot)
                {
                    if (slot == 17 || slot == 18 || slot == 19 || slot == 20 || slot == 21 || slot == 22)
                        continue;
                    const auto count = Invoker::TryCall<int>(GetNumVehicleModsHash, vehicle, slot);
                    if (count && *count > 0)
                        static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, *count - 1, false));
                }
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 18, true));
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 22, true));
                static_cast<void>(Invoker::TryCallVoid(SetVehicleTyresCanBurstHash, vehicle, false));
                return;
            }
        });
    }

    bool BackendCore::SpawnVehicle(std::string_view modelName, bool enterVehicle)
    {
        const auto modelHash = Features::OnlineVehicleSpawner::VehicleSpawner::HashModelName(modelName);
        if (!m_NativeReady.load(std::memory_order_acquire))
        {
            m_VehicleSpawner.Reject(modelHash, VehicleSpawnerState::NativeUnavailable);
            return false;
        }
        if (!m_VehicleSpawner.TryQueue(modelHash))
            return false;

        if (!QueueFiber([this, modelHash, enterVehicle]() {
                m_VehicleSpawner.Spawn(modelHash, enterVehicle);
            }))
        {
            m_VehicleSpawner.QueueFailed();
            return false;
        }
        return true;
    }

    bool BackendCore::SwitchOnlineSession(OnlineSessionType type)
    {
        m_SessionSwitchTarget.store(type, std::memory_order_release);
        if (!ValidSessionType(type))
        {
            m_SessionSwitchState.store(SessionSwitchState::Failed, std::memory_order_release);
            return false;
        }

        const auto state = m_SessionSwitchState.load(std::memory_order_acquire);
        if (state == SessionSwitchState::Queued || state == SessionSwitchState::Switching)
            return false;

        if (!m_ScriptReady.load(std::memory_order_acquire))
        {
            m_SessionSwitchState.store(SessionSwitchState::ScriptUnavailable, std::memory_order_release);
            return false;
        }

        m_SessionSwitchState.store(SessionSwitchState::Queued, std::memory_order_release);
        if (!QueueScript([this, type] {
                m_SessionSwitchState.store(SessionSwitchState::Switching, std::memory_order_release);
                const auto* specification = Game::Scripts::ScriptFunctionCatalog::Find(
                    Game::Scripts::KnownScriptFunction::SendToClouds);
                if (!specification)
                {
                    m_SessionSwitchState.store(SessionSwitchState::Failed, std::memory_order_release);
                    return;
                }

                Game::Enhanced::ScriptGlobal joinTypeGlobal{1575048};
                auto* joinType = joinTypeGlobal.As<std::int32_t*>();
                if (!joinType)
                {
                    m_SessionSwitchState.store(SessionSwitchState::GlobalUnavailable, std::memory_order_release);
                    return;
                }

                const auto sendToClouds = specification->Bind();
                if (!sendToClouds.TryCallVoid())
                {
                    m_SessionSwitchState.store(SessionSwitchState::ScriptUnavailable, std::memory_order_release);
                    return;
                }

                *joinType = static_cast<std::int32_t>(type);
                m_SessionSwitchState.store(SessionSwitchState::Complete, std::memory_order_release);
            }))
        {
            m_SessionSwitchState.store(SessionSwitchState::Failed, std::memory_order_release);
            return false;
        }
        return true;
    }

    bool BackendCore::SaveCurrentVehicleToPersonalGarage()
    {
        const auto state = m_PersonalVehicleSaveState.load(std::memory_order_acquire);
        if (state == PersonalVehicleSaveState::Queued ||
            state == PersonalVehicleSaveState::Validating ||
            state == PersonalVehicleSaveState::WaitingForGarageSelection)
        {
            return false;
        }

        m_PersonalVehicleSaveState.store(PersonalVehicleSaveState::Queued, std::memory_order_release);
        m_PersonalVehicleSaveAttempts.store(0, std::memory_order_release);
        m_PersonalVehicleSaveRequested.store(true, std::memory_order_release);
        return true;
    }

    void BackendCore::TickPersonalVehicleSave(bool nativeReady, bool scriptReady) noexcept
    {
        if (!m_PersonalVehicleSaveRequested.load(std::memory_order_acquire))
            return;

        const auto finish = [this](PersonalVehicleSaveState state, bool clearReward = false) {
            if (clearReward)
                ClearVehicleRewardLocals();
            m_PersonalVehicleSaveRequested.store(false, std::memory_order_release);
            m_PersonalVehicleSaveAttempts.store(0, std::memory_order_release);
            m_PersonalVehicleSaveState.store(state, std::memory_order_release);
        };

        if (!nativeReady)
        {
            finish(PersonalVehicleSaveState::NativeUnavailable, true);
            return;
        }
        if (!scriptReady)
        {
            finish(PersonalVehicleSaveState::ScriptUnavailable, true);
            return;
        }

        m_PersonalVehicleSaveState.store(PersonalVehicleSaveState::Validating, std::memory_order_release);
        const auto ped = Game::Natives::PLAYER::PLAYER_PED_ID();
        const auto vehicle = Game::Natives::PED::GET_VEHICLE_PED_IS_IN(ped, false);
        if (vehicle == 0 || !Game::Natives::ENTITY::DOES_ENTITY_EXIST(vehicle))
        {
            finish(PersonalVehicleSaveState::NoVehicle, true);
            return;
        }

        const auto model = Game::Natives::ENTITY::GET_ENTITY_MODEL(vehicle);
        if (IsPersonalVehicleBlacklisted(model))
        {
            finish(PersonalVehicleSaveState::InvalidVehicle, true);
            return;
        }

        const auto* validSpec = Game::Scripts::ScriptFunctionCatalog::Find(
            Game::Scripts::KnownScriptFunction::IsVehicleValidForPersonalVehicle);
        if (!validSpec)
        {
            finish(PersonalVehicleSaveState::Failed, true);
            return;
        }
        const auto isVehicleValid = validSpec->Bind();
        const auto valid = isVehicleValid.TryCall<bool>(model);
        if (!valid)
        {
            finish(PersonalVehicleSaveState::ScriptUnavailable, true);
            return;
        }
        if (!*valid)
        {
            finish(PersonalVehicleSaveState::InvalidVehicle, true);
            return;
        }

        Game::Enhanced::ScriptGlobal personalVehicleGlobal{2733326};
        auto* personalVehicle = personalVehicleGlobal.At(301).As<std::int32_t*>();
        if (!personalVehicle)
        {
            finish(PersonalVehicleSaveState::GlobalUnavailable, true);
            return;
        }
        if (*personalVehicle == vehicle)
        {
            finish(PersonalVehicleSaveState::AlreadyPersonal, true);
            return;
        }

        auto* reward = Game::Enhanced::EnhancedScriptHost::LocalAddress(RewardScript, 148, 47);
        auto* vehicleMenuData = Game::Enhanced::EnhancedScriptHost::LocalAddress(RewardScript, 195, 1);
        if (!reward || !vehicleMenuData)
        {
            finish(PersonalVehicleSaveState::RewardScriptUnavailable, true);
            return;
        }

        const auto* rewardSpec = Game::Scripts::ScriptFunctionCatalog::Find(
            Game::Scripts::KnownScriptFunction::GiveVehicleReward);
        if (!rewardSpec)
        {
            finish(PersonalVehicleSaveState::Failed, true);
            return;
        }
        const auto giveVehicleReward = rewardSpec->Bind();
        const auto opened = giveVehicleReward.TryCall<bool>(
            vehicle,
            vehicleMenuData,
            reward + 3,
            reward + 4,
            reward + 5,
            reward + 6,
            false,
            true,
            true,
            false,
            0,
            -1);
        if (!opened)
        {
            finish(PersonalVehicleSaveState::ScriptUnavailable, true);
            return;
        }
        if (!*opened)
        {
            const auto attempts = m_PersonalVehicleSaveAttempts.fetch_add(1, std::memory_order_acq_rel) + 1;
            if (attempts >= 600)
                finish(PersonalVehicleSaveState::Failed, true);
            return;
        }

        const auto controlStatus = static_cast<std::int32_t>(reward[6] & 0xFFFFFFFFULL);
        if (controlStatus == 3)
        {
            m_PersonalVehicleSaveAttempts.store(0, std::memory_order_release);
            m_PersonalVehicleSaveState.store(
                PersonalVehicleSaveState::WaitingForGarageSelection,
                std::memory_order_release);
            return;
        }

        finish(PersonalVehicleSaveState::Complete, true);
    }

    void BackendCore::SetHandlingEditorActive(bool active) noexcept { m_Features.SetHandlingEditorActive(active); }
    void BackendCore::SetHandlingValue(Handling::Field field, float value) noexcept { m_Features.SetHandlingValue(field, value); }
    void BackendCore::RestoreOriginalHandling() noexcept { m_Features.RestoreOriginalHandling(); }

    bool BackendCore::SaveHandlingProfile()
    {
        const auto handling = m_Features.HandlingSnapshot();
        return handling.backendAvailable && handling.vehicleAttached &&
            m_Background.HandlingProfiles().Save(m_Features.HandlingValues());
    }

    bool BackendCore::LoadHandlingProfile(std::string_view name)
    {
        return m_Background.HandlingProfiles().Load(name);
    }

    bool BackendCore::RefreshHandlingProfiles() { return m_Background.HandlingProfiles().Refresh(); }
    HandlingProfileCatalogSnapshot BackendCore::HandlingProfiles() const { return m_Background.HandlingProfiles().Snapshot(); }
    std::uint64_t BackendCore::HandlingProfileGeneration() const noexcept { return m_Background.HandlingProfiles().Generation(); }

    bool BackendCore::SaveProfile(std::string_view name)
    {
        return m_Background.Configs().Save(name, m_Features.Profile());
    }

    bool BackendCore::LoadProfile(std::string_view name)
    {
        return m_Background.Configs().Load(name);
    }

    bool BackendCore::SaveConfiguration()
    {
        const bool profileQueued = SaveProfile("default");
        const bool settingsQueued = m_Background.Settings().SaveAsync(m_Background.Io(), m_Background.Files());
        return profileQueued && settingsQueued;
    }

    AssetCatalogSnapshot BackendCore::Assets() const { return m_Background.Assets().Snapshot(); }
    std::uint64_t BackendCore::AssetGeneration() const noexcept { return m_Background.Assets().Generation(); }
    bool BackendCore::RefreshAssets() { return m_Background.Assets().Refresh(); }

    MenuPreferences BackendCore::Preferences() const
    {
        const auto frontend = m_Background.Settings().Snapshot().frontend;
        return {
            .scale = frontend.menuScale,
            .left = frontend.menuLeft,
            .top = frontend.menuTop,
            .theme = frontend.theme,
            .banner = frontend.banner,
            .font = frontend.font,
        };
    }

    void BackendCore::SetPreferences(MenuPreferences preferences) noexcept
    {
        auto settings = m_Background.Settings().Snapshot().frontend;
        settings.menuScale = std::clamp(preferences.scale, 0.5F, 2.5F);
        settings.menuLeft = std::clamp(preferences.left, -1920.0F, 3840.0F);
        settings.menuTop = std::clamp(preferences.top, -1080.0F, 2160.0F);
        settings.theme = std::move(preferences.theme);
        settings.banner = std::move(preferences.banner);
        settings.font = std::move(preferences.font);
        m_Background.Settings().SetFrontend(std::move(settings));
    }

    BackendSnapshot BackendCore::Snapshot() const noexcept
    {
        const auto player = m_Features.PlayerSnapshot();
        const auto vehicle = m_Features.VehicleSnapshot();
        const auto vehicleSpawner = m_VehicleSpawner.Snapshot();
        const auto sessionSwitchState = m_SessionSwitchState.load(std::memory_order_acquire);
        const auto personalVehicleSaveState = m_PersonalVehicleSaveState.load(std::memory_order_acquire);
        const auto handling = m_Features.HandlingSnapshot();
        const auto performance = m_Performance.Snapshot();
        const auto background = m_Background.Snapshot();
        return {
            .initialized = m_Initialized.load(std::memory_order_acquire),
            .nativeReady = m_NativeReady.load(std::memory_order_acquire),
            .scriptReady = m_ScriptReady.load(std::memory_order_acquire),
            .player = player,
            .vehicle = vehicle,
            .vehicleSpawner = vehicleSpawner,
            .sessionSwitch = {
                .state = sessionSwitchState,
                .target = m_SessionSwitchTarget.load(std::memory_order_acquire),
                .busy = sessionSwitchState == SessionSwitchState::Queued ||
                    sessionSwitchState == SessionSwitchState::Switching,
            },
            .personalVehicleSave = {
                .state = personalVehicleSaveState,
                .busy = personalVehicleSaveState == PersonalVehicleSaveState::Queued ||
                    personalVehicleSaveState == PersonalVehicleSaveState::Validating ||
                    personalVehicleSaveState == PersonalVehicleSaveState::WaitingForGarageSelection,
            },
            .handling = handling,
            .queues = {
                .gameCalls = m_CallHub.Pending(),
                .fibers = m_Fibers.Pending(),
                .background = background.io.pending,
            },
            .performance = {
                .lastTickMicros = performance.lastTickMicros,
                .maxTickMicros = performance.maxTickMicros,
                .overBudgetTicks = performance.overBudgetTicks,
                .lastJobs = performance.lastJobs,
                .lastFiberResumes = performance.lastFiberResumes,
            },
            .background = background,
        };
    }
}
