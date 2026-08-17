#pragma once

#include "ui/menu/Menu.hpp"

#include <utility>

namespace Sick::Ui::MenuSettings
{
    template <typename Callback>
    void AddSaveConfiguration(MenuPage& settings, Callback&& callback)
    {
        settings.AddAction("Save Configuration", std::forward<Callback>(callback));
    }
}
