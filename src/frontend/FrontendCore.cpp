#include "FrontendCore.hpp"

#include "backend/BackendApi.hpp"

#include <utility>

namespace
{
    Sick::Ui::MenuColor UnpackColor(std::uint32_t value) noexcept
    {
        return {
            static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
            static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
            static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
            static_cast<std::uint8_t>(value & 0xFFU),
        };
    }

    Sick::Ui::SickMenuAsset ConvertAsset(const Sick::Backend::AssetEntry& asset)
    {
        return {asset.name, asset.path};
    }

    Sick::Ui::SickMenuAssetCatalog ConvertCatalog(const Sick::Backend::AssetCatalogSnapshot& source)
    {
        Sick::Ui::SickMenuAssetCatalog result{};
        result.generation = source.generation;
        result.themes.reserve(source.themes.size());
        result.images.reserve(source.images.size());
        result.fonts.reserve(source.fonts.size());
        result.scripts.reserve(source.scripts.size());

        for (const auto& theme : source.themes)
        {
            result.themes.push_back({
                .asset = ConvertAsset(theme.asset),
                .border = UnpackColor(theme.palette.border),
                .header = UnpackColor(theme.palette.header),
                .headerBand = UnpackColor(theme.palette.headerBand),
                .title = UnpackColor(theme.palette.title),
                .body = UnpackColor(theme.palette.body),
                .footer = UnpackColor(theme.palette.footer),
                .selected = UnpackColor(theme.palette.selected),
                .text = UnpackColor(theme.palette.text),
                .selectedText = UnpackColor(theme.palette.selectedText),
                .disabledText = UnpackColor(theme.palette.disabledText),
                .accent = UnpackColor(theme.palette.accent),
                .inactiveToggle = UnpackColor(theme.palette.inactiveToggle),
                .logoCyan = UnpackColor(theme.palette.logoCyan),
                .logoMagenta = UnpackColor(theme.palette.logoMagenta),
                .logoShadow = UnpackColor(theme.palette.logoShadow),
            });
        }
        for (const auto& asset : source.images)
            result.images.push_back(ConvertAsset(asset));
        for (const auto& asset : source.fonts)
            result.fonts.push_back(ConvertAsset(asset));
        for (const auto& asset : source.scripts)
            result.scripts.push_back(ConvertAsset(asset));
        return result;
    }

    const char* VehicleSpawnerStatusText(Sick::Backend::VehicleSpawnerState state) noexcept
    {
        using State = Sick::Backend::VehicleSpawnerState;
        switch (state)
        {
        case State::Idle: return "READY";
        case State::Queued: return "QUEUED";
        case State::Loading: return "LOADING MODEL";
        case State::Spawned: return "SPAWNED";
        case State::NativeUnavailable: return "NATIVE BACKEND UNAVAILABLE";
        case State::InvalidModel: return "INVALID VEHICLE MODEL";
        case State::TimedOut: return "MODEL LOAD TIMED OUT";
        case State::Failed: return "SPAWN FAILED";
        }
        return "UNKNOWN";
    }

    const char* SessionSwitchStatusText(Sick::Backend::SessionSwitchState state) noexcept
    {
        using State = Sick::Backend::SessionSwitchState;
        switch (state)
        {
        case State::Idle: return "READY";
        case State::Queued: return "QUEUED";
        case State::Switching: return "SWITCHING SESSION";
        case State::Complete: return "SWITCH REQUESTED";
        case State::ScriptUnavailable: return "SCRIPT RUNTIME UNAVAILABLE";
        case State::GlobalUnavailable: return "SESSION GLOBAL UNAVAILABLE";
        case State::Failed: return "SESSION SWITCH FAILED";
        }
        return "UNKNOWN";
    }

    const char* PersonalVehicleSaveStatusText(Sick::Backend::PersonalVehicleSaveState state) noexcept
    {
        using State = Sick::Backend::PersonalVehicleSaveState;
        switch (state)
        {
        case State::Idle: return "READY";
        case State::Queued: return "QUEUED";
        case State::Validating: return "VALIDATING VEHICLE";
        case State::WaitingForGarageSelection: return "SELECT GARAGE";
        case State::Complete: return "GARAGE FLOW COMPLETE";
        case State::NativeUnavailable: return "NATIVE BACKEND UNAVAILABLE";
        case State::ScriptUnavailable: return "SCRIPT RUNTIME UNAVAILABLE";
        case State::RewardScriptUnavailable: return "VEHICLE REWARD SCRIPT INACTIVE";
        case State::GlobalUnavailable: return "PERSONAL VEHICLE GLOBAL UNAVAILABLE";
        case State::NoVehicle: return "ENTER A VEHICLE";
        case State::InvalidVehicle: return "VEHICLE CANNOT BE SAVED";
        case State::AlreadyPersonal: return "ALREADY A PERSONAL VEHICLE";
        case State::Failed: return "GARAGE SAVE FAILED";
        }
        return "UNKNOWN";
    }

    Sick::Ui::SickMenuCallbacks MakeCallbacks()
    {
        Sick::Ui::SickMenuCallbacks callbacks{};
        callbacks.godMode = [](bool enabled) { Sick::Backend::BackendApi::Get().SetGodMode(enabled); };
        callbacks.infiniteOxygen = [](bool enabled) { Sick::Backend::BackendApi::Get().SetInfiniteOxygen(enabled); };
        callbacks.noRagdoll = [](bool enabled) { Sick::Backend::BackendApi::Get().SetNoRagdoll(enabled); };
        callbacks.superJump = [](bool enabled) { Sick::Backend::BackendApi::Get().SetSuperJump(enabled); };
        callbacks.seatBelt = [](bool enabled) { Sick::Backend::BackendApi::Get().SetSeatBelt(enabled); };
        callbacks.noWantedLevel = [](bool enabled) { Sick::Backend::BackendApi::Get().SetNoWantedLevel(enabled); };
        callbacks.wantedLevel = [](int level) { Sick::Backend::BackendApi::Get().SetWantedLevel(level); };
        callbacks.fastRun = [](bool enabled) { Sick::Backend::BackendApi::Get().SetFastRun(enabled); };
        callbacks.fastSwim = [](bool enabled) { Sick::Backend::BackendApi::Get().SetFastSwim(enabled); };
        callbacks.keepPlayerClean = [](bool enabled) { Sick::Backend::BackendApi::Get().SetKeepPlayerClean(enabled); };
        callbacks.aqualung = [](bool enabled) { Sick::Backend::BackendApi::Get().SetAqualung(enabled); };
        callbacks.noGravity = [](bool enabled) { Sick::Backend::BackendApi::Get().SetNoGravity(enabled); };
        callbacks.waterproof = [](bool enabled) { Sick::Backend::BackendApi::Get().SetWaterproof(enabled); };

        callbacks.vehicleGodMode = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleGodMode(enabled); };
        callbacks.vehicleAutoRepair = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleAutoRepair(enabled); };
        callbacks.vehicleKeepClean = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleKeepClean(enabled); };
        callbacks.vehicleEngineAlwaysOn = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleEngineAlwaysOn(enabled); };
        callbacks.vehicleNoGravity = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleNoGravity(enabled); };
        callbacks.vehicleNoCollision = [](bool enabled) { Sick::Backend::BackendApi::Get().SetVehicleNoCollision(enabled); };
        callbacks.repairVehicle = [] { Sick::Backend::BackendApi::Get().RepairVehicle(); };
        callbacks.cleanVehicle = [] { Sick::Backend::BackendApi::Get().CleanVehicle(); };
        callbacks.putVehicleOnGround = [] { Sick::Backend::BackendApi::Get().PutVehicleOnGround(); };
        callbacks.customizeVehicle = [](std::int32_t command, int a, int b, int c, int d) {
            static_cast<void>(Sick::Backend::BackendApi::Get().CustomizeCurrentVehicle(
                static_cast<Sick::Backend::VehicleCustomizationCommand>(command), a, b, c, d));
        };
        callbacks.spawnVehicle = [](std::string_view modelName, bool enterVehicle) {
            static_cast<void>(Sick::Backend::BackendApi::Get().SpawnVehicle(modelName, enterVehicle));
        };
        callbacks.switchOnlineSession = [](std::int32_t type) {
            static_cast<void>(Sick::Backend::BackendApi::Get().SwitchOnlineSession(
                static_cast<Sick::Backend::OnlineSessionType>(type)));
        };
        callbacks.saveCurrentVehicleToPersonalGarage = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().SaveCurrentVehicleToPersonalGarage());
        };

        callbacks.handlingValue = [](Sick::Handling::Field field, float value) {
            Sick::Backend::BackendApi::Get().SetHandlingValue(field, value);
        };
        callbacks.restoreOriginalHandling = [] { Sick::Backend::BackendApi::Get().RestoreOriginalHandling(); };
        callbacks.saveHandlingProfile = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().SaveHandlingProfile());
        };
        callbacks.loadHandlingProfile = [](const std::string& name) {
            static_cast<void>(Sick::Backend::BackendApi::Get().LoadHandlingProfile(name));
        };
        callbacks.refreshHandlingProfiles = [] {
            static_cast<void>(Sick::Backend::BackendApi::Get().RefreshHandlingProfiles());
        };

        callbacks.regularAction = [] { static_cast<void>(Sick::Backend::BackendApi::Get().RunScriptVmTest()); };
        callbacks.preferencesChanged = [](const Sick::Ui::SickMenuPreferences& preferences) {
            Sick::Backend::BackendApi::Get().SetPreferences({
                .scale = preferences.scale,
                .left = preferences.left,
                .top = preferences.top,
                .theme = preferences.theme,
                .banner = preferences.banner,
                .font = preferences.font,
            });
        };
        callbacks.refreshAssets = [] { static_cast<void>(Sick::Backend::BackendApi::Get().RefreshAssets()); };
        callbacks.saveConfiguration = [] { static_cast<void>(Sick::Backend::BackendApi::Get().SaveConfiguration()); };
        callbacks.exitGta = [] { Sick::Backend::BackendApi::Get().RequestExitGta(); };
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
        auto& backend = Backend::BackendApi::Get();
        backend.SetHandlingEditorActive(m_Menu.IsHandlingPageActive());
        const auto snapshot = backend.Snapshot();

        m_Menu.State().godMode = snapshot.player.godMode.requested;
        m_Menu.State().infiniteOxygen = snapshot.player.infiniteOxygen.requested;
        m_Menu.State().noRagdoll = snapshot.player.noRagdoll.requested;
        m_Menu.State().superJump = snapshot.player.superJump.requested;
        m_Menu.State().seatBelt = snapshot.player.seatBelt.requested;
        m_Menu.State().noWantedLevel = snapshot.player.noWantedLevel.requested;
        m_Menu.State().wantedLevel = snapshot.player.wantedLevel;
        m_Menu.State().fastRun = snapshot.player.fastRun.requested;
        m_Menu.State().fastSwim = snapshot.player.fastSwim.requested;
        m_Menu.State().keepPlayerClean = snapshot.player.keepPlayerClean.requested;
        m_Menu.State().aqualung = snapshot.player.aqualung.requested;
        m_Menu.State().noGravity = snapshot.player.noGravity.requested;
        m_Menu.State().waterproof = snapshot.player.waterproof.requested;

        m_Menu.State().vehicleGodMode = snapshot.vehicle.godMode.requested;
        m_Menu.State().vehicleAutoRepair = snapshot.vehicle.autoRepair.requested;
        m_Menu.State().vehicleKeepClean = snapshot.vehicle.keepClean.requested;
        m_Menu.State().vehicleEngineAlwaysOn = snapshot.vehicle.engineAlwaysOn.requested;
        m_Menu.State().vehicleNoGravity = snapshot.vehicle.noGravity.requested;
        m_Menu.State().vehicleNoCollision = snapshot.vehicle.noCollision.requested;
        m_Menu.State().vehicleSpawnerBusy = snapshot.vehicleSpawner.busy;
        m_Menu.State().vehicleSpawnerStatus = VehicleSpawnerStatusText(snapshot.vehicleSpawner.state);
        m_Menu.State().onlineSessionBusy = snapshot.sessionSwitch.busy;
        m_Menu.State().onlineSessionStatus = SessionSwitchStatusText(snapshot.sessionSwitch.state);
        m_Menu.State().personalVehicleSaveBusy = snapshot.personalVehicleSave.busy;
        m_Menu.State().personalVehicleSaveStatus = PersonalVehicleSaveStatusText(snapshot.personalVehicleSave.state);

        m_Menu.State().handlingAvailable = snapshot.handling.backendAvailable;
        m_Menu.State().handlingVehicleAttached = snapshot.handling.vehicleAttached;
        if (snapshot.handling.revision != m_HandlingRevision)
        {
            m_HandlingRevision = snapshot.handling.revision;
            m_Menu.State().handlingValues = snapshot.handling.values;
        }

        if (!m_PreferencesLoaded && snapshot.initialized)
        {
            const auto preferences = backend.Preferences();
            m_Menu.SetPreferences({
                .scale = preferences.scale,
                .left = preferences.left,
                .top = preferences.top,
                .theme = preferences.theme,
                .banner = preferences.banner,
                .font = preferences.font,
            });
            m_PreferencesLoaded = true;
        }

        const auto assetGeneration = backend.AssetGeneration();
        if (assetGeneration != 0 && assetGeneration != m_AssetGeneration)
        {
            auto catalog = ConvertCatalog(backend.Assets());
            m_AssetGeneration = catalog.generation;
            m_Menu.SetAssetCatalog(std::move(catalog));
        }

        const auto handlingProfileGeneration = backend.HandlingProfileGeneration();
        if (handlingProfileGeneration != 0 && handlingProfileGeneration != m_HandlingProfileGeneration)
        {
            auto profiles = backend.HandlingProfiles();
            m_HandlingProfileGeneration = profiles.generation;
            m_Menu.SetHandlingProfiles(profiles.generation, std::move(profiles.names));
        }
    }
}
