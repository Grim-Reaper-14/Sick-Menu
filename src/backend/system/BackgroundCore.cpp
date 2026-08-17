#include "BackgroundCore.hpp"

#include "LoggerApi.hpp"

#include <algorithm>
#include <string>

namespace Sick::Backend::System
{
    bool BackgroundCore::Initialize(const std::filesystem::path& moduleDirectory) noexcept
    {
        if (m_Initialized)
            return true;
        if (!m_Files.Initialize(moduleDirectory))
            return false;
        if (!m_Settings.Load(m_Files))
            return false;

        const auto workers = std::clamp<std::size_t>(
            m_Settings.Snapshot().backend.backgroundWorkerCount, 1, 4);
        if (!m_Io.Start(workers, 1024))
            return false;
        if (!m_Configs.Initialize(m_Io, m_Files))
        {
            m_Io.Stop();
            return false;
        }
        if (!m_HandlingProfiles.Initialize(m_Io, m_Files))
        {
            m_Configs.Shutdown();
            m_Io.Stop();
            return false;
        }
        if (!m_Assets.Initialize(m_Io, m_Files))
        {
            m_HandlingProfiles.Shutdown();
            m_Configs.Shutdown();
            m_Io.Stop();
            return false;
        }
        if (!LoggerApi::Get().Initialize(m_Files))
        {
            m_Assets.Shutdown();
            m_HandlingProfiles.Shutdown();
            m_Configs.Shutdown();
            m_Io.Stop();
            return false;
        }

        m_Initialized = true;
        LoggerApi::Get().Info("background", "BackgroundCore initialized", {
            {"io_workers", std::to_string(workers)},
        });
        return true;
    }

    void BackgroundCore::Shutdown() noexcept
    {
        if (!m_Initialized)
            return;
        m_Initialized = false;

        m_Assets.Shutdown();
        m_HandlingProfiles.Shutdown();
        m_Configs.Shutdown();
        static_cast<void>(m_Settings.SaveAsync(m_Io, m_Files));
        LoggerApi::Get().Info("background", "BackgroundCore shutting down");
        m_Io.Stop();
        LoggerApi::Get().Flush();
        LoggerApi::Get().Shutdown();
    }

    BackgroundSnapshot BackgroundCore::Snapshot() const noexcept
    {
        return {
            .logger = LoggerApi::Get().Snapshot(),
            .filesystem = m_Files.Snapshot(),
            .io = m_Io.Snapshot(),
        };
    }
}
