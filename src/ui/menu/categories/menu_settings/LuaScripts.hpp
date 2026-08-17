#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddLuaScripts(MenuPage& settings, MenuPage& scripts)
    {
        settings.AddSubmenu("Lua Scripts", scripts);
    }
}
