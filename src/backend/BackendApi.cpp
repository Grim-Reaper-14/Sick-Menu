#include "BackendApi.hpp"

#include "BackendCore.hpp"
#include "backend/system/LoggerApi.hpp"
#include "game/scripts/ScriptFunctionCatalog.hpp"

#include <utility>

namespace Sick::Backend
{
    BackendApi& BackendApi::Get() noexcept
    {
        static BackendApi api;
        return api;
    }

    void BackendApi::SetGodMode(bool enabled) noexcept { BackendCore::Get().SetGodMode(enabled); }
    void BackendApi::SetInfiniteOxygen(bool enabled) noexcept { BackendCore::Get().SetInfiniteOxygen(enabled); }
    void BackendApi::SetNoRagdoll(bool enabled) noexcept { BackendCore::Get().SetNoRagdoll(enabled); }
    void BackendApi::SetSuperJump(bool enabled) noexcept { BackendCore::Get().SetSuperJump(enabled); }
    void BackendApi::SetSeatBelt(bool enabled) noexcept { BackendCore::Get().SetSeatBelt(enabled); }
    void BackendApi::SetNoWantedLevel(bool enabled) noexcept { BackendCore::Get().SetNoWantedLevel(enabled); }
    void BackendApi::SetWantedLevel(int level) noexcept { BackendCore::Get().SetWantedLevel(level); }
    void BackendApi::SetFastRun(bool enabled) noexcept { BackendCore::Get().SetFastRun(enabled); }
    void BackendApi::SetFastSwim(bool enabled) noexcept { BackendCore::Get().SetFastSwim(enabled); }
    void BackendApi::SetKeepPlayerClean(bool enabled) noexcept { BackendCore::Get().SetKeepPlayerClean(enabled); }
    void BackendApi::SetAqualung(bool enabled) noexcept { BackendCore::Get().SetAqualung(enabled); }
    void BackendApi::SetNoGravity(bool enabled) noexcept { BackendCore::Get().SetNoGravity(enabled); }
    void BackendApi::SetWaterproof(bool enabled) noexcept { BackendCore::Get().SetWaterproof(enabled); }

    bool BackendApi::SaveProfile(std::string_view name) { return BackendCore::Get().SaveProfile(name); }
    bool BackendApi::LoadProfile(std::string_view name) { return BackendCore::Get().LoadProfile(name); }
    bool BackendApi::SaveConfiguration() { return BackendCore::Get().SaveConfiguration(); }
    AssetCatalogSnapshot BackendApi::Assets() const { return BackendCore::Get().Assets(); }
    std::uint64_t BackendApi::AssetGeneration() const noexcept { return BackendCore::Get().AssetGeneration(); }
    bool BackendApi::RefreshAssets() { return BackendCore::Get().RefreshAssets(); }
    MenuPreferences BackendApi::Preferences() const { return BackendCore::Get().Preferences(); }
    void BackendApi::SetPreferences(MenuPreferences preferences) noexcept { BackendCore::Get().SetPreferences(std::move(preferences)); }
    void BackendApi::RequestExitGta() noexcept { BackendCore::Get().RequestExitGta(); }
    bool BackendApi::ExitGtaRequested() const noexcept { return BackendCore::Get().ExitGtaRequested(); }

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
