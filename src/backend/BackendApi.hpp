#pragma once

#include "BackendTypes.hpp"

#include <cstdint>
#include <string_view>

namespace Sick::Backend
{
    class BackendApi final
    {
    public:
        static BackendApi& Get() noexcept;

        void SetGodMode(bool enabled) noexcept;
        [[nodiscard]] bool SaveProfile(std::string_view name);
        [[nodiscard]] bool LoadProfile(std::string_view name);
        [[nodiscard]] bool SaveConfiguration();

        [[nodiscard]] AssetCatalogSnapshot Assets() const;
        [[nodiscard]] std::uint64_t AssetGeneration() const noexcept;
        [[nodiscard]] bool RefreshAssets();
        [[nodiscard]] MenuPreferences Preferences() const;
        void SetPreferences(MenuPreferences preferences) noexcept;

        void RequestExitGta() noexcept;
        [[nodiscard]] bool ExitGtaRequested() const noexcept;

        [[nodiscard]] bool RunScriptVmTest();
        [[nodiscard]] BackendSnapshot Snapshot() const noexcept;

    private:
        BackendApi() = default;
    };
}
