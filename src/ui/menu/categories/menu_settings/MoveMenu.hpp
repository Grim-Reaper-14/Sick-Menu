#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddMoveMenu(MenuPage& settings, bool& moveMode)
    {
        settings.AddToggle("Move Menu", moveMode);
    }
}
