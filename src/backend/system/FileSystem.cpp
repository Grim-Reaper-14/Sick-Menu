#include "FileSystem.hpp"

#include <array>
#include <system_error>

namespace Sick::Backend::System
{
    bool FileSystem::Initialize(const std::filesystem::path& moduleDirectory) noexcept
    {
        std::error_code error;
        auto base = moduleDirectory;
        if (base.empty())
            base = std::filesystem::current_path(error);
        if (error || base.empty())
            return false;

        m_Root = base / "SickMenu";
        m_Logs = m_Root / "logs";
        m_Configs = m_Root / "configs";
        m_Themes = m_Root / "themes";
        m_Scripts = m_Root / "scripts";
        m_LogFile = m_Logs / "SickMenu.log";
        m_SettingsFile = m_Root / "settings.cfg";

        const std::array directories{m_Root, m_Logs, m_Configs, m_Themes, m_Scripts};
        for (const auto& directory : directories)
        {
            std::filesystem::create_directories(directory, error);
            if (error)
                return false;
        }

        return true;
    }
}
