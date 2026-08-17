#pragma once

#include "ui/menu/Menu.hpp"

#include <utility>

namespace Sick::Ui::MenuSettings
{
    template <typename Callback>
    void AddMenuSize(MenuPage& settings, int& percent, Callback&& callback)
    {
        settings.AddInteger(
            "Menu Size",
            percent,
            50,
            250,
            5,
            std::forward<Callback>(callback));
    }
}
