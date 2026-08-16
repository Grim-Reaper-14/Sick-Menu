#pragma once

#include "ui/menu/SickMenu.hpp"

namespace Sick::Frontend
{
    class FrontendCore final
    {
    public:
        FrontendCore();

        [[nodiscard]] Ui::SickMenu& Menu() noexcept { return m_Menu; }
        [[nodiscard]] const Ui::SickMenu& Menu() const noexcept { return m_Menu; }

    private:
        Ui::SickMenu m_Menu;
    };
}
