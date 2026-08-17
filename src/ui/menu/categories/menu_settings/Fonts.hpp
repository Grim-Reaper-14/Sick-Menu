#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddFonts(MenuPage& settings, MenuPage& fonts)
    {
        settings.AddSubmenu("Fonts", fonts);
    }
}
