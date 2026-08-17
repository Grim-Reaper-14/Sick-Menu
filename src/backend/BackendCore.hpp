#pragma once

#include "BackendTypes.hpp"
#include "backend/calls/GameCallHub.hpp"
#include "backend/features/FeatureManager.hpp"
#include "backend/features/online_vehicle_spawner/VehicleSpawner.hpp"
#include "backend/system/BackgroundCore.hpp"
#include "backend/system/PerformanceMonitor.hpp"
#include "backend/tasking/GameFiberScheduler.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Sick::Backend
{
    class BackendCore final
    {
    public:
        static BackendCore& Get() noexcept;

        bool Initialize(const std::filesystem::path& moduleDirectory) noexcept;
        void Shutdown() noexcept;
        void ResetGameState() noexcept;
        void TickGame() noexcept;

        [[nodiscard]] bool QueueGame(Calls::GameCallHub::Job job);
        [[nodiscard]] bool QueueNative(Calls::GameCallHub::Job job);
        [[nodiscard]] bool QueueScript(Calls::GameCallHub::Job job);
        [[nodiscard]] bool QueueFiber(Tasking::GameFiberScheduler::Task task);

        void SetGodMode(bool enabled) noexcept;
        void SetInfiniteOxygen(bool enabled) noexcept;
        void SetNoRagdoll(bool enabled) noexcept;
        void SetSuperJump(bool enabled) noexcept;
        void SetSeatBelt(bool enabled) noexcept;
        void SetNoWantedLevel(bool enabled) noexcept;
        void SetWantedLevel(int level) noexcept;
        void SetFastRun(bool enabled) noexcept;
        void SetFastSwim(bool enabled) noexcept;
        void SetKeepPlayerClean(bool enabled) noexcept;
        void SetAqualung(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetWaterproof(bool enabled) noexcept;

        void SetVehicleGodMode(bool enabled) noexcept;
        void SetVehicleAutoRepair(bool enabled) noexcept;
        void SetVehicleKeepClean(bool enabled) noexcept;
        void SetVehicleEngineAlwaysOn(bool enabled) noexcept;
        void SetVehicleNoGravity(bool enabled) noexcept;
        void SetVehicleNoCollision(bool enabled) noexcept;
        void RepairVehicle() noexcept;
        void CleanVehicle() noexcept;
        void PutVehicleOnGround() noexcept;
        [[nodiscard]] bool SpawnVehicle(std::string_view modelName, bool enterVehicle);
        [[nodiscard]] bool SwitchOnlineSession(OnlineSessionType type);
        [[nodiscard]] bool SaveCurrentVehicleToPersonalGarage();

        void SetHandlingEditorActive(bool active) noexcept;
        void SetHandlingValue(Handling::Field field, float value) noexcept;
        void RestoreOriginalHandling() noexcept;
        [[nodiscard]] bool SaveHandlingProfile();
        [[nodiscard]] bool LoadHandlingProfile(std::string_view name);
        [[nodiscard]] bool RefreshHandlingProfiles();
        [[nodiscard]] HandlingProfileCatalogSnapshot HandlingProfiles() const;
        [[nodiscard]] std::uint64_t HandlingProfileGeneration() const noexcept;

        [[nodiscard]] bool SaveProfile(std::string_view name);
        [[nodiscard]] bool LoadProfile(std::string_view name);
        [[nodiscard]] bool SaveConfiguration();

        [[nodiscard]] AssetCatalogSnapshot Assets() const;
        [[nodiscard]] std::uint64_t AssetGeneration() const noexcept;
        [[nodiscard]] bool RefreshAssets();
        [[nodiscard]] MenuPreferences Preferences() const;
        void SetPreferences(MenuPreferences preferences) noexcept;

        void RequestExitGta() noexcept { m_ExitGtaRequested.store(true, std::memory_order_release); }
        [[nodiscard]] bool ExitGtaRequested() const noexcept { return m_ExitGtaRequested.load(std::memory_order_acquire); }

        [[nodiscard]] BackendSnapshot Snapshot() const noexcept;
        [[nodiscard]] const System::FileSystem& Files() const noexcept { return m_Background.Files(); }
        [[nodiscard]] System::SettingsManager& Settings() noexcept { return m_Background.Settings(); }

    private:
        BackendCore() = default;

        void TickPersonalVehicleSave(bool nativeReady, bool scriptReady) noexcept;

        Calls::GameCallHub m_CallHub;
        Features::FeatureManager m_Features;
        Features::OnlineVehicleSpawner::VehicleSpawner m_VehicleSpawner;
        Tasking::GameFiberScheduler m_Fibers;
        System::BackgroundCore m_Background;
        System::PerformanceMonitor m_Performance;

        std::atomic_bool m_Initialized{};
        std::atomic_bool m_NativeReady{};
        std::atomic_bool m_ScriptReady{};
        std::atomic_bool m_ExitGtaRequested{};
        std::atomic<SessionSwitchState> m_SessionSwitchState{SessionSwitchState::Idle};
        std::atomic<OnlineSessionType> m_SessionSwitchTarget{OnlineSessionType::Public};
        std::atomic<PersonalVehicleSaveState> m_PersonalVehicleSaveState{PersonalVehicleSaveState::Idle};
        std::atomic_bool m_PersonalVehicleSaveRequested{};
        std::atomic<std::uint32_t> m_PersonalVehicleSaveAttempts{};
        std::size_t m_MaxGameJobsPerTick{16};
        std::size_t m_MaxFiberResumesPerTick{8};
        std::uint64_t m_MaxBackendMicros{250};
    };
}
