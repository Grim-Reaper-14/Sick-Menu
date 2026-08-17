#include "BackendApi.hpp"

#include "BackendCore.hpp"
#include "backend/system/LoggerApi.hpp"
#include "game/scripts/ScriptFunctionCatalog.hpp"

namespace Sick::Backend
{
    BackendApi& BackendApi::Get() noexcept
    {
        static BackendApi api;
        return api;
    }

    void BackendApi::SetGodMode(bool enabled) noexcept
    {
        BackendCore::Get().SetGodMode(enabled);
    }

    bool BackendApi::SaveProfile(std::string_view name)
    {
        return BackendCore::Get().SaveProfile(name);
    }

    bool BackendApi::LoadProfile(std::string_view name)
    {
        return BackendCore::Get().LoadProfile(name);
    }

    bool BackendApi::RunScriptVmTest()
    {
        return BackendCore::Get().QueueScript([] {
            const auto* specification = Game::Scripts::ScriptFunctionCatalog::Find(
                Game::Scripts::KnownScriptFunction::GetFmmcVariationCount);
            if (!specification)
            {
                System::LoggerApi::Get().Warn("script", "Script VM test unavailable: function specification missing");
                return;
            }

            const auto function = specification->Bind();
            const auto result = function.TryCall<int>();
            if (result)
                System::LoggerApi::Get().Info("script", "Script VM test succeeded");
            else
                System::LoggerApi::Get().Warn("script", "Script VM test unavailable (freemode not loaded or signature changed)");
        });
    }

    BackendSnapshot BackendApi::Snapshot() const noexcept
    {
        return BackendCore::Get().Snapshot();
    }
}
