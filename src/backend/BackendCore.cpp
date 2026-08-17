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

        m_Initialized.store(true, std::memory_order_release);
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

    bool BackendCore::QueueGame(Calls::GameCallHub::Job job)
    {
        return m_CallHub.QueueGame(std::move(job));
    }

    bool BackendCore::QueueNative(Calls::GameCallHub::Job job)
    {
        return m_CallHub.QueueNative(std::move(job));
    }

    bool BackendCore::QueueScript(Calls::GameCallHub::Job job)
    {
        return m_CallHub.QueueScript(std::move(job));
    }

    bool BackendCore::QueueFiber(Tasking::GameFiberScheduler::Task task)
    {
        return m_Fibers.Queue(std::move(task));
    }

    void BackendCore::SetGodMode(bool enabled) noexcept
    {
        m_Features.SetGodMode(enabled);
    }

    bool BackendCore::SaveProfile(std::string_view name)
    {
        return m_Background.Configs().Save(name, m_Features.Profile());
    }

    bool BackendCore::LoadProfile(std::string_view name)
    {
        return m_Background.Configs().Load(name);
    }

    BackendSnapshot BackendCore::Snapshot() const noexcept
    {
        const auto player = m_Features.PlayerSnapshot();
        const auto performance = m_Performance.Snapshot();
        const auto background = m_Background.Snapshot();
        return {
            .initialized = m_Initialized.load(std::memory_order_acquire),
            .nativeReady = m_NativeReady.load(std::memory_order_acquire),
            .scriptReady = m_ScriptReady.load(std::memory_order_acquire),
            .player = player,
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
