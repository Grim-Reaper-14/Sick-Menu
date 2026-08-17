#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace Sick::Backend::System
{
    class FileSystem;
    class IoService;

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
        float menuLeft{48.0F};
        float menuTop{28.0F};
        bool animations{true};
        int toggleKey{0x73}; // VK_F4 without a Windows dependency.
        std::string theme{"Default"};
        std::string banner;
        std::string font;
    };

    struct SettingsSnapshot
    {
        BackendSettings backend;
        FrontendSettings frontend;
    };

    class SettingsManager final
    {
    public:
        bool Load(FileSystem& files) noexcept;
        [[nodiscard]] bool SaveAsync(IoService& io, FileSystem& files) const;

        [[nodiscard]] SettingsSnapshot Snapshot() const noexcept;
        void SetBackend(BackendSettings settings) noexcept;
        void SetFrontend(FrontendSettings settings) noexcept;

    private:
        mutable std::mutex m_Mutex;
        SettingsSnapshot m_Settings;
    };
}
