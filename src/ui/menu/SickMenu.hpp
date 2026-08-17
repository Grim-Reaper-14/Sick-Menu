#pragma once

#include "MenuRenderer.hpp"

#include <cstddef>
#include <functional>

namespace Sick::Ui
{
    struct SickMenuState
    {
        bool godMode{};
        bool beastJump{};
        bool gracefulLanding{};
        bool demoToggle{};
        int demoNumber{1};
        std::size_t demoVector{2};
    };

    // Frontend notifications execute on the frontend/render thread. They must
    // only submit intent to BackendApi; GTA natives and script functions are
    // executed by BackendCore on the game thread.
    struct SickMenuCallbacks
    {
        std::function<void(bool)> godMode;
        std::function<void(bool)> beastJump;
        std::function<void(bool)> gracefulLanding;
        std::function<void()> regularAction;
        std::function<void(bool)> demoToggle;
        std::function<void(int)> demoNumber;
        std::function<void(std::size_t)> demoVector;
    };

    class SickMenu final
    {
    public:
        explicit SickMenu(SickMenuCallbacks callbacks = {});

        SickMenu(const SickMenu&) = delete;
        SickMenu& operator=(const SickMenu&) = delete;
        SickMenu(SickMenu&&) = delete;
        SickMenu& operator=(SickMenu&&) = delete;

        bool Handle(MenuInput input);
        void Open();
        void Close() noexcept;

        void SetHeaderTexture(MenuTexture texture) noexcept;
        [[nodiscard]] MenuTexture HeaderTexture() const noexcept;
        [[nodiscard]] MenuDrawList Draw(MenuViewport viewport);

        [[nodiscard]] SickMenuState& State() noexcept;
        [[nodiscard]] const SickMenuState& State() const noexcept;
        [[nodiscard]] MenuController& Controller() noexcept;
        [[nodiscard]] const MenuController& Controller() const noexcept;
        [[nodiscard]] MenuRenderer& Renderer() noexcept;
        [[nodiscard]] const MenuRenderer& Renderer() const noexcept;

        [[nodiscard]] MenuPage& RootPage() noexcept;
        [[nodiscard]] const MenuPage& RootPage() const noexcept;
        [[nodiscard]] MenuPage& PlayerPage() noexcept;
        [[nodiscard]] const MenuPage& PlayerPage() const noexcept;
        [[nodiscard]] MenuPage& VehiclePage() noexcept;
        [[nodiscard]] const MenuPage& VehiclePage() const noexcept;
        [[nodiscard]] MenuPage& WeaponsPage() noexcept;
        [[nodiscard]] const MenuPage& WeaponsPage() const noexcept;
        [[nodiscard]] MenuPage& WorldPage() noexcept;
        [[nodiscard]] const MenuPage& WorldPage() const noexcept;
        [[nodiscard]] MenuPage& TeleportPage() noexcept;
        [[nodiscard]] const MenuPage& TeleportPage() const noexcept;
        [[nodiscard]] MenuPage& TunablesPage() noexcept;
        [[nodiscard]] const MenuPage& TunablesPage() const noexcept;
        [[nodiscard]] MenuPage& UnlocksPage() noexcept;
        [[nodiscard]] const MenuPage& UnlocksPage() const noexcept;
        [[nodiscard]] MenuPage& OnlineServicesPage() noexcept;
        [[nodiscard]] const MenuPage& OnlineServicesPage() const noexcept;
        [[nodiscard]] MenuPage& OnlineVehicleSpawnerPage() noexcept;
        [[nodiscard]] const MenuPage& OnlineVehicleSpawnerPage() const noexcept;
        [[nodiscard]] MenuPage& OnlineProtectionPage() noexcept;
        [[nodiscard]] const MenuPage& OnlineProtectionPage() const noexcept;
        [[nodiscard]] MenuPage& MenuSettingsPage() noexcept;
        [[nodiscard]] const MenuPage& MenuSettingsPage() const noexcept;

        // Compatibility alias retained for code written before the dedicated
        // menu hierarchy. SelfPage is now the Player page.
        [[nodiscard]] MenuPage& SelfPage() noexcept;
        [[nodiscard]] const MenuPage& SelfPage() const noexcept;

    private:
        SickMenuState m_State;
        MenuPage m_RootPage{"SICK MENU"};
        MenuPage m_PlayerPage{"PLAYER"};
        MenuPage m_VehiclePage{"VEHICLE"};
        MenuPage m_WeaponsPage{"WEAPONS"};
        MenuPage m_WorldPage{"WORLD"};
        MenuPage m_TeleportPage{"TELEPORT"};
        MenuPage m_TunablesPage{"TUNABLES"};
        MenuPage m_UnlocksPage{"UNLOCKS"};
        MenuPage m_OnlineServicesPage{"ONLINE SERVICES"};
        MenuPage m_OnlineVehicleSpawnerPage{"ONLINE VEHICLE SPAWNER"};
        MenuPage m_OnlineProtectionPage{"ONLINE PROTECTION"};
        MenuPage m_MenuSettingsPage{"MENU SETTINGS"};
        MenuController m_Controller;
        MenuRenderer m_Renderer;
        MenuTexture m_HeaderTexture{};
    };
}
