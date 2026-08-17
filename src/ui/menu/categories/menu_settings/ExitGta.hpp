#pragma once

#include "ui/menu/Menu.hpp"

#include <utility>

namespace Sick::Ui::MenuSettings
{
    template <typename Callback>
    void AddExitGta(
        MenuPage& settings,
        MenuPage& confirmation,
        MenuController& controller,
        Callback&& callback)
    {
        settings.AddSubmenu("Exit GTA", confirmation);
        confirmation.AddLabel("Are you sure?");
        confirmation.AddAction("Cancel", [&controller]() {
            static_cast<void>(controller.Handle(MenuInput::Back));
        });
        confirmation.AddAction(
            "Exit GTA",
            [&controller, callback = std::forward<Callback>(callback)]() mutable {
                callback();
                controller.Close();
            });
    }
}
