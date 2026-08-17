#pragma once

#include "ui/menu/Menu.hpp"

namespace Sick::Ui::MenuSettings
{
    inline void AddImageLoader(MenuPage& settings, MenuPage& images)
    {
        settings.AddSubmenu("Image Loader", images);
    }
}
