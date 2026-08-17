#include "ui/menu/SickMenu.hpp"

#include <algorithm>
#include <cmath>
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
            return 1; \
    } while (false)

    bool NearlyEqual(float left, float right) noexcept
    {
        return std::fabs(left - right) < 0.0001F;
    }

    bool HasText(const Sick::Ui::MenuDrawList& drawList, std::string_view text)
    {
        return std::ranges::any_of(drawList.Commands(), [text](const auto& command) {
            return command.kind == Sick::Ui::MenuDrawCommandKind::Text && command.text == text;
        });
    }
}

int main()
{
    Sick::Handling::Field lastField = Sick::Handling::Field::Count;
    float lastValue{};
    std::size_t valueCallbacks{};
    std::size_t restoreCallbacks{};
    std::size_t saveCallbacks{};
    std::size_t refreshCallbacks{};
    std::string loadedProfile;

    Sick::Ui::SickMenuCallbacks callbacks{};
    callbacks.handlingValue = [&](Sick::Handling::Field field, float value) {
        lastField = field;
        lastValue = value;
        ++valueCallbacks;
    };
    callbacks.restoreOriginalHandling = [&]() { ++restoreCallbacks; };
    callbacks.saveHandlingProfile = [&]() { ++saveCallbacks; };
    callbacks.loadHandlingProfile = [&](const std::string& name) { loadedProfile = name; };
    callbacks.refreshHandlingProfiles = [&]() { ++refreshCallbacks; };

    Sick::Ui::SickMenu menu{std::move(callbacks)};
    const auto& vehicleOptions = menu.VehiclePage().Options();
    CHECK(vehicleOptions.size() == 10);
    CHECK(vehicleOptions[6].LabelText() == "Handling");
    CHECK(vehicleOptions[6].Kind() == Sick::Ui::MenuOptionKind::Submenu);
    CHECK(vehicleOptions[7].LabelText() == "Repair Vehicle");

    const auto& handlingOptions = menu.HandlingPage().Options();
    CHECK(handlingOptions.size() == 13);
    CHECK(handlingOptions[0].Kind() == Sick::Ui::MenuOptionKind::Info);
    CHECK(handlingOptions[0].ValueText() == "ADAPTER REQUIRED");
    CHECK(handlingOptions[1].LabelText() == "General");
    CHECK(handlingOptions[9].LabelText() == "Other");
    CHECK(handlingOptions[10].LabelText() == "Restore Original Handling");
    CHECK(handlingOptions[11].LabelText() == "Save Handling Profile");
    CHECK(handlingOptions[12].LabelText() == "Saved Profiles");

    auto& general = menu.HandlingGeneralPage();
    CHECK(general.Options().size() == 9);
    CHECK(general.Options()[0].Kind() == Sick::Ui::MenuOptionKind::Float);
    CHECK(general.Options()[0].LabelText() == "Mass");
    CHECK(!general.Options()[0].Enabled());

    menu.State().handlingAvailable = true;
    menu.State().handlingVehicleAttached = true;
    CHECK(general.Options()[0].Enabled());
    CHECK(handlingOptions[0].ValueText() == "READY");

    const float beforeMass = menu.State().handlingValues[Sick::Handling::ToIndex(Sick::Handling::Field::Mass)];
    general.Options()[0].Adjust(1);
    CHECK(valueCallbacks == 1);
    CHECK(lastField == Sick::Handling::Field::Mass);
    CHECK(lastValue > beforeMass);
    CHECK(NearlyEqual(
        lastValue,
        menu.State().handlingValues[Sick::Handling::ToIndex(Sick::Handling::Field::Mass)]));

    menu.SetHandlingProfiles(1, {"race", "drift"});
    CHECK(menu.HandlingProfilesPage().Options().size() == 3);
    CHECK(menu.HandlingProfilesPage().Options()[0].LabelText() == "race");
    menu.HandlingProfilesPage().Options()[0].Activate();
    CHECK(loadedProfile == "race");
    menu.HandlingProfilesPage().Options()[2].Activate();
    CHECK(refreshCallbacks == 1);

    handlingOptions[10].Activate();
    handlingOptions[11].Activate();
    CHECK(restoreCallbacks == 1);
    CHECK(saveCallbacks == 1);

    menu.Open();
    CHECK(menu.Controller().SelectOption(1));
    CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
    CHECK(menu.Controller().CurrentPage() == &menu.VehiclePage());
    CHECK(menu.Controller().SelectOption(6));
    CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
    CHECK(menu.Controller().CurrentPage() == &menu.HandlingPage());
    CHECK(menu.IsHandlingPageActive());
    CHECK(menu.Controller().SelectionCounter().total == 12);
    CHECK(menu.Handle(Sick::Ui::MenuInput::Select));
    CHECK(menu.Controller().CurrentPage() == &menu.HandlingGeneralPage());
    CHECK(menu.IsHandlingPageActive());
    CHECK(HasText(menu.Draw({1920.0F, 1080.0F}), "< 51 >"));

    return 0;
}
