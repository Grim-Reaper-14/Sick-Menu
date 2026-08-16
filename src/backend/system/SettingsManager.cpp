#include "SettingsManager.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System
{
    namespace
    {
        bool CommitSettingsFile(
            const std::filesystem::path& temporary,
            const std::filesystem::path& destination) noexcept
        {
#if defined(_WIN32)
            return MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
            std::error_code error;
            std::filesystem::rename(temporary, destination, error);
            if (!error)
                return true;

            std::filesystem::remove(destination, error);
            error.clear();
            std::filesystem::rename(temporary, destination, error);
            return !error;
#endif
        }
    }

    bool SettingsManager::Load(const std::filesystem::path& path) noexcept
    {
        try
        {
            std::ifstream input(path);
            if (!input)
                return true;

            auto settings = Snapshot();
            std::string line;
            while (std::getline(input, line))
            {
                const auto separator = line.find('=');
                if (separator == std::string::npos)
                    continue;

                const auto key = line.substr(0, separator);
                const auto value = line.substr(separator + 1);

                if (key == "backend.background_workers")
                    settings.backend.backgroundWorkerCount = std::stoull(value);
                else if (key == "backend.max_game_jobs")
                    settings.backend.maxGameJobsPerTick = std::stoull(value);
                else if (key == "backend.max_fiber_resumes")
                    settings.backend.maxFiberResumesPerTick = std::stoull(value);
                else if (key == "backend.max_tick_micros")
                    settings.backend.maxBackendMicros = std::stoull(value);
                else if (key == "frontend.menu_scale")
                    settings.frontend.menuScale = std::stof(value);
                else if (key == "frontend.animations")
                    settings.frontend.animations = value == "1" || value == "true";
                else if (key == "frontend.toggle_key")
                    settings.frontend.toggleKey = std::stoi(value);
            }

            settings.backend.backgroundWorkerCount = std::clamp<std::size_t>(
                settings.backend.backgroundWorkerCount, 1, 4);
            settings.backend.maxGameJobsPerTick = std::clamp<std::size_t>(
                settings.backend.maxGameJobsPerTick, 1, 64);
            settings.backend.maxFiberResumesPerTick = std::clamp<std::size_t>(
                settings.backend.maxFiberResumesPerTick, 1, 32);
            settings.backend.maxBackendMicros = std::clamp<std::uint64_t>(
                settings.backend.maxBackendMicros, 50, 2000);
            settings.frontend.menuScale = std::clamp(settings.frontend.menuScale, 0.5F, 2.5F);

            {
                std::scoped_lock lock(m_Mutex);
                m_Settings = settings;
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool SettingsManager::SaveAsync(
        Tasking::ThreadPool& pool,
        const std::filesystem::path& path) const
    {
        const auto settings = Snapshot();
        return pool.Submit([settings, path]() {
            auto temporary = path;
            temporary += ".tmp";

            {
                std::ofstream output(temporary, std::ios::out | std::ios::trunc);
                if (!output)
                    return;

                output << "backend.background_workers=" << settings.backend.backgroundWorkerCount << '\n';
                output << "backend.max_game_jobs=" << settings.backend.maxGameJobsPerTick << '\n';
                output << "backend.max_fiber_resumes=" << settings.backend.maxFiberResumesPerTick << '\n';
                output << "backend.max_tick_micros=" << settings.backend.maxBackendMicros << '\n';
                output << "frontend.menu_scale=" << settings.frontend.menuScale << '\n';
                output << "frontend.animations=" << (settings.frontend.animations ? 1 : 0) << '\n';
                output << "frontend.toggle_key=" << settings.frontend.toggleKey << '\n';
                output.flush();
                if (!output)
                    return;
            }

            if (!CommitSettingsFile(temporary, path))
            {
                std::error_code error;
                std::filesystem::remove(temporary, error);
            }
        });
    }

    SettingsSnapshot SettingsManager::Snapshot() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Settings;
    }

    void SettingsManager::SetBackend(BackendSettings settings) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Settings.backend = std::move(settings);
    }

    void SettingsManager::SetFrontend(FrontendSettings settings) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Settings.frontend = std::move(settings);
    }
}
