#pragma once

#include "ui/menu/SickMenu.hpp"

#include <cstdint>

namespace Sick::Frontend
{
    class FrontendCore final
    {
    public:
        FrontendCore();

        // Pulls desired backend state and background-owned asset metadata into
        // frontend view state without invoking GTA natives or filesystem I/O.
        void Tick() noexcept;

        [[nodiscard]] Ui::SickMenu& Menu() noexcept
        {
            Tick();
            return m_Menu;
        }
        [[nodiscard]] const Ui::SickMenu& Menu() const noexcept { return m_Menu; }

    private:
        Ui::SickMenu m_Menu;
        std::uint64_t m_AssetGeneration{};
        bool m_PreferencesLoaded{};
    };
}
