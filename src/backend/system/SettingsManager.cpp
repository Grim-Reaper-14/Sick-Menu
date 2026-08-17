#include "SettingsManager.hpp"

#include "FileSystem.hpp"
#include "IoService.hpp"
#include "LoggerApi.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

namespace Sick::Backend::System
{
    bool SettingsManager::Load(FileSystem& files) noexcept
    {
        try
        {
            if (!files.Exists(FileArea::Root, "settings.cfg"))
                return true;
            const auto contents = files.ReadText(FileArea::Root, "settings.cfg");
            if (!contents)
                return false;

            auto settings = Snapshot();
            std::istringstream input(*contents);
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
                else if (key == "frontend.menu_left")
                    settings.frontend.menuLeft = std::stof(value);
                else if (key == "frontend.menu_top")
                    settings.frontend.menuTop = std::stof(value);
                else if (key == "frontend.animations")
                    settings.frontend.animations = value == "1" || value == "true";
                else if (key == "frontend.toggle_key")
                    settings.frontend.toggleKey = std::stoi(value);
                else if (key == "frontend.theme")
                    settings.frontend.theme = value;
                else if (key == "frontend.banner")
                    settings.frontend.banner = value;
                else if (key == "frontend.font")
                    settings.frontend.font = value;
            }

            settings.backend.backgroundWorkerCount = std::clamp<std::size_t>(settings.backend.backgroundWorkerCount, 1, 4);
            settings.backend.maxGameJobsPerTick = std::clamp<std::size_t>(settings.backend.maxGameJobsPerTick, 1, 64);
            settings.backend.maxFiberResumesPerTick = std::clamp<std::size_t>(settings.backend.maxFiberResumesPerTick, 1, 32);
            settings.backend.maxBackendMicros = std::clamp<std::uint64_t>(settings.backend.maxBackendMicros, 50, 2000);
            settings.frontend.menuScale = std::clamp(settings.frontend.menuScale, 0.5F, 2.5F);
            settings.frontend.menuLeft = std::clamp(settings.frontend.menuLeft, -1920.0F, 3840.0F);
            settings.frontend.menuTop = std::clamp(settings.frontend.menuTop, -1080.0F, 2160.0F);

            std::scoped_lock lock(m_Mutex);
            m_Settings = std::move(settings);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool SettingsManager::SaveAsync(IoService& io, FileSystem& files) const
    {
        const auto settings = Snapshot();
        return io.Submit(IoPriority::Maintenance, [&files, settings]() {
            std::ostringstream output;
            output << "backend.background_workers=" << settings.backend.backgroundWorkerCount << '\n';
            output << "backend.max_game_jobs=" << settings.backend.maxGameJobsPerTick << '\n';
            output << "backend.max_fiber_resumes=" << settings.backend.maxFiberResumesPerTick << '\n';
            output << "backend.max_tick_micros=" << settings.backend.maxBackendMicros << '\n';
            output << "frontend.menu_scale=" << settings.frontend.menuScale << '\n';
            output << "frontend.menu_left=" << settings.frontend.menuLeft << '\n';
            output << "frontend.menu_top=" << settings.frontend.menuTop << '\n';
            output << "frontend.animations=" << (settings.frontend.animations ? 1 : 0) << '\n';
            output << "frontend.toggle_key=" << settings.frontend.toggleKey << '\n';
            output << "frontend.theme=" << settings.frontend.theme << '\n';
            output << "frontend.banner=" << settings.frontend.banner << '\n';
            output << "frontend.font=" << settings.frontend.font << '\n';
            if (!files.AtomicWriteText(FileArea::Root, "settings.cfg", output.str()))
                LoggerApi::Get().Error("settings", "Settings save failed");
        }).has_value();
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
