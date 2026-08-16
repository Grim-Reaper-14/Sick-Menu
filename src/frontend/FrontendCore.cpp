#include "FrontendCore.hpp"

#include "backend/BackendApi.hpp"

namespace
{
    Sick::Ui::SickMenuCallbacks MakeCallbacks()
    {
        Sick::Ui::SickMenuCallbacks callbacks{};
        callbacks.godMode = [](bool enabled) {
            Sick::Backend::BackendApi::Get().SetGodMode(enabled);
        };
        callbacks.regularAction = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().RunScriptVmTest());
        };
        return callbacks;
    }
}

namespace Sick::Frontend
{
    FrontendCore::FrontendCore()
        : m_Menu(MakeCallbacks())
    {
        Tick();
    }

    void FrontendCore::Tick() noexcept
    {
        const auto snapshot = Backend::BackendApi::Get().Snapshot();
        m_Menu.State().godMode = snapshot.player.godMode.requested;
    }
}
