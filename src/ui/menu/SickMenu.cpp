#include "SickMenu.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    template <typename Callback, typename... Arguments>
    void Notify(Callback& callback, Arguments&&... arguments)
    {
        if (callback)
            callback(std::forward<Arguments>(arguments)...);
    }
}

namespace Sick::Ui
{
    SickMenu::SickMenu(SickMenuCallbacks callbacks)
        : m_Callbacks(std::move(callbacks)),
          m_Controller(m_RootPage)
    {
        m_RootPage.AddSubmenu("Player", m_PlayerPage);
        m_RootPage.AddSubmenu("Vehicle", m_VehiclePage);
        m_RootPage.AddSubmenu("Weapons", m_WeaponsPage);
        m_RootPage.AddSubmenu("World", m_WorldPage);
        m_RootPage.AddSubmenu("Teleport", m_TeleportPage);
        m_RootPage.AddSubmenu("Tunables", m_TunablesPage);
        m_RootPage.AddSubmenu("Unlocks", m_UnlocksPage);
        m_RootPage.AddSubmenu("Online Services", m_OnlineServicesPage);
        m_RootPage.AddSubmenu("Online Vehicle Spawner", m_OnlineVehicleSpawnerPage);
        m_RootPage.AddSubmenu("Online Protection", m_OnlineProtectionPage);
        m_RootPage.AddSubmenu("Menu Settings", m_MenuSettingsPage);

        m_PlayerPage.AddToggle("GodMode", m_State.godMode, [this](bool enabled) {
            Notify(m_Callbacks.godMode, enabled);
        });
        m_PlayerPage.AddToggle("Beast Jump", m_State.beastJump, [this](bool enabled) {
            Notify(m_Callbacks.beastJump, enabled);
        });
        m_PlayerPage.AddToggle("Graceful Landing", m_State.gracefulLanding, [this](bool enabled) {
            Notify(m_Callbacks.gracefulLanding, enabled);
        });

        m_VehiclePage.AddLabel("No options yet");
        m_WeaponsPage.AddLabel("No options yet");
        m_WorldPage.AddLabel("No options yet");
        m_TeleportPage.AddLabel("No options yet");
        m_TunablesPage.AddLabel("No options yet");
        m_UnlocksPage.AddLabel("No options yet");
        m_OnlineServicesPage.AddLabel("No options yet");
        m_OnlineVehicleSpawnerPage.AddLabel("No options yet");
        m_OnlineProtectionPage.AddLabel("No options yet");

        BuildSettingsPages();
        RebuildAssetPages();
        ApplyLayout();
    }

    void SickMenu::BuildSettingsPages()
    {
        m_MenuSettingsPage.AddSubmenu("Themes", m_ThemesPage);
        m_MenuSettingsPage.AddSubmenu("Image Loader", m_ImageLoaderPage);
        m_MenuSettingsPage.AddSubmenu("Fonts", m_FontsPage);
        m_MenuSettingsPage.AddSubmenu("Lua Scripts", m_ScriptsPage);
        m_MenuSettingsPage.AddToggle("Move Menu", m_State.moveMode);
        m_MenuSettingsPage.AddInteger(
            "Menu Size",
            m_State.menuScalePercent,
            50,
            250,
            5,
            [this](int) {
                ApplyLayout();
                NotifyPreferences();
            });
        m_MenuSettingsPage.AddSubmenu("Controls", m_ControlsPage);
        m_MenuSettingsPage.AddAction("Save Configuration", [this]() {
            NotifyPreferences();
            Notify(m_Callbacks.saveConfiguration);
        });
        m_MenuSettingsPage.AddAction("Exit Menu", [this]() {
            m_State.moveMode = false;
            m_Controller.Close();
        });
        m_MenuSettingsPage.AddSubmenu("Exit GTA", m_ExitGtaPage);

        m_ControlsPage.AddLabel("F4 - Open / Close");
        m_ControlsPage.AddLabel("Numpad 8 - Up");
        m_ControlsPage.AddLabel("Numpad 2 - Down");
        m_ControlsPage.AddLabel("Numpad 4 - Left");
        m_ControlsPage.AddLabel("Numpad 6 - Right");
        m_ControlsPage.AddLabel("Numpad 5 - Select");
        m_ControlsPage.AddLabel("Backspace - Back");

        m_ExitGtaPage.AddLabel("Are you sure?");
        m_ExitGtaPage.AddAction("Cancel", [this]() {
            static_cast<void>(m_Controller.Handle(MenuInput::Back));
        });
        m_ExitGtaPage.AddAction("Exit GTA", [this]() {
            Notify(m_Callbacks.exitGta);
            m_Controller.Close();
        });
    }

    void SickMenu::RebuildAssetPages()
    {
        m_ThemesPage.Options().clear();
        m_ThemesPage.AddAction("Default", [this]() {
            m_State.theme = "Default";
            ApplySelectedTheme();
            NotifyPreferences();
        });
        for (const auto& theme : m_Assets.themes)
        {
            const auto name = theme.asset.name;
            m_ThemesPage.AddAction(name, [this, name]() {
                m_State.theme = name;
                ApplySelectedTheme();
                NotifyPreferences();
            });
        }
        m_ThemesPage.AddAction("Reload Themes", [this]() { Notify(m_Callbacks.refreshAssets); });

        m_ImageLoaderPage.Options().clear();
        m_ImageLoaderPage.AddAction("Built-in Header", [this]() {
            m_State.banner.clear();
            NotifyPreferences();
        });
        for (const auto& image : m_Assets.images)
        {
            const auto name = image.name;
            m_ImageLoaderPage.AddAction(name, [this, name]() {
                m_State.banner = name;
                NotifyPreferences();
            });
        }
        m_ImageLoaderPage.AddAction("Reload Images", [this]() { Notify(m_Callbacks.refreshAssets); });

        m_FontsPage.Options().clear();
        m_FontsPage.AddAction("Default ImGui Font", [this]() {
            m_State.font.clear();
            NotifyPreferences();
        });
        for (const auto& font : m_Assets.fonts)
        {
            const auto name = font.name;
            m_FontsPage.AddAction(name, [this, name]() {
                m_State.font = name;
                NotifyPreferences();
            });
        }
        m_FontsPage.AddAction("Reload Fonts", [this]() { Notify(m_Callbacks.refreshAssets); });

        m_ScriptsPage.Options().clear();
        if (m_Assets.scripts.empty())
            m_ScriptsPage.AddLabel("No Lua scripts found");
        for (const auto& script : m_Assets.scripts)
        {
            auto& page = m_ScriptDetailPages[script.path];
            if (!page)
            {
                page = std::make_unique<MenuPage>(script.name);
                page->AddLabel("Detected from SickMenu/scripts");
                page->AddLabel("Lua bindings reserved for final phase");
            }
            m_ScriptsPage.AddSubmenu(script.name, *page);
        }
        m_ScriptsPage.AddAction("Reload Scripts", [this]() { Notify(m_Callbacks.refreshAssets); });
    }

    bool SickMenu::Handle(MenuInput input)
    {
        if (m_State.moveMode && m_Controller.IsOpen())
        {
            constexpr float MoveStep = 8.0F;
            switch (input)
            {
            case MenuInput::Up: m_State.menuTop -= MoveStep; break;
            case MenuInput::Down: m_State.menuTop += MoveStep; break;
            case MenuInput::Left: m_State.menuLeft -= MoveStep; break;
            case MenuInput::Right: m_State.menuLeft += MoveStep; break;
            case MenuInput::Select:
            case MenuInput::Back:
                m_State.moveMode = false;
                NotifyPreferences();
                return true;
            case MenuInput::Toggle:
                m_State.moveMode = false;
                return m_Controller.Handle(input);
            }
            ApplyLayout();
            NotifyPreferences();
            return true;
        }
        return m_Controller.Handle(input);
    }

    void SickMenu::Open()
    {
        m_Controller.Open();
    }

    void SickMenu::Close() noexcept
    {
        m_State.moveMode = false;
        m_Controller.Close();
    }

    void SickMenu::SetHeaderTexture(MenuTexture texture) noexcept
    {
        m_HeaderTexture = texture;
    }

    MenuTexture SickMenu::HeaderTexture() const noexcept
    {
        return m_HeaderTexture;
    }

    MenuDrawList SickMenu::Draw(MenuViewport viewport)
    {
        return m_Renderer.Render(m_Controller, viewport, m_HeaderTexture);
    }

    void SickMenu::SetPreferences(SickMenuPreferences preferences) noexcept
    {
        m_State.menuScalePercent = std::clamp(
            static_cast<int>(std::lround(preferences.scale * 100.0F)), 50, 250);
        m_State.menuLeft = preferences.left;
        m_State.menuTop = preferences.top;
        m_State.theme = std::move(preferences.theme);
        m_State.banner = std::move(preferences.banner);
        m_State.font = std::move(preferences.font);
        ApplyLayout();
        ApplySelectedTheme();
    }

    SickMenuPreferences SickMenu::Preferences() const
    {
        return {
            .scale = static_cast<float>(m_State.menuScalePercent) / 100.0F,
            .left = m_State.menuLeft,
            .top = m_State.menuTop,
            .theme = m_State.theme,
            .banner = m_State.banner,
            .font = m_State.font,
        };
    }

    void SickMenu::SetAssetCatalog(SickMenuAssetCatalog catalog)
    {
        if (catalog.generation == m_Assets.generation)
            return;
        m_Assets = std::move(catalog);
        RebuildAssetPages();
        ApplySelectedTheme();
    }

    std::string SickMenu::SelectedBannerPath() const
    {
        const auto* asset = FindAsset(m_Assets.images, m_State.banner);
        return asset ? asset->path : std::string{};
    }

    std::string SickMenu::SelectedFontPath() const
    {
        const auto* asset = FindAsset(m_Assets.fonts, m_State.font);
        return asset ? asset->path : std::string{};
    }

    void SickMenu::ApplyLayout() noexcept
    {
        auto& style = m_Renderer.Style();
        style.left = m_State.menuLeft;
        style.top = m_State.menuTop;
        style.uiScale = std::clamp(static_cast<float>(m_State.menuScalePercent) / 100.0F, 0.5F, 2.5F);
    }

    void SickMenu::ApplySelectedTheme() noexcept
    {
        if (m_State.theme == "Default" || m_State.theme.empty())
        {
            const auto layout = m_Renderer.Style();
            m_Renderer.Style() = m_DefaultStyle;
            m_Renderer.Style().left = layout.left;
            m_Renderer.Style().top = layout.top;
            m_Renderer.Style().uiScale = layout.uiScale;
            return;
        }
        const auto it = std::find_if(m_Assets.themes.begin(), m_Assets.themes.end(), [this](const SickMenuTheme& theme) {
            return theme.asset.name == m_State.theme;
        });
        if (it != m_Assets.themes.end())
            ApplyTheme(*it);
    }

    void SickMenu::ApplyTheme(const SickMenuTheme& theme) noexcept
    {
        auto& style = m_Renderer.Style();
        style.borderColor = theme.border;
        style.headerColor = theme.header;
        style.headerBandColor = theme.headerBand;
        style.titleColor = theme.title;
        style.bodyColor = theme.body;
        style.footerColor = theme.footer;
        style.selectedColor = theme.selected;
        style.textColor = theme.text;
        style.selectedTextColor = theme.selectedText;
        style.disabledTextColor = theme.disabledText;
        style.accentColor = theme.accent;
        style.inactiveToggleColor = theme.inactiveToggle;
        style.logoCyan = theme.logoCyan;
        style.logoMagenta = theme.logoMagenta;
        style.logoShadow = theme.logoShadow;
    }

    void SickMenu::NotifyPreferences() noexcept
    {
        if (!m_Callbacks.preferencesChanged)
            return;
        try
        {
            m_Callbacks.preferencesChanged(Preferences());
        }
        catch (...)
        {
        }
    }

    const SickMenuAsset* SickMenu::FindAsset(
        const std::vector<SickMenuAsset>& assets,
        const std::string& name) const noexcept
    {
        const auto it = std::find_if(assets.begin(), assets.end(), [&name](const SickMenuAsset& asset) {
            return asset.name == name;
        });
        return it == assets.end() ? nullptr : &*it;
    }

    SickMenuState& SickMenu::State() noexcept { return m_State; }
    const SickMenuState& SickMenu::State() const noexcept { return m_State; }
    MenuController& SickMenu::Controller() noexcept { return m_Controller; }
    const MenuController& SickMenu::Controller() const noexcept { return m_Controller; }
    MenuRenderer& SickMenu::Renderer() noexcept { return m_Renderer; }
    const MenuRenderer& SickMenu::Renderer() const noexcept { return m_Renderer; }

    MenuPage& SickMenu::RootPage() noexcept { return m_RootPage; }
    const MenuPage& SickMenu::RootPage() const noexcept { return m_RootPage; }
    MenuPage& SickMenu::PlayerPage() noexcept { return m_PlayerPage; }
    const MenuPage& SickMenu::PlayerPage() const noexcept { return m_PlayerPage; }
    MenuPage& SickMenu::VehiclePage() noexcept { return m_VehiclePage; }
    const MenuPage& SickMenu::VehiclePage() const noexcept { return m_VehiclePage; }
    MenuPage& SickMenu::WeaponsPage() noexcept { return m_WeaponsPage; }
    const MenuPage& SickMenu::WeaponsPage() const noexcept { return m_WeaponsPage; }
    MenuPage& SickMenu::WorldPage() noexcept { return m_WorldPage; }
    const MenuPage& SickMenu::WorldPage() const noexcept { return m_WorldPage; }
    MenuPage& SickMenu::TeleportPage() noexcept { return m_TeleportPage; }
    const MenuPage& SickMenu::TeleportPage() const noexcept { return m_TeleportPage; }
    MenuPage& SickMenu::TunablesPage() noexcept { return m_TunablesPage; }
    const MenuPage& SickMenu::TunablesPage() const noexcept { return m_TunablesPage; }
    MenuPage& SickMenu::UnlocksPage() noexcept { return m_UnlocksPage; }
    const MenuPage& SickMenu::UnlocksPage() const noexcept { return m_UnlocksPage; }
    MenuPage& SickMenu::OnlineServicesPage() noexcept { return m_OnlineServicesPage; }
    const MenuPage& SickMenu::OnlineServicesPage() const noexcept { return m_OnlineServicesPage; }
    MenuPage& SickMenu::OnlineVehicleSpawnerPage() noexcept { return m_OnlineVehicleSpawnerPage; }
    const MenuPage& SickMenu::OnlineVehicleSpawnerPage() const noexcept { return m_OnlineVehicleSpawnerPage; }
    MenuPage& SickMenu::OnlineProtectionPage() noexcept { return m_OnlineProtectionPage; }
    const MenuPage& SickMenu::OnlineProtectionPage() const noexcept { return m_OnlineProtectionPage; }
    MenuPage& SickMenu::MenuSettingsPage() noexcept { return m_MenuSettingsPage; }
    const MenuPage& SickMenu::MenuSettingsPage() const noexcept { return m_MenuSettingsPage; }
    MenuPage& SickMenu::SelfPage() noexcept { return m_PlayerPage; }
    const MenuPage& SickMenu::SelfPage() const noexcept { return m_PlayerPage; }
}
