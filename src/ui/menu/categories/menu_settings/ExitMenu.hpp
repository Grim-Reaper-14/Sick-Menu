#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddExitMenu(MenuPage& settings, bool& moveMode, MenuController& controller)
    {
        settings.AddAction("Exit Menu", [&moveMode, &controller]() {
            moveMode = false;
            controller.Close();
        });
    }
}
