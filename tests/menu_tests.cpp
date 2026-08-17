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

    bool HasText(
        const Reaper::UI::MenuDrawList& drawList,
        std::string_view text,
        Sick::Ui::MenuTextAlign alignment)
    {
        return std::ranges::any_of(drawList.Commands(), [text, alignment](const auto& command) {
            return command.kind == Sick::Ui::MenuDrawCommandKind::Text &&
                command.text == text && command.textAlign == alignment;
        });
    }

    std::size_t CountKind(
        const Reaper::UI::MenuDrawList& drawList,
        Sick::Ui::MenuDrawCommandKind kind)
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            drawList.Commands(),
            [kind](const auto& command) { return command.kind == kind; }));
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
        CHECK(controller.SelectionCounter().current == 1);
        CHECK(controller.SelectionCounter().total == 5);

        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.SelectedOptionIndex() == 2);
        CHECK(controller.SelectionCounter().current == 2);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(secondToggle);

        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Right));
        CHECK(number == 4);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Right));
        CHECK(number == 4);

        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.SelectedOption()->ValueText() == "Two [ 2 / 3 ]");
        CHECK(controller.Handle(Reaper::UI::MenuInput::Left));
        CHECK(choice == 0);
        CHECK(controller.SelectedOption()->ValueText() == "One [ 1 / 3 ]");

        CHECK(controller.Handle(Reaper::UI::MenuInput::Down));
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(controller.Depth() == 2);
        CHECK(controller.CurrentPage()->Title() == "CHILD");
        CHECK(controller.Handle(Reaper::UI::MenuInput::Select));
        CHECK(actions == 1);
        CHECK(controller.Handle(Reaper::UI::MenuInput::Back));
        CHECK(controller.Depth() == 1);
        CHECK(controller.CurrentPage()->Title() == "ROOT");
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
        CHECK(menu.RootPage().Title() == "SICK MENU");
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
        CHECK(rootOptions[1].LabelText() == "Vehicle");
        CHECK(rootOptions[2].LabelText() == "Weapons");
        CHECK(rootOptions[3].LabelText() == "World");
        CHECK(rootOptions[4].LabelText() == "Teleport");
        CHECK(rootOptions[5].LabelText() == "Tunables");
        CHECK(rootOptions[6].LabelText() == "Unlocks");
        CHECK(rootOptions[7].LabelText() == "Online Services");
        CHECK(rootOptions[8].LabelText() == "Online Vehicle Spawner");
        CHECK(rootOptions[9].LabelText() == "Online Protection");
        CHECK(rootOptions[10].LabelText() == "Menu Settings");

        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());
        menu.Open();
        CHECK(menu.Controller().SelectedOptionIndex() == 0);
        CHECK(menu.Controller().SelectionCounter().current == 1);
        CHECK(menu.Controller().SelectionCounter().total == 11);

        const auto rootDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(!rootDraw.Empty());
        CHECK(HasText(rootDraw, "SICK MENU", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(rootDraw, "1 / 11", Sick::Ui::MenuTextAlign::Right));

        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().Depth() == 2);
        CHECK(menu.Controller().CurrentPage()->Title() == "PLAYER");
        CHECK(menu.Controller().SelectionCounter().total == 3);

        const auto playerDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(playerDraw, "PLAYER", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(playerDraw, "1 / 3", Sick::Ui::MenuTextAlign::Right));
        CHECK(HasText(playerDraw, "GodMode", Sick::Ui::MenuTextAlign::Left));

        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().godMode);
        CHECK(godModeCallbacks == 1 && lastGodMode);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().Depth() == 1);
        CHECK(menu.Controller().CurrentPage()->Title() == "SICK MENU");

        CHECK(menu.Controller().SelectOption(1));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "VEHICLE");
        CHECK(menu.Controller().SelectionCounter().total == 0);
        const auto vehicleDraw = menu.Draw({1920.0F, 1080.0F});
        CHECK(HasText(vehicleDraw, "VEHICLE", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(vehicleDraw, "No options yet", Sick::Ui::MenuTextAlign::Center));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));

        menu.SetHeaderTexture(0x1234U);
        const auto textured = menu.Draw({1920.0F, 1080.0F});
        CHECK(CountKind(textured, Sick::Ui::MenuDrawCommandKind::Image) == 1);

        menu.Close();
        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());
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
            TestFrontendSynchronization();
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
