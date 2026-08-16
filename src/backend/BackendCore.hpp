#pragma once

#include "BackendTypes.hpp"
#include "backend/calls/GameCallHub.hpp"
#include "backend/system/FileSystem.hpp"
#include "backend/system/PerformanceMonitor.hpp"
#include "backend/system/SettingsManager.hpp"
#include "backend/tasking/GameFiberScheduler.hpp"
#include "backend/tasking/ThreadPool.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>

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

        [[nodiscard]] BackendSnapshot Snapshot() const noexcept;
        [[nodiscard]] const System::FileSystem& Files() const noexcept { return m_FileSystem; }
        [[nodiscard]] System::SettingsManager& Settings() noexcept { return m_Settings; }

    private:
        BackendCore() = default;

        Calls::GameCallHub m_CallHub;
        Tasking::GameFiberScheduler m_Fibers;
        Tasking::ThreadPool m_Threads;
        System::FileSystem m_FileSystem;
        System::SettingsManager m_Settings;
        System::PerformanceMonitor m_Performance;

        std::atomic_bool m_Initialized{};
        std::atomic_bool m_NativeReady{};
        std::atomic_bool m_ScriptReady{};
        std::size_t m_MaxGameJobsPerTick{16};
        std::size_t m_MaxFiberResumesPerTick{8};
        std::uint64_t m_MaxBackendMicros{250};
    };
}
