#include "Reaper.hpp"
#include "backend/BackendApi.hpp"
#include "frontend/FrontendCore.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
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
        bool lastGodMode{};
        Reaper::UI::SickMenuCallbacks callbacks{};
        callbacks.godMode = [&godModeCallbacks, &lastGodMode](bool value) {
            ++godModeCallbacks;
            lastGodMode = value;
        };
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
        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());
        menu.Open();
        const auto rootDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(rootDraw, "SICK MENU", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(rootDraw, "1 / 11", Sick::Ui::MenuTextAlign::Right));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "PLAYER");
        CHECK(menu.Controller().SelectionCounter().total == 3);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().godMode);
        CHECK(godModeCallbacks == 1 && lastGodMode);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().SelectOption(1));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().SelectionCounter().total == 0);
        const auto vehicleDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(vehicleDraw, "No options yet", Sick::Ui::MenuTextAlign::Center));
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

        menu.SetPreferences({
            .scale = 1.5F,
            .left = 100.0F,
            .top = 120.0F,
            .theme = "Neon",
            .banner = "sick-banner",
            .font = "Segoe UI",
        });
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
        Sick::Frontend::FrontendCore frontend;
        frontend.Tick();
        CHECK(frontend.Menu().State().godMode);
        backend.SetGodMode(false);
        frontend.Tick();
        CHECK(!frontend.Menu().State().godMode);
        return true;
    }

    bool RunTests()
    {
        return TestNavigationAndSubmenus() &&
            TestSickMenuStructureAndRenderer() &&
            TestSettingsAssetsAndMoveMode() &&
            TestFrontendSynchronization();
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
