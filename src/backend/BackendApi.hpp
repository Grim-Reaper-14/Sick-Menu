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
        void SetInfiniteOxygen(bool enabled) noexcept;
        void SetNoRagdoll(bool enabled) noexcept;
        void SetSuperJump(bool enabled) noexcept;
        void SetSeatBelt(bool enabled) noexcept;
        void SetNoWantedLevel(bool enabled) noexcept;
        void SetWantedLevel(int level) noexcept;
        void SetFastRun(bool enabled) noexcept;
        void SetFastSwim(bool enabled) noexcept;
        void SetKeepPlayerClean(bool enabled) noexcept;
        void SetAqualung(bool enabled) noexcept;
        void SetNoGravity(bool enabled) noexcept;
        void SetWaterproof(bool enabled) noexcept;

        void SetVehicleGodMode(bool enabled) noexcept;
        void SetVehicleAutoRepair(bool enabled) noexcept;
        void SetVehicleKeepClean(bool enabled) noexcept;
        void SetVehicleEngineAlwaysOn(bool enabled) noexcept;
        void SetVehicleNoGravity(bool enabled) noexcept;
        void SetVehicleNoCollision(bool enabled) noexcept;
        void RepairVehicle() noexcept;
        void CleanVehicle() noexcept;
        void PutVehicleOnGround() noexcept;
        [[nodiscard]] bool CustomizeCurrentVehicle(
            VehicleCustomizationCommand command,
            int a = 0,
            int b = 0,
            int c = 0,
            int d = 0);
        [[nodiscard]] bool SpawnVehicle(std::string_view modelName, bool enterVehicle);
        [[nodiscard]] bool SwitchOnlineSession(OnlineSessionType type);
        [[nodiscard]] bool SaveCurrentVehicleToPersonalGarage();

        void SetHandlingEditorActive(bool active) noexcept;
        void SetHandlingValue(Handling::Field field, float value) noexcept;
        void RestoreOriginalHandling() noexcept;
        [[nodiscard]] bool SaveHandlingProfile();
        [[nodiscard]] bool LoadHandlingProfile(std::string_view name);
        [[nodiscard]] bool RefreshHandlingProfiles();
        [[nodiscard]] HandlingProfileCatalogSnapshot HandlingProfiles() const;
        [[nodiscard]] std::uint64_t HandlingProfileGeneration() const noexcept;

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
