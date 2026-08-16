#pragma once

#include "backend/tasking/ThreadPool.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>

namespace Sick::Backend::System
{
    struct BackendSettings
    {
        std::size_t backgroundWorkerCount{2};
        std::size_t maxGameJobsPerTick{16};
        std::size_t maxFiberResumesPerTick{8};
        std::uint64_t maxBackendMicros{250};
    };

    struct FrontendSettings
    {
        float menuScale{1.25F};
        bool animations{true};
        int toggleKey{0x73}; // VK_F4 without a Windows dependency.
    };

    struct SettingsSnapshot
    {
        BackendSettings backend;
        FrontendSettings frontend;
    };

    class SettingsManager final
    {
    public:
        bool Load(const std::filesystem::path& path) noexcept;
        [[nodiscard]] bool SaveAsync(
            Tasking::ThreadPool& pool,
            const std::filesystem::path& path) const;

        [[nodiscard]] SettingsSnapshot Snapshot() const noexcept;
        void SetBackend(BackendSettings settings) noexcept;
        void SetFrontend(FrontendSettings settings) noexcept;

    private:
        mutable std::mutex m_Mutex;
        SettingsSnapshot m_Settings;
    };
}
