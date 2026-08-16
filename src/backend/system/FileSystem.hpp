#pragma once

#include <filesystem>

namespace Sick::Backend::System
{
    class FileSystem final
    {
    public:
        bool Initialize(const std::filesystem::path& moduleDirectory) noexcept;

        [[nodiscard]] const std::filesystem::path& Root() const noexcept { return m_Root; }
        [[nodiscard]] const std::filesystem::path& Logs() const noexcept { return m_Logs; }
        [[nodiscard]] const std::filesystem::path& Configs() const noexcept { return m_Configs; }
        [[nodiscard]] const std::filesystem::path& Themes() const noexcept { return m_Themes; }
        [[nodiscard]] const std::filesystem::path& Scripts() const noexcept { return m_Scripts; }
        [[nodiscard]] const std::filesystem::path& LogFile() const noexcept { return m_LogFile; }
        [[nodiscard]] const std::filesystem::path& SettingsFile() const noexcept { return m_SettingsFile; }

    private:
        std::filesystem::path m_Root;
        std::filesystem::path m_Logs;
        std::filesystem::path m_Configs;
        std::filesystem::path m_Themes;
        std::filesystem::path m_Scripts;
        std::filesystem::path m_LogFile;
        std::filesystem::path m_SettingsFile;
    };
}
