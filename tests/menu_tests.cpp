#include "Reaper.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string_view>

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

    bool TestReferenceMenuAndRenderer()
    {
        std::size_t regularActions{};
        std::size_t toggleCallbacks{};
        std::size_t numberCallbacks{};
        std::size_t vectorCallbacks{};
        bool lastToggle{};
        int lastNumber{};
        std::size_t lastVector{};

        Reaper::UI::SickMenuCallbacks callbacks{};
        callbacks.regularAction = [&regularActions]() { ++regularActions; };
        callbacks.demoToggle = [&toggleCallbacks, &lastToggle](bool value) {
            ++toggleCallbacks;
            lastToggle = value;
        };
        callbacks.demoNumber = [&numberCallbacks, &lastNumber](int value) {
            ++numberCallbacks;
            lastNumber = value;
        };
        callbacks.demoVector = [&vectorCallbacks, &lastVector](std::size_t value) {
            ++vectorCallbacks;
            lastVector = value;
        };

        Reaper::UI::SickMenu menu{std::move(callbacks)};
        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());

        menu.Open();
        CHECK(menu.Controller().SelectedOptionIndex() == 4);
        const auto counter = menu.Controller().SelectionCounter();
        CHECK(counter.current == 4);
        CHECK(counter.total == 7);

        const auto drawList = menu.Draw({1920.0F, 1080.0F});
        CHECK(!drawList.Empty());
        CHECK(HasText(drawList, "SELF", Sick::Ui::MenuTextAlign::Left));
        CHECK(HasText(drawList, "4 / 7", Sick::Ui::MenuTextAlign::Right));
        CHECK(HasText(drawList, "Demo", Sick::Ui::MenuTextAlign::Center));
        CHECK(HasText(drawList, "Three [ 3 / 3 ]", Sick::Ui::MenuTextAlign::Right));
        CHECK(CountKind(drawList, Sick::Ui::MenuDrawCommandKind::FilledCircle) == 4);
        CHECK(CountKind(drawList, Sick::Ui::MenuDrawCommandKind::Line) == 8);
        CHECK(std::ranges::any_of(drawList.Commands(), [&menu](const auto& command) {
            return command.kind == Sick::Ui::MenuDrawCommandKind::FilledRect &&
                command.color == menu.Renderer().Style().selectedColor;
        }));

        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(regularActions == 1);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Down));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.State().demoToggle);
        CHECK(toggleCallbacks == 1 && lastToggle);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Down));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Right));
        CHECK(menu.State().demoNumber == 2);
        CHECK(numberCallbacks == 1 && lastNumber == 2);

        CHECK(menu.Handle(Reaper::UI::MenuInput::Down));
        CHECK(menu.Controller().SelectionCounter().current == 7);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Right));
        CHECK(menu.State().demoVector == 0);
        CHECK(vectorCallbacks == 1 && lastVector == 0);

        menu.SetHeaderTexture(0x1234U);
        const auto textured = menu.Draw({1920.0F, 1080.0F});
        CHECK(CountKind(textured, Sick::Ui::MenuDrawCommandKind::Image) == 1);
        CHECK(CountKind(textured, Sick::Ui::MenuDrawCommandKind::Line) == 4);

        menu.Close();
        CHECK(menu.Draw({1920.0F, 1080.0F}).Empty());
        return true;
    }

    bool RunTests()
    {
        return TestNavigationAndSubmenus() && TestReferenceMenuAndRenderer();
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
