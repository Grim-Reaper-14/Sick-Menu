#pragma once

#include "backend/BackendTypes.hpp"
#include "AssetCatalog.hpp"
#include "ConfigManager.hpp"
#include "FileSystem.hpp"
#include "HandlingProfileManager.hpp"
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
        [[nodiscard]] HandlingProfileManager& HandlingProfiles() noexcept { return m_HandlingProfiles; }
        [[nodiscard]] const HandlingProfileManager& HandlingProfiles() const noexcept { return m_HandlingProfiles; }
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
        HandlingProfileManager m_HandlingProfiles;
        AssetCatalog m_Assets;
        bool m_Initialized{};
    };
}
