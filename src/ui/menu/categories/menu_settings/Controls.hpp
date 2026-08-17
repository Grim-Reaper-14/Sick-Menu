#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddControls(MenuPage& settings, MenuPage& controls)
    {
        settings.AddSubmenu("Controls", controls);
        controls.AddLabel("F4 - Open / Close");
        controls.AddLabel("Numpad 8 - Up");
        controls.AddLabel("Numpad 2 - Down");
        controls.AddLabel("Numpad 4 - Left");
        controls.AddLabel("Numpad 6 - Right");
        controls.AddLabel("Numpad 5 - Select");
        controls.AddLabel("Backspace - Back");
    }
}
