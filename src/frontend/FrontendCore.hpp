#pragma once

#include "ui/menu/SickMenu.hpp"

namespace Sick::Frontend
{
    class FrontendCore final
    {
    public:
        FrontendCore();

        // Pulls desired backend state into frontend-owned view state without
        // invoking menu callbacks.
        void Tick() noexcept;

        [[nodiscard]] Ui::SickMenu& Menu() noexcept
        {
            Tick();
            return m_Menu;
        }
        [[nodiscard]] const Ui::SickMenu& Menu() const noexcept { return m_Menu; }

    private:
        Ui::SickMenu m_Menu;
    };
}
