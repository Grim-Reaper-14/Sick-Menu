#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddThemes(MenuPage& settings, MenuPage& themes)
    {
        settings.AddSubmenu("Themes", themes);
    }
}
