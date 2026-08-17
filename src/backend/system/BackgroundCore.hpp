#pragma once

#include "backend/BackendTypes.hpp"
#include "AssetCatalog.hpp"
#include "ConfigManager.hpp"
#include "FileSystem.hpp"
#include "IoService.hpp"
#include "SettingsManager.hpp"

#include <filesystem>

namespace Sick::Backend::System
{
    class BackgroundCore final
    {
    public:
        bool Initialize(const std::filesystem::path& moduleDirectory) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] FileSystem& Files() noexcept { return m_Files; }
        [[nodiscard]] const FileSystem& Files() const noexcept { return m_Files; }
        [[nodiscard]] SettingsManager& Settings() noexcept { return m_Settings; }
        [[nodiscard]] const SettingsManager& Settings() const noexcept { return m_Settings; }
        [[nodiscard]] ConfigManager& Configs() noexcept { return m_Configs; }
        [[nodiscard]] const ConfigManager& Configs() const noexcept { return m_Configs; }
        [[nodiscard]] AssetCatalog& Assets() noexcept { return m_Assets; }
        [[nodiscard]] const AssetCatalog& Assets() const noexcept { return m_Assets; }
        [[nodiscard]] IoService& Io() noexcept { return m_Io; }
        [[nodiscard]] const IoService& Io() const noexcept { return m_Io; }
        [[nodiscard]] BackgroundSnapshot Snapshot() const noexcept;

    private:
        FileSystem m_Files;
        SettingsManager m_Settings;
        IoService m_Io;
        ConfigManager m_Configs;
        AssetCatalog m_Assets;
        bool m_Initialized{};
    };
}
