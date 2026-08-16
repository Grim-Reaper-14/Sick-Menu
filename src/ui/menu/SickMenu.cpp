#include "SickMenu.hpp"

#include "game/scheduler/GameScheduler.hpp"
#include "game/services/PlayerService.hpp"

#include <utility>
#include <vector>

namespace
{
    template <typename Callback, typename... Arguments>
    void QueueCallback(Callback callback, Arguments... arguments)
    {
        if (!callback)
            return;

        Sick::Game::GameScheduler::Get().Queue(
            [callback = std::move(callback), ... arguments = std::move(arguments)]() mutable {
                callback(std::move(arguments)...);
            });
    }
}

namespace Sick::Ui
{
    SickMenu::SickMenu(SickMenuCallbacks callbacks)
        : m_Controller(m_SelfPage)
    {
        auto godModeCallback = std::move(callbacks.godMode);
        m_SelfPage.AddToggle(
            "GodMode",
            m_State.godMode,
            [callback = std::move(godModeCallback)](bool enabled) mutable {
                Game::PlayerService{}.SetInvincible(enabled);
                QueueCallback(callback, enabled);
            });

        m_SelfPage.AddToggle(
            "Beast Jump",
            m_State.beastJump,
            [callback = std::move(callbacks.beastJump)](bool enabled) mutable {
                QueueCallback(callback, enabled);
            });
        m_SelfPage.AddToggle(
            "Graceful Landing",
            m_State.gracefulLanding,
            [callback = std::move(callbacks.gracefulLanding)](bool enabled) mutable {
                QueueCallback(callback, enabled);
            });

        m_SelfPage.AddLabel("Demo");
        m_SelfPage.AddAction(
            "Regular Option",
            [callback = std::move(callbacks.regularAction)]() mutable {
                QueueCallback(callback);
            });
        m_SelfPage.AddToggle(
            "Toggle Option",
            m_State.demoToggle,
            [callback = std::move(callbacks.demoToggle)](bool enabled) mutable {
                QueueCallback(callback, enabled);
            });
        m_SelfPage.AddInteger(
            "Number Option",
            m_State.demoNumber,
            0,
            10,
            1,
            [callback = std::move(callbacks.demoNumber)](int value) mutable {
                QueueCallback(callback, value);
            });
        m_SelfPage.AddChoice(
            "Vector Option",
            m_State.demoVector,
            std::vector<std::string>{"One", "Two", "Three"},
            [callback = std::move(callbacks.demoVector)](std::size_t index) mutable {
                QueueCallback(callback, index);
            });

        // Match the supplied reference frame: Regular Option is item 4 / 7.
        m_Controller.SelectOption(4);
    }

    bool SickMenu::Handle(MenuInput input)
    {
        return m_Controller.Handle(input);
    }

    void SickMenu::Open()
    {
        m_Controller.Open();
    }

    void SickMenu::Close() noexcept
    {
        m_Controller.Close();
    }

    void SickMenu::SetHeaderTexture(MenuTexture texture) noexcept
    {
        m_HeaderTexture = texture;
    }

    MenuTexture SickMenu::HeaderTexture() const noexcept
    {
        return m_HeaderTexture;
    }

    MenuDrawList SickMenu::Draw(MenuViewport viewport)
    {
        return m_Renderer.Render(m_Controller, viewport, m_HeaderTexture);
    }

    SickMenuState& SickMenu::State() noexcept
    {
        return m_State;
    }

    const SickMenuState& SickMenu::State() const noexcept
    {
        return m_State;
    }

    MenuController& SickMenu::Controller() noexcept
    {
        return m_Controller;
    }

    const MenuController& SickMenu::Controller() const noexcept
    {
        return m_Controller;
    }

    MenuRenderer& SickMenu::Renderer() noexcept
    {
        return m_Renderer;
    }

    const MenuRenderer& SickMenu::Renderer() const noexcept
    {
        return m_Renderer;
    }

    MenuPage& SickMenu::SelfPage() noexcept
    {
        return m_SelfPage;
    }

    const MenuPage& SickMenu::SelfPage() const noexcept
    {
        return m_SelfPage;
    }
}
