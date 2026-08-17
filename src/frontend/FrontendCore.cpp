#include "FrontendCore.hpp"

#include "backend/BackendApi.hpp"

#include <utility>

namespace
{
    Sick::Ui::MenuColor UnpackColor(std::uint32_t value) noexcept
    {
        return {
            static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
            static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
            static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
            static_cast<std::uint8_t>(value & 0xFFU),
        };
    }

    Sick::Ui::SickMenuAsset ConvertAsset(const Sick::Backend::AssetEntry& asset)
    {
        return {asset.name, asset.path};
    }

    Sick::Ui::SickMenuAssetCatalog ConvertCatalog(const Sick::Backend::AssetCatalogSnapshot& source)
    {
        Sick::Ui::SickMenuAssetCatalog result{};
        result.generation = source.generation;
        result.themes.reserve(source.themes.size());
        result.images.reserve(source.images.size());
        result.fonts.reserve(source.fonts.size());
        result.scripts.reserve(source.scripts.size());

        for (const auto& theme : source.themes)
        {
            result.themes.push_back({
                .asset = ConvertAsset(theme.asset),
                .border = UnpackColor(theme.palette.border),
                .header = UnpackColor(theme.palette.header),
                .headerBand = UnpackColor(theme.palette.headerBand),
                .title = UnpackColor(theme.palette.title),
                .body = UnpackColor(theme.palette.body),
                .footer = UnpackColor(theme.palette.footer),
                .selected = UnpackColor(theme.palette.selected),
                .text = UnpackColor(theme.palette.text),
                .selectedText = UnpackColor(theme.palette.selectedText),
                .disabledText = UnpackColor(theme.palette.disabledText),
                .accent = UnpackColor(theme.palette.accent),
                .inactiveToggle = UnpackColor(theme.palette.inactiveToggle),
                .logoCyan = UnpackColor(theme.palette.logoCyan),
                .logoMagenta = UnpackColor(theme.palette.logoMagenta),
                .logoShadow = UnpackColor(theme.palette.logoShadow),
            });
        }
        for (const auto& asset : source.images)
            result.images.push_back(ConvertAsset(asset));
        for (const auto& asset : source.fonts)
            result.fonts.push_back(ConvertAsset(asset));
        for (const auto& asset : source.scripts)
            result.scripts.push_back(ConvertAsset(asset));
        return result;
    }

    Sick::Ui::SickMenuCallbacks MakeCallbacks()
    {
        Sick::Ui::SickMenuCallbacks callbacks{};
        callbacks.godMode = [](bool enabled) {
            Sick::Backend::BackendApi::Get().SetGodMode(enabled);
        };
        callbacks.regularAction = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().RunScriptVmTest());
        };
        callbacks.preferencesChanged = [](const Sick::Ui::SickMenuPreferences& preferences) {
            Sick::Backend::BackendApi::Get().SetPreferences({
                .scale = preferences.scale,
                .left = preferences.left,
                .top = preferences.top,
                .theme = preferences.theme,
                .banner = preferences.banner,
                .font = preferences.font,
            });
        };
        callbacks.refreshAssets = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().RefreshAssets());
        };
        callbacks.saveConfiguration = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().SaveConfiguration());
        };
        callbacks.exitGta = [] {
            Sick::Backend::BackendApi::Get().RequestExitGta();
        };
        return callbacks;
    }
}

namespace Sick::Frontend
{
    FrontendCore::FrontendCore()
        : m_Menu(MakeCallbacks())
    {
        Tick();
    }

    void FrontendCore::Tick() noexcept
    {
        auto& backend = Backend::BackendApi::Get();
        const auto snapshot = backend.Snapshot();
        m_Menu.State().godMode = snapshot.player.godMode.requested;

        if (!m_PreferencesLoaded && snapshot.initialized)
        {
            const auto preferences = backend.Preferences();
            m_Menu.SetPreferences({
                .scale = preferences.scale,
                .left = preferences.left,
                .top = preferences.top,
                .theme = preferences.theme,
                .banner = preferences.banner,
                .font = preferences.font,
            });
            m_PreferencesLoaded = true;
        }

        const auto generation = backend.AssetGeneration();
        if (generation != 0 && generation != m_AssetGeneration)
        {
            auto catalog = ConvertCatalog(backend.Assets());
            m_AssetGeneration = catalog.generation;
            m_Menu.SetAssetCatalog(std::move(catalog));
        }
    }
}
