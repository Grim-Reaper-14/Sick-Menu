#include "SickMenu.hpp"

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
        : m_Controller(m_RootPage)
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

        auto godModeCallback = std::move(callbacks.godMode);
        m_PlayerPage.AddToggle(
            "GodMode",
            m_State.godMode,
            [callback = std::move(godModeCallback)](bool enabled) mutable {
                Notify(callback, enabled);
            });
        m_PlayerPage.AddToggle(
            "Beast Jump",
            m_State.beastJump,
            [callback = std::move(callbacks.beastJump)](bool enabled) mutable {
                Notify(callback, enabled);
            });
        m_PlayerPage.AddToggle(
            "Graceful Landing",
            m_State.gracefulLanding,
            [callback = std::move(callbacks.gracefulLanding)](bool enabled) mutable {
                Notify(callback, enabled);
            });

        // These pages intentionally start empty except for a non-selectable
        // marker. Their dedicated objects give future features a stable home
        // without mixing gameplay categories together.
        m_VehiclePage.AddLabel("No options yet");
        m_WeaponsPage.AddLabel("No options yet");
        m_WorldPage.AddLabel("No options yet");
        m_TeleportPage.AddLabel("No options yet");
        m_TunablesPage.AddLabel("No options yet");
        m_UnlocksPage.AddLabel("No options yet");
        m_OnlineServicesPage.AddLabel("No options yet");
        m_OnlineVehicleSpawnerPage.AddLabel("No options yet");
        m_OnlineProtectionPage.AddLabel("No options yet");
        m_MenuSettingsPage.AddLabel("No options yet");
    }

    bool SickMenu::Handle(MenuInput input)
    {
        return m_Controller.Handle(input);
    }

    void SickMenu::Open()
    {
        m_Controller.Open();
    }

    void SickMenu::Close() noexcept
    {
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

    SickMenuState& SickMenu::State() noexcept
    {
        return m_State;
    }

    const SickMenuState& SickMenu::State() const noexcept
    {
        return m_State;
    }

    MenuController& SickMenu::Controller() noexcept
    {
        return m_Controller;
    }

    const MenuController& SickMenu::Controller() const noexcept
    {
        return m_Controller;
    }

    MenuRenderer& SickMenu::Renderer() noexcept
    {
        return m_Renderer;
    }

    const MenuRenderer& SickMenu::Renderer() const noexcept
    {
        return m_Renderer;
    }

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

    MenuPage& SickMenu::SelfPage() noexcept
    {
        return m_PlayerPage;
    }

    const MenuPage& SickMenu::SelfPage() const noexcept
    {
        return m_PlayerPage;
    }
}
