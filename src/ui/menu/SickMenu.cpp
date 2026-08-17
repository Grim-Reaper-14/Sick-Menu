#include "SickMenu.hpp"

#include "categories/online_vehicle_spawner/VehicleCatalog.hpp"
#include "categories/menu_settings/Controls.hpp"
#include "categories/menu_settings/ExitGta.hpp"
#include "categories/menu_settings/ExitMenu.hpp"
#include "categories/menu_settings/Fonts.hpp"
#include "categories/menu_settings/ImageLoader.hpp"
#include "categories/menu_settings/LuaScripts.hpp"
#include "categories/menu_settings/MenuSize.hpp"
#include "categories/menu_settings/MoveMenu.hpp"
#include "categories/menu_settings/SaveConfiguration.hpp"
#include "categories/menu_settings/Themes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace
{
    constexpr std::array<std::int32_t, 11> OnlineSessionValues{
        0, 1, 13, 3, 12, 2, 6, 9, 11, 10, -1,
    };

    enum class VehicleCustomizationUiCommand : std::int32_t
    {
        SetMod = 0,
        ToggleMod = 1,
        SetWheelType = 2,
        SetPrimaryModColor = 3,
        SetSecondaryModColor = 4,
        SetExtraColours = 5,
        SetInteriorColor = 6,
        SetDashboardColor = 7,
        SetNeonEnabled = 8,
        SetNeonColor = 9,
        SetTireSmokeColor = 10,
        SetXenonColor = 11,
        SetExtra = 12,
        SetBulletproofTires = 13,
        MaxVehicle = 14,
    };

    struct VehicleModSpec
    {
        const char* label;
        int slot;
    };

    constexpr std::array<VehicleModSpec, 23> VehicleModSlots{{
        {"Spoiler", 0}, {"Front Bumper", 1}, {"Rear Bumper", 2}, {"Side Skirt", 3},
        {"Exhaust", 4}, {"Frame", 5}, {"Grille", 6}, {"Hood", 7}, {"Left Fender", 8},
        {"Right Fender", 9}, {"Roof", 10}, {"Engine", 11}, {"Brakes", 12},
        {"Transmission", 13}, {"Horn", 14}, {"Suspension", 15}, {"Armor", 16},
        {"Plate Holder", 25}, {"Vanity Plate", 26}, {"Trim Design", 27}, {"Ornaments", 28},
        {"Dashboard", 29}, {"Dial Design", 30},
    }};

    const std::vector<std::string> WheelTypes{
        "Sport", "Muscle", "Lowrider", "SUV", "Offroad", "Tuner", "Bike", "High End",
        "Benny's Original", "Benny's Bespoke", "Open Wheel", "Street", "Track",
    };

    const std::vector<std::string> PaintTypes{
        "Normal", "Metallic", "Pearlescent", "Matte", "Metal", "Chrome", "Worn", "Chameleon",
    };

    const std::vector<std::string> XenonColors{
        "White", "Blue", "Electric Blue", "Mint Green", "Lime Green", "Yellow", "Golden",
        "Orange", "Red", "Pony Pink", "Hot Pink", "Purple", "Blacklight", "Stock",
    };

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
        }).Describe("Makes the local player invincible while enabled.");
        m_PlayerPage.AddToggle("Infinite Oxygen", m_State.infiniteOxygen, [this](bool enabled) {
            Notify(m_Callbacks.infiniteOxygen, enabled);
        }).Describe("Keeps underwater breathing time effectively unlimited.");
        m_PlayerPage.AddToggle("No Ragdoll", m_State.noRagdoll, [this](bool enabled) {
            Notify(m_Callbacks.noRagdoll, enabled);
        }).Describe("Prevents the local player from entering ragdoll state.");
        m_PlayerPage.AddToggle("Super Jump", m_State.superJump, [this](bool enabled) {
            Notify(m_Callbacks.superJump, enabled);
        }).Describe("Applies GTA's super-jump effect each game frame while enabled.");
        m_PlayerPage.AddToggle("Seat Belt", m_State.seatBelt, [this](bool enabled) {
            Notify(m_Callbacks.seatBelt, enabled);
        }).Describe("Reduces vehicle ejection behavior for the local player.");
        m_PlayerPage.AddToggle("No Wanted Level", m_State.noWantedLevel, [this](bool enabled) {
            Notify(m_Callbacks.noWantedLevel, enabled);
        }).Describe("Continuously clears the local player's wanted level.");
        m_PlayerPage.AddInteger("Set Wanted Level", m_State.wantedLevel, 0, 5, 1, [this](int level) {
            Notify(m_Callbacks.wantedLevel, level);
        }).Describe("Sets the requested wanted level from 0 to 5 when No Wanted Level is off.");
        m_PlayerPage.AddToggle("Fast Run", m_State.fastRun, [this](bool enabled) {
            Notify(m_Callbacks.fastRun, enabled);
        }).Describe("Raises the player run and sprint multiplier to GTA's supported maximum.");
        m_PlayerPage.AddToggle("Fast Swim", m_State.fastSwim, [this](bool enabled) {
            Notify(m_Callbacks.fastSwim, enabled);
        }).Describe("Raises the player swim multiplier to GTA's supported maximum.");
        m_PlayerPage.AddToggle("Keep Player Clean", m_State.keepPlayerClean, [this](bool enabled) {
            Notify(m_Callbacks.keepPlayerClean, enabled);
        }).Describe("Periodically clears dirt, blood and visible damage from the player.");
        m_PlayerPage.AddToggle("Aqualung", m_State.aqualung, [this](bool enabled) {
            Notify(m_Callbacks.aqualung, enabled);
        }).Describe("Enables scuba behavior and extended underwater breathing.");
        m_PlayerPage.AddToggle("No Gravity", m_State.noGravity, [this](bool enabled) {
            Notify(m_Callbacks.noGravity, enabled);
        }).Describe("Disables gravity on the local player entity.");
        m_PlayerPage.AddToggle("Waterproof", m_State.waterproof, [this](bool enabled) {
            Notify(m_Callbacks.waterproof, enabled);
        }).Describe("Prevents drowning and suppresses swimming so gravity can settle the player on the sea floor.");
        m_PlayerPage.AddToggle("Beast Jump", m_State.beastJump, [this](bool enabled) {
            Notify(m_Callbacks.beastJump, enabled);
        });
        m_PlayerPage.AddToggle("Graceful Landing", m_State.gracefulLanding, [this](bool enabled) {
            Notify(m_Callbacks.gracefulLanding, enabled);
        });

        m_VehiclePage.AddToggle("Vehicle God Mode", m_State.vehicleGodMode, [this](bool enabled) {
            Notify(m_Callbacks.vehicleGodMode, enabled);
        }).Describe("Keeps the vehicle you are using invincible while enabled.");
        m_VehiclePage.AddToggle("Auto Repair", m_State.vehicleAutoRepair, [this](bool enabled) {
            Notify(m_Callbacks.vehicleAutoRepair, enabled);
        }).Describe("Periodically restores engine, body and fuel-tank health on the current vehicle.");
        m_VehiclePage.AddToggle("Keep Vehicle Clean", m_State.vehicleKeepClean, [this](bool enabled) {
            Notify(m_Callbacks.vehicleKeepClean, enabled);
        }).Describe("Keeps the current vehicle's dirt level at zero.");
        m_VehiclePage.AddToggle("Engine Always On", m_State.vehicleEngineAlwaysOn, [this](bool enabled) {
            Notify(m_Callbacks.vehicleEngineAlwaysOn, enabled);
        }).Describe("Keeps the current vehicle engine running without forcing it off when disabled.");
        m_VehiclePage.AddToggle("No Gravity", m_State.vehicleNoGravity, [this](bool enabled) {
            Notify(m_Callbacks.vehicleNoGravity, enabled);
        }).Describe("Disables gravity on the current vehicle and restores it when the feature is released.");
        m_VehiclePage.AddToggle("No Collision", m_State.vehicleNoCollision, [this](bool enabled) {
            Notify(m_Callbacks.vehicleNoCollision, enabled);
        }).Describe("Disables collision on the current vehicle while preserving its physics state.");
        m_VehiclePage.AddSubmenu("Handling", m_HandlingPage)
            .Describe("Opens the build-aware vehicle handling editor and saved handling profiles.");
        m_VehiclePage.AddSubmenu("Customization", m_VehicleCustomizationPage)
            .Describe("Kiddion-style vehicle editor with Yim-style mod, wheel, paint, neon and extras categories.");
        m_VehiclePage.AddAction("Repair Vehicle", [this]() {
            Notify(m_Callbacks.repairVehicle);
        }).Describe("Repairs the current vehicle and restores its major health values once.");
        m_VehiclePage.AddAction("Clean Vehicle", [this]() {
            Notify(m_Callbacks.cleanVehicle);
        }).Describe("Sets the current vehicle's dirt level to zero once.");
        m_VehiclePage.AddAction("Put On Ground", [this]() {
            Notify(m_Callbacks.putVehicleOnGround);
        }).Describe("Places the current vehicle upright on all wheels using GTA's ground-placement native.");

        BuildVehicleCustomizationPages();
        BuildHandlingPages();

        m_WeaponsPage.AddLabel("No options yet");
        m_WorldPage.AddLabel("No options yet");
        m_TeleportPage.AddLabel("No options yet");
        m_TunablesPage.AddLabel("No options yet");
        m_UnlocksPage.AddLabel("No options yet");
        m_OnlineServicesPage.AddInfo("Session Status", [this]() {
            return m_State.onlineSessionStatus;
        });
        m_OnlineServicesPage.AddChoice(
            "Session Type",
            m_State.onlineSessionType,
            {
                "Public",
                "Solo Public",
                "SCTV",
                "Crew",
                "Join Crew",
                "Closed Crew",
                "Closed Friend",
                "Find Friend",
                "Invite Only",
                "Solo",
                "Leave Online",
            });
        m_OnlineServicesPage.AddAction("Switch Session", [this]() {
            if (m_State.onlineSessionType < OnlineSessionValues.size())
                Notify(m_Callbacks.switchOnlineSession, OnlineSessionValues[m_State.onlineSessionType]);
        }).EnabledWhen([this]() {
            return !m_State.onlineSessionBusy;
        }).Describe("Requests the selected GTA Online session type through shop_controller.");
        m_OnlineServicesPage.AddInfo("Garage Status", [this]() {
            return m_State.personalVehicleSaveStatus;
        });
        m_OnlineServicesPage.AddAction("Save Current Vehicle To Garage", [this]() {
            Notify(m_Callbacks.saveCurrentVehicleToPersonalGarage);
        }).EnabledWhen([this]() {
            return !m_State.personalVehicleSaveBusy;
        }).Describe("Opens GTA's personal-vehicle garage save flow for the vehicle you are currently driving.");
        BuildVehicleSpawnerPages();
        m_OnlineProtectionPage.AddLabel("No options yet");

        BuildSettingsPages();
        RebuildAssetPages();
        ApplyLayout();
    }

    void SickMenu::BuildVehicleCustomizationPages()
    {
        const auto command = [this](VehicleCustomizationUiCommand action, int a = 0, int b = 0, int c = 0, int d = 0) {
            Notify(m_Callbacks.customizeVehicle, static_cast<std::int32_t>(action), a, b, c, d);
        };

        m_VehicleCustomizationPage.AddSubmenu("Mods", m_VehicleModsPage)
            .Describe("Body, performance and interior mod slots for the vehicle you are currently driving.");
        m_VehicleCustomizationPage.AddSubmenu("Wheels", m_VehicleWheelsPage)
            .Describe("Wheel family, wheel indexes and bulletproof tire controls.");
        m_VehicleCustomizationPage.AddSubmenu("Paint", m_VehiclePaintPage)
            .Describe("Primary, secondary, Worn/Chameleon, pearlescent, wheel, interior and dashboard colors.");
        m_VehicleCustomizationPage.AddSubmenu("Lights & Neon", m_VehicleLightsPage)
            .Describe("Xenon headlights, neon zones/colors and tire-smoke RGB.");
        m_VehicleCustomizationPage.AddSubmenu("Extras", m_VehicleExtrasPage)
            .Describe("Toggles supported vehicle extras 1 through 14.");
        m_VehicleCustomizationPage.AddAction("Max Vehicle", [command]() mutable {
            command(VehicleCustomizationUiCommand::MaxVehicle);
        }).Describe("Applies the highest available mod in each supported slot, enables turbo/xenon and bulletproof tires.");

        for (const auto& spec : VehicleModSlots)
        {
            m_VehicleModsPage.AddInteger(
                spec.label,
                m_State.vehicleMods[static_cast<std::size_t>(spec.slot)],
                -1,
                100,
                1,
                [command, slot = spec.slot](int index) mutable {
                    command(VehicleCustomizationUiCommand::SetMod, slot, index, 0, 0);
                })
                .Describe("-1 restores the stock part; values above the vehicle's real mod count are safely clamped by the backend.");
        }
        m_VehicleModsPage.AddToggle("Turbo", m_State.vehicleTurbo, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 18, enabled ? 1 : 0, 0, 0);
        });

        m_VehicleWheelsPage.AddChoice("Wheel Type", m_State.vehicleWheelType, WheelTypes, [command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetWheelType, static_cast<int>(index), 0, 0, 0);
        });
        m_VehicleWheelsPage.AddInteger("Front Wheel", m_State.vehicleMods[23], -1, 100, 1, [command](int index) mutable {
            command(VehicleCustomizationUiCommand::SetMod, 23, index, 0, 0);
        });
        m_VehicleWheelsPage.AddInteger("Rear Wheel", m_State.vehicleMods[24], -1, 100, 1, [command](int index) mutable {
            command(VehicleCustomizationUiCommand::SetMod, 24, index, 0, 0);
        });
        m_VehicleWheelsPage.AddToggle("Bulletproof Tires", m_State.vehicleBulletproofTires, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::SetBulletproofTires, enabled ? 1 : 0, 0, 0, 0);
        });

        m_VehiclePaintPage.AddChoice("Primary Paint Type", m_State.vehiclePrimaryPaintType, PaintTypes, [this, command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetPrimaryModColor, static_cast<int>(index), m_State.vehiclePrimaryColor, m_State.vehicleSecondaryColor, 0);
        });
        m_VehiclePaintPage.AddInteger("Primary Color", m_State.vehiclePrimaryColor, 0, 222, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetPrimaryModColor, static_cast<int>(m_State.vehiclePrimaryPaintType), color, m_State.vehicleSecondaryColor, 0);
        }).Describe("Legacy colors use 0-160. Chameleon colors use GTA Enhanced IDs 161-222; Worn uses its legacy Worn color IDs.");
        m_VehiclePaintPage.AddChoice("Secondary Paint Type", m_State.vehicleSecondaryPaintType, PaintTypes, [this, command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetSecondaryModColor, static_cast<int>(index), m_State.vehicleSecondaryColor, m_State.vehiclePrimaryColor, 0);
        });
        m_VehiclePaintPage.AddInteger("Secondary Color", m_State.vehicleSecondaryColor, 0, 222, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetSecondaryModColor, static_cast<int>(m_State.vehicleSecondaryPaintType), color, m_State.vehiclePrimaryColor, 0);
        }).Describe("Legacy colors use 0-160. Chameleon colors use GTA Enhanced IDs 161-222; Worn uses its legacy Worn color IDs.");
        m_VehiclePaintPage.AddInteger("Pearlescent Color", m_State.vehiclePearlescentColor, 0, 160, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetExtraColours, color, m_State.vehicleWheelColor, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Wheel Color", m_State.vehicleWheelColor, 0, 222, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetExtraColours, m_State.vehiclePearlescentColor, color, 0, 0);
        }).Describe("Supports standard wheel colors and GTA Enhanced Chameleon color IDs 161-222.");
        m_VehiclePaintPage.AddInteger("Interior Color", m_State.vehicleInteriorColor, 0, 160, 1, [command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetInteriorColor, color, 0, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Dashboard Color", m_State.vehicleDashboardColor, 0, 160, 1, [command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetDashboardColor, color, 0, 0, 0);
        });

        m_VehicleLightsPage.AddToggle("Xenon Headlights", m_State.vehicleXenon, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 22, enabled ? 1 : 0, 0, 0);
        });
        m_VehicleLightsPage.AddChoice("Xenon Color", m_State.vehicleXenonColor, XenonColors, [command](std::size_t index) mutable {
            const int nativeIndex = index + 1 == XenonColors.size() ? -1 : static_cast<int>(index);
            command(VehicleCustomizationUiCommand::SetXenonColor, nativeIndex, 0, 0, 0);
        });
        constexpr std::array<const char*, 4> NeonLabels{"Neon Left", "Neon Right", "Neon Front", "Neon Back"};
        for (std::size_t index = 0; index < NeonLabels.size(); ++index)
        {
            m_VehicleLightsPage.AddToggle(NeonLabels[index], m_State.vehicleNeonEnabled[index], [command, index](bool enabled) mutable {
                command(VehicleCustomizationUiCommand::SetNeonEnabled, static_cast<int>(index), enabled ? 1 : 0, 0, 0);
            });
        }
        m_VehicleLightsPage.AddInteger("Neon Red", m_State.vehicleNeonRed, 0, 255);
        m_VehicleLightsPage.AddInteger("Neon Green", m_State.vehicleNeonGreen, 0, 255);
        m_VehicleLightsPage.AddInteger("Neon Blue", m_State.vehicleNeonBlue, 0, 255);
        m_VehicleLightsPage.AddAction("Apply Neon Color", [this, command]() mutable {
            command(VehicleCustomizationUiCommand::SetNeonColor, m_State.vehicleNeonRed, m_State.vehicleNeonGreen, m_State.vehicleNeonBlue, 0);
        });
        m_VehicleLightsPage.AddToggle("Tire Smoke", m_State.vehicleTireSmoke, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 20, enabled ? 1 : 0, 0, 0);
        });
        m_VehicleLightsPage.AddInteger("Smoke Red", m_State.vehicleSmokeRed, 0, 255);
        m_VehicleLightsPage.AddInteger("Smoke Green", m_State.vehicleSmokeGreen, 0, 255);
        m_VehicleLightsPage.AddInteger("Smoke Blue", m_State.vehicleSmokeBlue, 0, 255);
        m_VehicleLightsPage.AddAction("Apply Tire Smoke Color", [this, command]() mutable {
            command(VehicleCustomizationUiCommand::SetTireSmokeColor, m_State.vehicleSmokeRed, m_State.vehicleSmokeGreen, m_State.vehicleSmokeBlue, 0);
        });

        for (std::size_t index = 0; index < m_State.vehicleExtras.size(); ++index)
        {
            const int extraId = static_cast<int>(index) + 1;
            m_VehicleExtrasPage.AddToggle("Extra " + std::to_string(extraId), m_State.vehicleExtras[index], [command, extraId](bool enabled) mutable {
                command(VehicleCustomizationUiCommand::SetExtra, extraId, enabled ? 1 : 0, 0, 0);
            });
        }
    }

    void SickMenu::BuildVehicleSpawnerPages()
    {
        m_OnlineVehicleSpawnerPage.AddInfo("Status", [this]() {
            return m_State.vehicleSpawnerStatus;
        });
        m_OnlineVehicleSpawnerPage.AddToggle("Enter After Spawn", m_State.vehicleSpawnerEnterVehicle)
            .Describe("Places the local player in the driver seat after a successful spawn.");

        for (const auto category : OnlineVehicleSpawner::Categories)
        {
            auto page = std::make_unique<MenuPage>(
                std::string{"SPAWNER / "} + std::string{category});

            for (const auto model : OnlineVehicleSpawner::Vehicles)
            {
                if (OnlineVehicleSpawner::CategoryFor(model) != category)
                    continue;

                page->AddAction(std::string{model}, [this, model]() {
                    Notify(m_Callbacks.spawnVehicle, model, m_State.vehicleSpawnerEnterVehicle);
                }).EnabledWhen([this]() {
                    return !m_State.vehicleSpawnerBusy;
                }).Describe(std::string{"Spawns vehicle model: "} + std::string{model});
            }

            auto& categoryPage = m_VehicleSpawnerCategoryPages[std::string{category}];
            categoryPage = std::move(page);
            m_OnlineVehicleSpawnerPage.AddSubmenu(std::string{category}, *categoryPage);
        }
    }

    void SickMenu::BuildHandlingPages()
    {
        m_HandlingPage.AddInfo("Status", [this]() {
            if (!m_State.handlingAvailable)
                return std::string{"ADAPTER REQUIRED"};
            if (!m_State.handlingVehicleAttached)
                return std::string{"ENTER VEHICLE"};
            return std::string{"READY"};
        });
        m_HandlingPage.AddSubmenu("General", m_HandlingGeneralPage)
            .Describe("Mass, drag, submersion, centre of mass and inertia values.");
        m_HandlingPage.AddSubmenu("Transmission", m_HandlingTransmissionPage)
            .Describe("Drive bias, gears, drive force, clutch rates and top-speed handling values.");
        m_HandlingPage.AddSubmenu("Brakes", m_HandlingBrakesPage)
            .Describe("Service brake force, brake bias and hand-brake force.");
        m_HandlingPage.AddSubmenu("Steering", m_HandlingSteeringPage)
            .Describe("Steering-lock handling controls.");
        m_HandlingPage.AddSubmenu("Traction", m_HandlingTractionPage)
            .Describe("Tire grip curves, traction bias and traction-loss behavior.");
        m_HandlingPage.AddSubmenu("Suspension", m_HandlingSuspensionPage)
            .Describe("Spring force, damping, travel, ride height and suspension bias.");
        m_HandlingPage.AddSubmenu("Anti-Roll Bars", m_HandlingAntiRollPage)
            .Describe("Anti-roll force and front-to-rear anti-roll bias.");
        m_HandlingPage.AddSubmenu("Roll Centre", m_HandlingRollCentrePage)
            .Describe("Front and rear roll-centre height values.");
        m_HandlingPage.AddSubmenu("Other", m_HandlingOtherPage)
            .Describe("Collision, weapon, deformation and engine damage multipliers.");
        m_HandlingPage.AddAction("Restore Original Handling", [this]() {
            Notify(m_Callbacks.restoreOriginalHandling);
        }).EnabledWhen([this]() {
            return m_State.handlingAvailable && m_State.handlingVehicleAttached;
        }).Describe("Restores the original handling values captured when the current vehicle was attached.");
        m_HandlingPage.AddAction("Save Handling Profile", [this]() {
            Notify(m_Callbacks.saveHandlingProfile);
        }).EnabledWhen([this]() {
            return m_State.handlingAvailable && m_State.handlingVehicleAttached;
        }).Describe("Saves the current handling values to SickMenu/configs/handling on a background worker.");
        m_HandlingPage.AddSubmenu("Saved Profiles", m_HandlingProfilesPage)
            .Describe("Loads previously saved handling profiles without blocking the game thread.");

        const auto pageFor = [this](Handling::Group group) -> MenuPage* {
            switch (group)
            {
            case Handling::Group::General: return &m_HandlingGeneralPage;
            case Handling::Group::Transmission: return &m_HandlingTransmissionPage;
            case Handling::Group::Brakes: return &m_HandlingBrakesPage;
            case Handling::Group::Steering: return &m_HandlingSteeringPage;
            case Handling::Group::Traction: return &m_HandlingTractionPage;
            case Handling::Group::Suspension: return &m_HandlingSuspensionPage;
            case Handling::Group::AntiRoll: return &m_HandlingAntiRollPage;
            case Handling::Group::RollCentre: return &m_HandlingRollCentrePage;
            case Handling::Group::Other: return &m_HandlingOtherPage;
            }
            return nullptr;
        };

        for (const auto& spec : Handling::FieldSpecs)
        {
            auto* page = pageFor(spec.group);
            if (!page)
                continue;
            const auto index = Handling::ToIndex(spec.field);
            page->AddFloat(
                spec.label,
                m_State.handlingValues[index],
                spec.minimum,
                spec.maximum,
                spec.step,
                static_cast<int>(spec.precision),
                [this, field = spec.field](float value) {
                    Notify(m_Callbacks.handlingValue, field, value);
                })
                .EnabledWhen([this]() {
                    return m_State.handlingAvailable && m_State.handlingVehicleAttached;
                })
                .Describe(spec.description);
        }

        RebuildHandlingProfiles();
    }

    void SickMenu::RebuildHandlingProfiles()
    {
        m_HandlingProfilesPage.Options().clear();
        if (m_HandlingProfiles.empty())
            m_HandlingProfilesPage.AddLabel("No handling profiles found");
        else
        {
            for (const auto& profile : m_HandlingProfiles)
            {
                m_HandlingProfilesPage.AddAction(profile, [this, profile]() {
                    Notify(m_Callbacks.loadHandlingProfile, profile);
                }).EnabledWhen([this]() {
                    return m_State.handlingAvailable && m_State.handlingVehicleAttached;
                }).Describe("Loads this handling profile and applies changed fields in bounded game-thread batches.");
            }
        }
        m_HandlingProfilesPage.AddAction("Reload Profiles", [this]() {
            Notify(m_Callbacks.refreshHandlingProfiles);
        }).Describe("Rescans SickMenu/configs/handling on a background worker.");
    }

    void SickMenu::BuildSettingsPages()
    {
        MenuSettings::AddThemes(m_MenuSettingsPage, m_ThemesPage);
        MenuSettings::AddImageLoader(m_MenuSettingsPage, m_ImageLoaderPage);
        MenuSettings::AddFonts(m_MenuSettingsPage, m_FontsPage);
        MenuSettings::AddLuaScripts(m_MenuSettingsPage, m_ScriptsPage);
        MenuSettings::AddMoveMenu(m_MenuSettingsPage, m_State.moveMode);
        MenuSettings::AddMenuSize(m_MenuSettingsPage, m_State.menuScalePercent, [this](int) {
            ApplyLayout();
            NotifyPreferences();
        });
        MenuSettings::AddControls(m_MenuSettingsPage, m_ControlsPage);
        MenuSettings::AddSaveConfiguration(m_MenuSettingsPage, [this]() {
            NotifyPreferences();
            Notify(m_Callbacks.saveConfiguration);
        });
        MenuSettings::AddExitMenu(m_MenuSettingsPage, m_State.moveMode, m_Controller);
        MenuSettings::AddExitGta(m_MenuSettingsPage, m_ExitGtaPage, m_Controller, [this]() {
            Notify(m_Callbacks.exitGta);
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

    void SickMenu::Open() { m_Controller.Open(); }
    void SickMenu::Close() noexcept { m_State.moveMode = false; m_Controller.Close(); }
    void SickMenu::SetHeaderTexture(MenuTexture texture) noexcept { m_HeaderTexture = texture; }
    MenuTexture SickMenu::HeaderTexture() const noexcept { return m_HeaderTexture; }
    MenuDrawList SickMenu::Draw(MenuViewport viewport) { return m_Renderer.Render(m_Controller, viewport, m_HeaderTexture); }

    void SickMenu::SetPreferences(SickMenuPreferences preferences) noexcept
    {
        m_State.menuScalePercent = std::clamp(static_cast<int>(std::lround(preferences.scale * 100.0F)), 50, 250);
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

    void SickMenu::SetHandlingProfiles(std::uint64_t generation, std::vector<std::string> profiles)
    {
        if (generation == m_HandlingProfileGeneration)
            return;
        m_HandlingProfileGeneration = generation;
        m_HandlingProfiles = std::move(profiles);
        RebuildHandlingProfiles();

        if (m_Controller.IsOpen() && m_Controller.CurrentPage() == &m_HandlingProfilesPage)
        {
            for (std::size_t index = 0; index < m_HandlingProfilesPage.Options().size(); ++index)
            {
                if (m_Controller.SelectOption(index))
                    break;
            }
        }
    }

    bool SickMenu::IsHandlingPageActive() const noexcept
    {
        if (!m_Controller.IsOpen())
            return false;
        const auto* page = m_Controller.CurrentPage();
        return page == &m_HandlingPage ||
            page == &m_HandlingGeneralPage ||
            page == &m_HandlingTransmissionPage ||
            page == &m_HandlingBrakesPage ||
            page == &m_HandlingSteeringPage ||
            page == &m_HandlingTractionPage ||
            page == &m_HandlingSuspensionPage ||
            page == &m_HandlingAntiRollPage ||
            page == &m_HandlingRollCentrePage ||
            page == &m_HandlingOtherPage ||
            page == &m_HandlingProfilesPage;
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
        try { m_Callbacks.preferencesChanged(Preferences()); } catch (...) {}
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
