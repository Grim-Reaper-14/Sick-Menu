#include "Reaper.hpp"
#include "backend/BackendApi.hpp"
#include "frontend/FrontendCore.hpp"
#include "ui/menu/categories/online_vehicle_spawner/VehicleCatalog.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    bool Check(bool condition, const char* expression, int line)
    {
        if (condition)
            return true;
        std::cerr << "check failed at line " << line << ": " << expression << '\n';
        return false;
    }

#define CHECK(expression) \
    do \
    { \
        if (!Check(static_cast<bool>(expression), #expression, __LINE__)) \
            return false; \
    } while (false)

    bool HasText(const Reaper::UI::MenuDrawList& drawList, std::string_view text, Sick::Ui::MenuTextAlign alignment)
    {
        return std::ranges::any_of(drawList.Commands(), [text, alignment](const auto& command) {
            return command.kind == Sick::Ui::MenuDrawCommandKind::Text && command.text == text && command.textAlign == alignment;
        });
    }

    std::size_t CountKind(const Reaper::UI::MenuDrawList& drawList, Sick::Ui::MenuDrawCommandKind kind)
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            drawList.Commands(), [kind](const auto& command) { return command.kind == kind; }));
    }

    bool TestNavigationAndSubmenus()
    {
        bool firstToggle{};
        bool secondToggle{};
        int number{3};
        std::size_t choice{1};
        int actions{};
        Reaper::UI::MenuPage child{"CHILD"};
        child.AddAction("Run", [&actions]() { ++actions; });
        Reaper::UI::MenuPage root{"ROOT"};
        root.AddToggle("First", firstToggle);
        root.AddLabel("Section");
        root.AddToggle("Second", secondToggle);
        root.AddInteger("Number", number, 0, 4);
        root.AddChoice("Choice", choice, {"One", "Two", "Three"});
        root.AddSubmenu("Child", child);
        Reaper::UI::MenuController controller{root};
        controller.Open();
        CHECK(controller.SelectedOptionIndex() == 0);
        CHECK(controller.SelectionCounter().total == 5);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.SelectedOptionIndex() == 2);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(secondToggle);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Right));
        CHECK(number == 4);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Left));
        CHECK(choice == 0);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(controller.CurrentPage()->Title() == "CHILD");
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(actions == 1);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Back));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Back));
        CHECK(!controller.IsOpen());
        return true;
    }

    bool TestSickMenuStructureAndRenderer()
    {
        std::size_t godModeCallbacks{};
        std::size_t oxygenCallbacks{};
        std::size_t wantedCallbacks{};
        std::size_t vehicleGodCallbacks{};
        std::size_t repairCallbacks{};
        std::size_t spawnVehicleCallbacks{};
        std::size_t sessionSwitchCallbacks{};
        std::size_t garageSaveCallbacks{};
        bool lastGodMode{};
        bool lastOxygen{};
        bool lastVehicleGod{};
        bool lastSpawnEnter{};
        int lastWanted{};
        std::int32_t lastSessionType{};
        std::string lastSpawnModel;
        Reaper::UI::SickMenuCallbacks callbacks{};
        callbacks.godMode = [&](bool value) { ++godModeCallbacks; lastGodMode = value; };
        callbacks.infiniteOxygen = [&](bool value) { ++oxygenCallbacks; lastOxygen = value; };
        callbacks.wantedLevel = [&](int value) { ++wantedCallbacks; lastWanted = value; };
        callbacks.vehicleGodMode = [&](bool value) { ++vehicleGodCallbacks; lastVehicleGod = value; };
        callbacks.repairVehicle = [&]() { ++repairCallbacks; };
        callbacks.spawnVehicle = [&](std::string_view modelName, bool enterVehicle) {
            ++spawnVehicleCallbacks;
            lastSpawnModel = modelName;
            lastSpawnEnter = enterVehicle;
        };
        callbacks.switchOnlineSession = [&](std::int32_t type) {
            ++sessionSwitchCallbacks;
            lastSessionType = type;
        };
        callbacks.saveCurrentVehicleToPersonalGarage = [&]() { ++garageSaveCallbacks; };

        Reaper::UI::SickMenu menu{std::move(callbacks)};
        CHECK(menu.RootPage().Options().size() == 11);
        CHECK(menu.PlayerPage().Title() == "PLAYER");
        CHECK(menu.VehiclePage().Title() == "VEHICLE");
        CHECK(menu.WeaponsPage().Title() == "WEAPONS");
        CHECK(menu.WorldPage().Title() == "WORLD");
        CHECK(menu.TeleportPage().Title() == "TELEPORT");
        CHECK(menu.TunablesPage().Title() == "TUNABLES");
        CHECK(menu.UnlocksPage().Title() == "UNLOCKS");
        CHECK(menu.OnlineServicesPage().Title() == "ONLINE SERVICES");
        CHECK(menu.OnlineVehicleSpawnerPage().Title() == "ONLINE VEHICLE SPAWNER");
        CHECK(menu.OnlineProtectionPage().Title() == "ONLINE PROTECTION");
        CHECK(menu.MenuSettingsPage().Title() == "MENU SETTINGS");
        CHECK(&menu.SelfPage() == &menu.PlayerPage());

        const auto& rootOptions = menu.RootPage().Options();
        CHECK(rootOptions[0].LabelText() == "Player");
        CHECK(rootOptions[10].LabelText() == "Menu Settings");

        const auto& playerOptions = menu.PlayerPage().Options();
        CHECK(playerOptions.size() == 15);
        CHECK(playerOptions[0].LabelText() == "GodMode");
        CHECK(playerOptions[1].LabelText() == "Infinite Oxygen");
        CHECK(playerOptions[6].LabelText() == "Set Wanted Level");
        CHECK(playerOptions[12].LabelText() == "Waterproof");
        CHECK(playerOptions[14].LabelText() == "Graceful Landing");

        const auto& vehicleOptions = menu.VehiclePage().Options();
        CHECK(vehicleOptions.size() == 11);
        CHECK(vehicleOptions[0].LabelText() == "Vehicle God Mode");
        CHECK(vehicleOptions[1].LabelText() == "Auto Repair");
        CHECK(vehicleOptions[2].LabelText() == "Keep Vehicle Clean");
        CHECK(vehicleOptions[3].LabelText() == "Engine Always On");
        CHECK(vehicleOptions[4].LabelText() == "No Gravity");
        CHECK(vehicleOptions[5].LabelText() == "No Collision");
        CHECK(vehicleOptions[6].LabelText() == "Handling");
        CHECK(vehicleOptions[7].LabelText() == "Customization");
        CHECK(vehicleOptions[8].LabelText() == "Repair Vehicle");
        CHECK(vehicleOptions[9].LabelText() == "Clean Vehicle");
        CHECK(vehicleOptions[10].LabelText() == "Put On Ground");

        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());
        menu.Open();
        const auto rootDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(rootDraw, "SICK MENU", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(rootDraw, "1 / 11", Sick::Ui::MenuTextAlign::Right));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "PLAYER");
        CHECK(menu.Controller().SelectionCounter().total == 15);

        const auto playerDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(playerDraw, "OFF", Sick::Ui::MenuTextAlign::Right));
        CHECK(HasText(playerDraw, "< 0 >", Sick::Ui::MenuTextAlign::Right));
        CHECK(HasText(playerDraw, "Makes the local player invincible while enabled.", Sick::Ui::MenuTextAlign::Left));
        CHECK(CountKind(playerDraw, Sick::Ui::MenuDrawCommandKind::FilledCircle) >= 3);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().godMode);
        CHECK(godModeCallbacks == 1 && lastGodMode);
        CHECK(HasText(menu.Draw({1920.0F, 1080.0F}), "ON", Sick::Ui::MenuTextAlign::Right));

        CHECK(menu.Controller().SelectOption(1));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().infiniteOxygen);
        CHECK(oxygenCallbacks == 1 && lastOxygen);

        CHECK(menu.Controller().SelectOption(6));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Right));
        CHECK(menu.State().wantedLevel == 1);
        CHECK(wantedCallbacks == 1 && lastWanted == 1);
        CHECK(HasText(menu.Draw({1920.0F, 1080.0F}), "< 1 >", Sick::Ui::MenuTextAlign::Right));

        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().SelectOption(1));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "VEHICLE");
        CHECK(menu.Controller().SelectionCounter().total == 11);
        const auto vehicleDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(vehicleDraw, "Keeps the vehicle you are using invincible while enabled.", Sick::Ui::MenuTextAlign::Left));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().vehicleGodMode);
        CHECK(vehicleGodCallbacks == 1 && lastVehicleGod);
        CHECK(menu.Controller().SelectOption(7));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "VEHICLE / CUSTOMIZATION");
        CHECK(menu.Controller().SelectionCounter().total == 6);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().SelectOption(8));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(repairCallbacks == 1);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().SelectOption(7));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "ONLINE SERVICES");
        CHECK(menu.Controller().SelectionCounter().total == 3);
        const auto& onlineOptions = menu.OnlineServicesPage().Options();
        CHECK(onlineOptions.size() == 5);
        CHECK(onlineOptions[0].LabelText() == "Session Status");
        CHECK(onlineOptions[1].LabelText() == "Session Type");
        CHECK(onlineOptions[2].LabelText() == "Switch Session");
        CHECK(onlineOptions[3].LabelText() == "Garage Status");
        CHECK(onlineOptions[4].LabelText() == "Save Current Vehicle To Garage");
        CHECK(menu.Controller().SelectOption(1));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Right));
        CHECK(menu.State().onlineSessionType == 1);
        CHECK(menu.Controller().SelectOption(2));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(sessionSwitchCallbacks == 1 && lastSessionType == 1);
        CHECK(menu.Controller().SelectOption(4));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(garageSaveCallbacks == 1);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));

        CHECK(menu.Controller().SelectOption(8));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "ONLINE VEHICLE SPAWNER");
        CHECK(menu.Controller().SelectionCounter().total == 9);
        const auto& spawnerOptions = menu.OnlineVehicleSpawnerPage().Options();
        CHECK(spawnerOptions.size() == 10);
        CHECK(spawnerOptions[0].LabelText() == "Status");
        CHECK(spawnerOptions[1].LabelText() == "Enter After Spawn");
        CHECK(spawnerOptions[2].LabelText() == "A-C");
        CHECK(spawnerOptions[9].LabelText() == "V-Z");
        CHECK(Sick::Ui::OnlineVehicleSpawner::Vehicles.size() == 935);
        CHECK(std::ranges::is_sorted(Sick::Ui::OnlineVehicleSpawner::Vehicles));
        CHECK(std::ranges::adjacent_find(Sick::Ui::OnlineVehicleSpawner::Vehicles) ==
            Sick::Ui::OnlineVehicleSpawner::Vehicles.end());
        CHECK(menu.Controller().SelectOption(2));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "SPAWNER / A-C");
        CHECK(menu.Controller().SelectionCounter().total == 196);
        CHECK(menu.Controller().CurrentPage()->Options()[0].LabelText() == "adder");
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(spawnVehicleCallbacks == 1);
        CHECK(lastSpawnModel == "adder");
        CHECK(lastSpawnEnter);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        menu.SetHeaderTexture(0x1234U);
        CHECK(CountKind(menu.Draw({1920.0F, 1080.0F}), Sick::Ui::MenuDrawCommandKind::Image) == 1);
        menu.Close();
        return true;
    }

    bool TestSettingsAssetsAndMoveMode()
    {
        std::size_t preferenceChanges{};
        std::size_t saves{};
        std::size_t refreshes{};
        std::size_t exits{};
        Reaper::UI::SickMenuCallbacks callbacks{};
        callbacks.preferencesChanged = [&preferenceChanges](const Sick::Ui::SickMenuPreferences&) { ++preferenceChanges; };
        callbacks.saveConfiguration = [&saves]() { ++saves; };
        callbacks.refreshAssets = [&refreshes]() { ++refreshes; };
        callbacks.exitGta = [&exits]() { ++exits; };
        Reaper::UI::SickMenu menu{std::move(callbacks)};
        CHECK(menu.MenuSettingsPage().Options().size() == 10);
        CHECK(menu.MenuSettingsPage().Options()[0].LabelText() == "Themes");
        CHECK(menu.MenuSettingsPage().Options()[1].LabelText() == "Image Loader");
        CHECK(menu.MenuSettingsPage().Options()[2].LabelText() == "Fonts");
        CHECK(menu.MenuSettingsPage().Options()[3].LabelText() == "Lua Scripts");
        CHECK(menu.MenuSettingsPage().Options()[7].LabelText() == "Save Configuration");
        CHECK(menu.MenuSettingsPage().Options()[8].LabelText() == "Exit Menu");
        CHECK(menu.MenuSettingsPage().Options()[9].LabelText() == "Exit GTA");

        Sick::Ui::SickMenuAssetCatalog catalog{};
        catalog.generation = 1;
        Sick::Ui::SickMenuTheme theme{};
        theme.asset = {"Neon", "C:/SickMenu/themes/Neon.json"};
        theme.accent = {10, 20, 30, 255};
        catalog.themes.push_back(theme);
        catalog.images.push_back({"sick-banner", "C:/SickMenu/images/sick-banner.png"});
        catalog.fonts.push_back({"Segoe UI", "C:/Windows/Fonts/segoeui.ttf"});
        catalog.scripts.push_back({"VehicleTools", "C:/SickMenu/scripts/VehicleTools.lua"});
        menu.SetAssetCatalog(std::move(catalog));
        CHECK(menu.ThemesPage().Options().size() == 3);
        CHECK(menu.ImageLoaderPage().Options().size() == 3);
        CHECK(menu.FontsPage().Options().size() == 3);
        CHECK(menu.ScriptsPage().Options().size() == 2);

        menu.SetPreferences({.scale = 1.5F, .left = 100.0F, .top = 120.0F, .theme = "Neon", .banner = "sick-banner", .font = "Segoe UI"});
        const Sick::Ui::MenuColor expectedAccent{10, 20, 30, 255};
        CHECK(menu.SelectedBannerPath() == "C:/SickMenu/images/sick-banner.png");
        CHECK(menu.SelectedFontPath() == "C:/Windows/Fonts/segoeui.ttf");
        CHECK(menu.Renderer().Style().accentColor == expectedAccent);
        CHECK(menu.Renderer().Style().uiScale == 1.5F);
        CHECK(menu.Renderer().Style().left == 100.0F);

        menu.Open();
        CHECK(menu.Controller().SelectOption(10));
        CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
        CHECK(menu.Controller().SelectOption(4));
        CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
        CHECK(menu.State().moveMode);
        const auto oldLeft = menu.State().menuLeft;
        CHECK(menu.Handle(Sick::Ui::MenuInput::Right));
        CHECK(menu.State().menuLeft == oldLeft + 8.0F);
        CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
        CHECK(!menu.State().moveMode);
        CHECK(preferenceChanges >= 2);
        CHECK(menu.Controller().SelectOption(7));
        CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
        CHECK(saves == 1);
        CHECK(refreshes == 0);
        CHECK(exits == 0);
        return true;
    }

    bool TestFrontendSynchronization()
    {
        auto& backend = Sick::Backend::BackendApi::Get();
        backend.SetGodMode(true);
        backend.SetInfiniteOxygen(true);
        backend.SetNoRagdoll(true);
        backend.SetSuperJump(true);
        backend.SetSeatBelt(true);
        backend.SetNoWantedLevel(true);
        backend.SetWantedLevel(4);
        backend.SetFastRun(true);
        backend.SetFastSwim(true);
        backend.SetKeepPlayerClean(true);
        backend.SetAqualung(true);
        backend.SetNoGravity(true);
        backend.SetWaterproof(true);
        backend.SetVehicleGodMode(true);
        backend.SetVehicleAutoRepair(true);
        backend.SetVehicleKeepClean(true);
        backend.SetVehicleEngineAlwaysOn(true);
        backend.SetVehicleNoGravity(true);
        backend.SetVehicleNoCollision(true);

        Sick::Frontend::FrontendCore frontend;
        frontend.Tick();
        CHECK(frontend.Menu().State().godMode);
        CHECK(frontend.Menu().State().infiniteOxygen);
        CHECK(frontend.Menu().State().noRagdoll);
        CHECK(frontend.Menu().State().superJump);
        CHECK(frontend.Menu().State().seatBelt);
        CHECK(frontend.Menu().State().noWantedLevel);
        CHECK(frontend.Menu().State().wantedLevel == 4);
        CHECK(frontend.Menu().State().fastRun);
        CHECK(frontend.Menu().State().fastSwim);
        CHECK(frontend.Menu().State().keepPlayerClean);
        CHECK(frontend.Menu().State().aqualung);
        CHECK(frontend.Menu().State().noGravity);
        CHECK(frontend.Menu().State().waterproof);
        CHECK(frontend.Menu().State().vehicleGodMode);
        CHECK(frontend.Menu().State().vehicleAutoRepair);
        CHECK(frontend.Menu().State().vehicleKeepClean);
        CHECK(frontend.Menu().State().vehicleEngineAlwaysOn);
        CHECK(frontend.Menu().State().vehicleNoGravity);
        CHECK(frontend.Menu().State().vehicleNoCollision);

        backend.SetGodMode(false);
        backend.SetInfiniteOxygen(false);
        backend.SetNoRagdoll(false);
        backend.SetSuperJump(false);
        backend.SetSeatBelt(false);
        backend.SetNoWantedLevel(false);
        backend.SetWantedLevel(0);
        backend.SetFastRun(false);
        backend.SetFastSwim(false);
        backend.SetKeepPlayerClean(false);
        backend.SetAqualung(false);
        backend.SetNoGravity(false);
        backend.SetWaterproof(false);
        backend.SetVehicleGodMode(false);
        backend.SetVehicleAutoRepair(false);
        backend.SetVehicleKeepClean(false);
        backend.SetVehicleEngineAlwaysOn(false);
        backend.SetVehicleNoGravity(false);
        backend.SetVehicleNoCollision(false);
        frontend.Tick();
        CHECK(!frontend.Menu().State().godMode);
        CHECK(!frontend.Menu().State().infiniteOxygen);
        CHECK(!frontend.Menu().State().vehicleGodMode);
        CHECK(!frontend.Menu().State().vehicleAutoRepair);
        CHECK(!frontend.Menu().State().vehicleKeepClean);
        CHECK(!frontend.Menu().State().vehicleEngineAlwaysOn);
        CHECK(!frontend.Menu().State().vehicleNoGravity);
        CHECK(!frontend.Menu().State().vehicleNoCollision);
        return true;
    }

    bool RunTests()
    {
        return TestNavigationAndSubmenus() && TestSickMenuStructureAndRenderer() && TestSettingsAssetsAndMoveMode() && TestFrontendSynchronization();
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
