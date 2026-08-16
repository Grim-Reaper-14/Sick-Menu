#pragma once

#include "MenuRenderer.hpp"

#include <cstddef>
#include <functional>

namespace Sick::Ui
{
    struct SickMenuState
    {
        bool godMode{};
        bool beastJump{};
        bool gracefulLanding{};
        bool demoToggle{};
        int demoNumber{1};
        std::size_t demoVector{2};
    };

    // Frontend notifications execute on the frontend/render thread. They must
    // only submit intent to BackendApi; GTA natives and script functions are
    // executed by BackendCore on the game thread.
    struct SickMenuCallbacks
    {
        std::function<void(bool)> godMode;
        std::function<void(bool)> beastJump;
        std::function<void(bool)> gracefulLanding;
        std::function<void()> regularAction;
        std::function<void(bool)> demoToggle;
        std::function<void(int)> demoNumber;
        std::function<void(std::size_t)> demoVector;
    };

    class SickMenu final
    {
    public:
        explicit SickMenu(SickMenuCallbacks callbacks = {});

        SickMenu(const SickMenu&) = delete;
        SickMenu& operator=(const SickMenu&) = delete;
        SickMenu(SickMenu&&) = delete;
        SickMenu& operator=(SickMenu&&) = delete;

        bool Handle(MenuInput input);
        void Open();
        void Close() noexcept;

        void SetHeaderTexture(MenuTexture texture) noexcept;
        [[nodiscard]] MenuTexture HeaderTexture() const noexcept;
        [[nodiscard]] MenuDrawList Draw(MenuViewport viewport);

        [[nodiscard]] SickMenuState& State() noexcept;
        [[nodiscard]] const SickMenuState& State() const noexcept;
        [[nodiscard]] MenuController& Controller() noexcept;
        [[nodiscard]] const MenuController& Controller() const noexcept;
        [[nodiscard]] MenuRenderer& Renderer() noexcept;
        [[nodiscard]] const MenuRenderer& Renderer() const noexcept;
        [[nodiscard]] MenuPage& SelfPage() noexcept;
        [[nodiscard]] const MenuPage& SelfPage() const noexcept;

    private:
        SickMenuState m_State;
        MenuPage m_SelfPage{"SELF"};
        MenuController m_Controller;
        MenuRenderer m_Renderer;
        MenuTexture m_HeaderTexture{};
    };
}
