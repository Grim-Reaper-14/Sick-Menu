#include "BackendCore.hpp"

#include "backend/system/LoggerApi.hpp"
#include "backend/tasking/TaskAffinity.hpp"
#include "game/natives/NativeSystem.hpp"
#include "game/scripts/ScriptRuntime.hpp"

#include <algorithm>
#include <chrono>
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
        m_NativeReady.store(false, std::memory_order_release);
        m_ScriptReady.store(false, std::memory_order_release);
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
        const auto handling = m_Features.HandlingSnapshot();
        const auto performance = m_Performance.Snapshot();
        const auto background = m_Background.Snapshot();
        return {
            .initialized = m_Initialized.load(std::memory_order_acquire),
            .nativeReady = m_NativeReady.load(std::memory_order_acquire),
            .scriptReady = m_ScriptReady.load(std::memory_order_acquire),
            .player = player,
            .vehicle = vehicle,
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
