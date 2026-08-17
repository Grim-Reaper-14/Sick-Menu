#pragma once

#include "MenuRenderer.hpp"
#include "shared/HandlingTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Sick::Ui
{
    struct SickMenuPreferences
    {
        float scale{1.25F};
        float left{48.0F};
        float top{28.0F};
        std::string theme{"Default"};
        std::string banner;
        std::string font;
    };

    struct SickMenuAsset
    {
        std::string name;
        std::string path;
    };

    struct SickMenuTheme
    {
        SickMenuAsset asset;
        MenuColor border{35, 50, 77, 255};
        MenuColor header{0, 7, 50, 255};
        MenuColor headerBand{0, 11, 70, 255};
        MenuColor title{2, 4, 9, 255};
        MenuColor body{7, 13, 22, 255};
        MenuColor footer{2, 4, 9, 255};
        MenuColor selected{246, 246, 246, 255};
        MenuColor text{244, 244, 246, 255};
        MenuColor selectedText{16, 18, 22, 255};
        MenuColor disabledText{116, 122, 132, 255};
        MenuColor accent{226, 0, 82, 255};
        MenuColor inactiveToggle{66, 72, 84, 255};
        MenuColor logoCyan{41, 214, 255, 255};
        MenuColor logoMagenta{226, 0, 198, 255};
        MenuColor logoShadow{28, 8, 80, 255};
    };

    struct SickMenuAssetCatalog
    {
        std::uint64_t generation{};
        std::vector<SickMenuTheme> themes;
        std::vector<SickMenuAsset> images;
        std::vector<SickMenuAsset> fonts;
        std::vector<SickMenuAsset> scripts;
    };

    struct SickMenuState
    {
        bool godMode{};
        bool infiniteOxygen{};
        bool noRagdoll{};
        bool superJump{};
        bool seatBelt{};
        bool noWantedLevel{};
        int wantedLevel{};
        bool fastRun{};
        bool fastSwim{};
        bool keepPlayerClean{};
        bool aqualung{};
        bool noGravity{};
        bool waterproof{};
        bool beastJump{};
        bool gracefulLanding{};

        bool vehicleGodMode{};
        bool vehicleAutoRepair{};
        bool vehicleKeepClean{};
        bool vehicleEngineAlwaysOn{};
        bool vehicleNoGravity{};
        bool vehicleNoCollision{};

        bool handlingAvailable{};
        bool handlingVehicleAttached{};
        Handling::Values handlingValues{};

        bool demoToggle{};
        int demoNumber{1};
        std::size_t demoVector{2};
        bool moveMode{};
        int menuScalePercent{125};
        float menuLeft{48.0F};
        float menuTop{28.0F};
        std::string theme{"Default"};
        std::string banner;
        std::string font;
    };

    struct SickMenuCallbacks
    {
        std::function<void(bool)> godMode;
        std::function<void(bool)> infiniteOxygen;
        std::function<void(bool)> noRagdoll;
        std::function<void(bool)> superJump;
        std::function<void(bool)> seatBelt;
        std::function<void(bool)> noWantedLevel;
        std::function<void(int)> wantedLevel;
        std::function<void(bool)> fastRun;
        std::function<void(bool)> fastSwim;
        std::function<void(bool)> keepPlayerClean;
        std::function<void(bool)> aqualung;
        std::function<void(bool)> noGravity;
        std::function<void(bool)> waterproof;
        std::function<void(bool)> beastJump;
        std::function<void(bool)> gracefulLanding;

        std::function<void(bool)> vehicleGodMode;
        std::function<void(bool)> vehicleAutoRepair;
        std::function<void(bool)> vehicleKeepClean;
        std::function<void(bool)> vehicleEngineAlwaysOn;
        std::function<void(bool)> vehicleNoGravity;
        std::function<void(bool)> vehicleNoCollision;
        std::function<void()> repairVehicle;
        std::function<void()> cleanVehicle;
        std::function<void()> putVehicleOnGround;

        std::function<void(Handling::Field, float)> handlingValue;
        std::function<void()> restoreOriginalHandling;
        std::function<void()> saveHandlingProfile;
        std::function<void(const std::string&)> loadHandlingProfile;
        std::function<void()> refreshHandlingProfiles;

        std::function<void()> regularAction;
        std::function<void(bool)> demoToggle;
        std::function<void(int)> demoNumber;
        std::function<void(std::size_t)> demoVector;
        std::function<void(const SickMenuPreferences&)> preferencesChanged;
        std::function<void()> refreshAssets;
        std::function<void()> saveConfiguration;
        std::function<void()> exitGta;
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

        void SetPreferences(SickMenuPreferences preferences) noexcept;
        [[nodiscard]] SickMenuPreferences Preferences() const;
        void SetAssetCatalog(SickMenuAssetCatalog catalog);
        void SetHandlingProfiles(std::uint64_t generation, std::vector<std::string> profiles);
        [[nodiscard]] bool IsHandlingPageActive() const noexcept;
        [[nodiscard]] std::uint64_t AssetGeneration() const noexcept { return m_Assets.generation; }
        [[nodiscard]] std::uint64_t HandlingProfileGeneration() const noexcept { return m_HandlingProfileGeneration; }
        [[nodiscard]] std::string SelectedBannerPath() const;
        [[nodiscard]] std::string SelectedFontPath() const;

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
        [[nodiscard]] MenuPage& HandlingPage() noexcept { return m_HandlingPage; }
        [[nodiscard]] const MenuPage& HandlingPage() const noexcept { return m_HandlingPage; }
        [[nodiscard]] MenuPage& HandlingGeneralPage() noexcept { return m_HandlingGeneralPage; }
        [[nodiscard]] const MenuPage& HandlingGeneralPage() const noexcept { return m_HandlingGeneralPage; }
        [[nodiscard]] MenuPage& HandlingProfilesPage() noexcept { return m_HandlingProfilesPage; }
        [[nodiscard]] const MenuPage& HandlingProfilesPage() const noexcept { return m_HandlingProfilesPage; }
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
        [[nodiscard]] MenuPage& ThemesPage() noexcept { return m_ThemesPage; }
        [[nodiscard]] MenuPage& ImageLoaderPage() noexcept { return m_ImageLoaderPage; }
        [[nodiscard]] MenuPage& FontsPage() noexcept { return m_FontsPage; }
        [[nodiscard]] MenuPage& ScriptsPage() noexcept { return m_ScriptsPage; }
        [[nodiscard]] MenuPage& ControlsPage() noexcept { return m_ControlsPage; }
        [[nodiscard]] MenuPage& SelfPage() noexcept;
        [[nodiscard]] const MenuPage& SelfPage() const noexcept;

    private:
        void BuildHandlingPages();
        void RebuildHandlingProfiles();
        void BuildSettingsPages();
        void RebuildAssetPages();
        void ApplyLayout() noexcept;
        void ApplySelectedTheme() noexcept;
        void ApplyTheme(const SickMenuTheme& theme) noexcept;
        void NotifyPreferences() noexcept;
        [[nodiscard]] const SickMenuAsset* FindAsset(
            const std::vector<SickMenuAsset>& assets,
            const std::string& name) const noexcept;

        SickMenuCallbacks m_Callbacks;
        SickMenuState m_State;
        SickMenuAssetCatalog m_Assets;
        std::vector<std::string> m_HandlingProfiles;
        std::uint64_t m_HandlingProfileGeneration{};
        MenuStyle m_DefaultStyle{};
        MenuPage m_RootPage{"SICK MENU"};
        MenuPage m_PlayerPage{"PLAYER"};
        MenuPage m_VehiclePage{"VEHICLE"};
        MenuPage m_HandlingPage{"HANDLING"};
        MenuPage m_HandlingGeneralPage{"HANDLING / GENERAL"};
        MenuPage m_HandlingTransmissionPage{"HANDLING / TRANSMISSION"};
        MenuPage m_HandlingBrakesPage{"HANDLING / BRAKES"};
        MenuPage m_HandlingSteeringPage{"HANDLING / STEERING"};
        MenuPage m_HandlingTractionPage{"HANDLING / TRACTION"};
        MenuPage m_HandlingSuspensionPage{"HANDLING / SUSPENSION"};
        MenuPage m_HandlingAntiRollPage{"HANDLING / ANTI-ROLL BARS"};
        MenuPage m_HandlingRollCentrePage{"HANDLING / ROLL CENTRE"};
        MenuPage m_HandlingOtherPage{"HANDLING / OTHER"};
        MenuPage m_HandlingProfilesPage{"HANDLING PROFILES"};
        MenuPage m_WeaponsPage{"WEAPONS"};
        MenuPage m_WorldPage{"WORLD"};
        MenuPage m_TeleportPage{"TELEPORT"};
        MenuPage m_TunablesPage{"TUNABLES"};
        MenuPage m_UnlocksPage{"UNLOCKS"};
        MenuPage m_OnlineServicesPage{"ONLINE SERVICES"};
        MenuPage m_OnlineVehicleSpawnerPage{"ONLINE VEHICLE SPAWNER"};
        MenuPage m_OnlineProtectionPage{"ONLINE PROTECTION"};
        MenuPage m_MenuSettingsPage{"MENU SETTINGS"};
        MenuPage m_ThemesPage{"THEMES"};
        MenuPage m_ImageLoaderPage{"IMAGE LOADER"};
        MenuPage m_FontsPage{"FONTS"};
        MenuPage m_ScriptsPage{"LUA SCRIPTS"};
        MenuPage m_ControlsPage{"CONTROLS"};
        MenuPage m_ExitGtaPage{"EXIT GTA"};
        std::unordered_map<std::string, std::unique_ptr<MenuPage>> m_ScriptDetailPages;
        MenuController m_Controller;
        MenuRenderer m_Renderer;
        MenuTexture m_HeaderTexture{};
    };
}
