from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"missing patch anchor: {label}")
    if text.count(old) != 1:
        raise RuntimeError(f"patch anchor not unique ({text.count(old)}): {label}")
    return text.replace(old, new, 1)


# 1) Compact Kiddion-style choice rendering: renderer already adds < value >,
# so Choice::ValueText must return only the selected value (no [ x / y ] suffix).
path = "src/ui/menu/Menu.cpp"
text = read(path)
old = '''        option.m_ValueText = [&index, sharedValues]() {
            if (sharedValues->empty()) return std::string{};
            return (*sharedValues)[index] + " [ " + std::to_string(index + 1) +
                " / " + std::to_string(sharedValues->size()) + " ]";
        };'''
new = '''        option.m_ValueText = [&index, sharedValues]() {
            if (sharedValues->empty()) return std::string{};
            return (*sharedValues)[index];
        };'''
text = replace_once(text, old, new, "choice value formatting")
write(path, text)


# 2) Numpad 0 Back: ImGui may report keypad 0 as Insert when NumLock is off.
path = "src/ui/menu/ImGuiMenuBackend.hpp"
text = read(path)
text = replace_once(
    text,
    '        ImGuiKey back{ImGuiKey_Keypad0};\n',
    '        ImGuiKey back{ImGuiKey_Keypad0};\n        ImGuiKey backAlternate{ImGuiKey_Insert};\n',
    "alternate back key")
old = '''            if (ImGui::IsKeyPressed(keys.back, false))
                controller.Handle(MenuInput::Back);'''
new = '''            const bool backPressed = ImGui::IsKeyPressed(keys.back, false) ||
                (keys.back == ImGuiKey_Keypad0 && ImGui::IsKeyPressed(keys.backAlternate, false));
            if (backPressed)
                controller.Handle(MenuInput::Back);'''
text = replace_once(text, old, new, "controller back handling")
old = '''            if (ImGui::IsKeyPressed(keys.back, false))
                menu.Handle(MenuInput::Back);'''
new = '''            const bool backPressed = ImGui::IsKeyPressed(keys.back, false) ||
                (keys.back == ImGuiKey_Keypad0 && ImGui::IsKeyPressed(keys.backAlternate, false));
            if (backPressed)
                menu.Handle(MenuInput::Back);'''
text = replace_once(text, old, new, "menu back handling")
write(path, text)


# 3) Backend command enum for real current-vehicle customization.
path = "src/backend/BackendTypes.hpp"
text = read(path)
anchor = '''    enum class SessionSwitchState : std::uint8_t
'''
insert = '''    enum class VehicleCustomizationCommand : std::int32_t
    {
        SetMod = 0,
        ToggleMod = 1,
        SetWheelType = 2,
        SetPrimaryModColor = 3,
        SetSecondaryModColor = 4,
        SetExtraColours = 5,
        SetInteriorColor = 6,
        SetDashboardColor = 7,
        SetNeonEnabled = 8,
        SetNeonColor = 9,
        SetTireSmokeColor = 10,
        SetXenonColor = 11,
        SetExtra = 12,
        SetBulletproofTires = 13,
        MaxVehicle = 14,
    };

'''
text = replace_once(text, anchor, insert + anchor, "vehicle customization enum")
write(path, text)


# 4) Backend public API/core declarations.
for path in ("src/backend/BackendCore.hpp", "src/backend/BackendApi.hpp"):
    text = read(path)
    anchor = '        void PutVehicleOnGround() noexcept;\n'
    addition = '''        [[nodiscard]] bool CustomizeCurrentVehicle(
            VehicleCustomizationCommand command,
            int a = 0,
            int b = 0,
            int c = 0,
            int d = 0);
'''
    text = replace_once(text, anchor, anchor + addition, f"customization declaration in {path}")
    write(path, text)


# 5) BackendApi forwarding wrapper.
path = "src/backend/BackendApi.cpp"
text = read(path)
anchor = '    void BackendApi::PutVehicleOnGround() noexcept { BackendCore::Get().PutVehicleOnGround(); }\n'
addition = '''    bool BackendApi::CustomizeCurrentVehicle(
        VehicleCustomizationCommand command,
        int a,
        int b,
        int c,
        int d)
    {
        return BackendCore::Get().CustomizeCurrentVehicle(command, a, b, c, d);
    }
'''
text = replace_once(text, anchor, anchor + addition, "BackendApi customization wrapper")
write(path, text)


# 6) Real Enhanced native backend. Hashes are current Enhanced translations from
# YimMenuV2 enhanced/src/game/gta/invoker/crossmap.txt.
path = "src/backend/BackendCore.cpp"
text = read(path)
anchor = '''        void ClearVehicleRewardLocals() noexcept
        {
            auto* reward = Sick::Game::Enhanced::EnhancedScriptHost::LocalAddress(RewardScript, 148, 47);
            if (!reward)
                return;
            reward[3] = 0;
            reward[4] = 0;
            reward[5] = 0;
            reward[6] = 0;
        }
'''
addition = '''
        constexpr Sick::Game::Natives::NativeHash SetVehicleModKitHash = 0xB5AD06DDA85E2E8FULL;
        constexpr Sick::Game::Natives::NativeHash GetNumVehicleModsHash = 0x5B59C12A02157D00ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModHash = 0x8450270DC5896D39ULL;
        constexpr Sick::Game::Natives::NativeHash ToggleVehicleModHash = 0xF5501FF9869DAC7CULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleWheelTypeHash = 0xE33678A9AE50A01BULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModColor1Hash = 0xA5277ECCD081FCC1ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleModColor2Hash = 0x941B1F179D6AE19AULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleExtraColoursHash = 0xBB361D7264AC4FD8ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleInteriorColorHash = 0xC0C8E6AAA00F1A58ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleDashboardColorHash = 0x77B012A683295B6EULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleNeonEnabledHash = 0xE62930EC6FAABCA5ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleNeonColourHash = 0xEAB8A43F6621850FULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleTireSmokeColorHash = 0x5DA0536AEAD1FF31ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleXenonColorHash = 0x89D1FDCA3735A1E0ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleExtraHash = 0xD772F6AA66750D2BULL;
        constexpr Sick::Game::Natives::NativeHash DoesExtraExistHash = 0x579FA5568DE0C2A0ULL;
        constexpr Sick::Game::Natives::NativeHash SetVehicleTyresCanBurstHash = 0x439C904840715871ULL;
'''
text = replace_once(text, anchor, anchor + addition, "Enhanced customization hashes")

anchor = '    void BackendCore::PutVehicleOnGround() noexcept { m_Features.PutVehicleOnGround(); }\n\n'
addition = r'''    bool BackendCore::CustomizeCurrentVehicle(
        VehicleCustomizationCommand command,
        int a,
        int b,
        int c,
        int d)
    {
        if (!m_NativeReady.load(std::memory_order_acquire))
            return false;

        return QueueNative([command, a, b, c, d] {
            const auto ped = Game::Natives::PLAYER::PLAYER_PED_ID();
            const auto vehicle = Game::Natives::PED::GET_VEHICLE_PED_IS_IN(ped, false);
            if (vehicle == 0 || !Game::Natives::ENTITY::DOES_ENTITY_EXIST(vehicle))
                return;

            using Invoker = Game::Natives::NativeInvoker;
            static_cast<void>(Invoker::TryCallVoid(SetVehicleModKitHash, vehicle, 0));

            switch (command)
            {
            case VehicleCustomizationCommand::SetMod:
            {
                const int slot = std::clamp(a, 0, 49);
                const auto count = Invoker::TryCall<int>(GetNumVehicleModsHash, vehicle, slot);
                if (!count || *count <= 0)
                {
                    static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, -1, false));
                    return;
                }
                const int index = std::clamp(b, -1, *count - 1);
                static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, index, c != 0));
                return;
            }
            case VehicleCustomizationCommand::ToggleMod:
                static_cast<void>(Invoker::TryCallVoid(
                    ToggleVehicleModHash,
                    vehicle,
                    std::clamp(a, 0, 49),
                    b != 0));
                return;
            case VehicleCustomizationCommand::SetWheelType:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleWheelTypeHash, vehicle, std::clamp(a, 0, 12)));
                return;
            case VehicleCustomizationCommand::SetPrimaryModColor:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleModColor1Hash,
                    vehicle,
                    std::clamp(a, 0, 5),
                    std::clamp(b, 0, 160),
                    0));
                return;
            case VehicleCustomizationCommand::SetSecondaryModColor:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleModColor2Hash,
                    vehicle,
                    std::clamp(a, 0, 5),
                    std::clamp(b, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetExtraColours:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleExtraColoursHash,
                    vehicle,
                    std::clamp(a, 0, 160),
                    std::clamp(b, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetInteriorColor:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleInteriorColorHash, vehicle, std::clamp(a, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetDashboardColor:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleDashboardColorHash, vehicle, std::clamp(a, 0, 160)));
                return;
            case VehicleCustomizationCommand::SetNeonEnabled:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleNeonEnabledHash,
                    vehicle,
                    std::clamp(a, 0, 3),
                    b != 0));
                return;
            case VehicleCustomizationCommand::SetNeonColor:
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleNeonColourHash,
                    vehicle,
                    std::clamp(a, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(c, 0, 255)));
                return;
            case VehicleCustomizationCommand::SetTireSmokeColor:
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 20, true));
                static_cast<void>(Invoker::TryCallVoid(
                    SetVehicleTireSmokeColorHash,
                    vehicle,
                    std::clamp(a, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(c, 0, 255)));
                return;
            case VehicleCustomizationCommand::SetXenonColor:
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 22, true));
                static_cast<void>(Invoker::TryCallVoid(SetVehicleXenonColorHash, vehicle, std::clamp(a, -1, 12)));
                return;
            case VehicleCustomizationCommand::SetExtra:
            {
                const int extra = std::clamp(a, 1, 14);
                const auto exists = Invoker::TryCall<bool>(DoesExtraExistHash, vehicle, extra);
                if (exists && *exists)
                    static_cast<void>(Invoker::TryCallVoid(SetVehicleExtraHash, vehicle, extra, b == 0));
                return;
            }
            case VehicleCustomizationCommand::SetBulletproofTires:
                static_cast<void>(Invoker::TryCallVoid(SetVehicleTyresCanBurstHash, vehicle, a == 0));
                return;
            case VehicleCustomizationCommand::MaxVehicle:
                for (int slot = 0; slot < 50; ++slot)
                {
                    if (slot == 17 || slot == 18 || slot == 19 || slot == 20 || slot == 21 || slot == 22)
                        continue;
                    const auto count = Invoker::TryCall<int>(GetNumVehicleModsHash, vehicle, slot);
                    if (count && *count > 0)
                        static_cast<void>(Invoker::TryCallVoid(SetVehicleModHash, vehicle, slot, *count - 1, false));
                }
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 18, true));
                static_cast<void>(Invoker::TryCallVoid(ToggleVehicleModHash, vehicle, 22, true));
                static_cast<void>(Invoker::TryCallVoid(SetVehicleTyresCanBurstHash, vehicle, false));
                return;
            }
        });
    }

'''
text = replace_once(text, anchor, anchor + addition, "BackendCore customization implementation")
write(path, text)


# 7) UI state/callback/pages.
path = "src/ui/menu/SickMenu.hpp"
text = read(path)
text = replace_once(text, '#include <cstddef>\n', '#include <array>\n#include <cstddef>\n', "array include")
anchor = '        bool vehicleNoCollision{};\n'
addition = '''        std::array<int, 50> vehicleMods{};
        std::size_t vehicleWheelType{};
        std::size_t vehiclePrimaryPaintType{1};
        std::size_t vehicleSecondaryPaintType{1};
        int vehiclePrimaryColor{};
        int vehicleSecondaryColor{};
        int vehiclePearlescentColor{};
        int vehicleWheelColor{};
        int vehicleInteriorColor{};
        int vehicleDashboardColor{};
        bool vehicleTurbo{};
        bool vehicleXenon{};
        std::size_t vehicleXenonColor{};
        bool vehicleTireSmoke{};
        bool vehicleBulletproofTires{true};
        std::array<bool, 4> vehicleNeonEnabled{};
        int vehicleNeonRed{255};
        int vehicleNeonGreen{};
        int vehicleNeonBlue{255};
        int vehicleSmokeRed{255};
        int vehicleSmokeGreen{255};
        int vehicleSmokeBlue{255};
        std::array<bool, 14> vehicleExtras{};
'''
text = replace_once(text, anchor, anchor + addition, "vehicle customization UI state")
anchor = '        std::function<void()> putVehicleOnGround;\n'
addition = '''        std::function<void(std::int32_t, int, int, int, int)> customizeVehicle;
'''
text = replace_once(text, anchor, anchor + addition, "vehicle customization callback")
anchor = '        void BuildVehicleSpawnerPages();\n'
text = replace_once(text, anchor, '        void BuildVehicleCustomizationPages();\n' + anchor, "vehicle customization builder declaration")
anchor = '        MenuPage m_HandlingPage{"HANDLING"};\n'
addition = '''        MenuPage m_VehicleCustomizationPage{"VEHICLE / CUSTOMIZATION"};
        MenuPage m_VehicleModsPage{"CUSTOMIZATION / MODS"};
        MenuPage m_VehicleWheelsPage{"CUSTOMIZATION / WHEELS"};
        MenuPage m_VehiclePaintPage{"CUSTOMIZATION / PAINT"};
        MenuPage m_VehicleLightsPage{"CUSTOMIZATION / LIGHTS & NEON"};
        MenuPage m_VehicleExtrasPage{"CUSTOMIZATION / EXTRAS"};
'''
text = replace_once(text, anchor, addition + anchor, "vehicle customization pages")
write(path, text)


# 8) Build the customization menu and wire callbacks.
path = "src/ui/menu/SickMenu.cpp"
text = read(path)
anchor = '''    constexpr std::array<std::int32_t, 11> OnlineSessionValues{
        0, 1, 13, 3, 12, 2, 6, 9, 11, 10, -1,
    };
'''
addition = r'''
    enum class VehicleCustomizationUiCommand : std::int32_t
    {
        SetMod = 0,
        ToggleMod = 1,
        SetWheelType = 2,
        SetPrimaryModColor = 3,
        SetSecondaryModColor = 4,
        SetExtraColours = 5,
        SetInteriorColor = 6,
        SetDashboardColor = 7,
        SetNeonEnabled = 8,
        SetNeonColor = 9,
        SetTireSmokeColor = 10,
        SetXenonColor = 11,
        SetExtra = 12,
        SetBulletproofTires = 13,
        MaxVehicle = 14,
    };

    struct VehicleModSpec
    {
        const char* label;
        int slot;
    };

    constexpr std::array<VehicleModSpec, 23> VehicleModSlots{{
        {"Spoiler", 0}, {"Front Bumper", 1}, {"Rear Bumper", 2}, {"Side Skirt", 3},
        {"Exhaust", 4}, {"Frame", 5}, {"Grille", 6}, {"Hood", 7}, {"Left Fender", 8},
        {"Right Fender", 9}, {"Roof", 10}, {"Engine", 11}, {"Brakes", 12},
        {"Transmission", 13}, {"Horn", 14}, {"Suspension", 15}, {"Armor", 16},
        {"Plate Holder", 25}, {"Vanity Plate", 26}, {"Trim Design", 27}, {"Ornaments", 28},
        {"Dashboard", 29}, {"Dial Design", 30},
    }};

    const std::vector<std::string> WheelTypes{
        "Sport", "Muscle", "Lowrider", "SUV", "Offroad", "Tuner", "Bike", "High End",
        "Benny's Original", "Benny's Bespoke", "Open Wheel", "Street", "Track",
    };

    const std::vector<std::string> PaintTypes{
        "Normal", "Metallic", "Pearlescent", "Matte", "Metal", "Chrome",
    };

    const std::vector<std::string> XenonColors{
        "White", "Blue", "Electric Blue", "Mint Green", "Lime Green", "Yellow", "Golden",
        "Orange", "Red", "Pony Pink", "Hot Pink", "Purple", "Blacklight", "Stock",
    };
'''
text = replace_once(text, anchor, anchor + addition, "vehicle customization UI constants")

anchor = '''        m_VehiclePage.AddSubmenu("Handling", m_HandlingPage)
            .Describe("Opens the build-aware vehicle handling editor and saved handling profiles.");
'''
addition = '''        m_VehiclePage.AddSubmenu("Customization", m_VehicleCustomizationPage)
            .Describe("Kiddion-style vehicle editor with Yim-style mod, wheel, paint, neon and extras categories.");
'''
text = replace_once(text, anchor, anchor + addition, "Customization submenu")
text = replace_once(text, '        BuildHandlingPages();\n', '        BuildVehicleCustomizationPages();\n        BuildHandlingPages();\n', "build customization pages")

anchor = '    void SickMenu::BuildVehicleSpawnerPages()\n'
method = r'''    void SickMenu::BuildVehicleCustomizationPages()
    {
        const auto command = [this](VehicleCustomizationUiCommand action, int a = 0, int b = 0, int c = 0, int d = 0) {
            Notify(m_Callbacks.customizeVehicle, static_cast<std::int32_t>(action), a, b, c, d);
        };

        m_VehicleCustomizationPage.AddSubmenu("Mods", m_VehicleModsPage)
            .Describe("Body, performance and interior mod slots for the vehicle you are currently driving.");
        m_VehicleCustomizationPage.AddSubmenu("Wheels", m_VehicleWheelsPage)
            .Describe("Wheel family, wheel indexes and bulletproof tire controls.");
        m_VehicleCustomizationPage.AddSubmenu("Paint", m_VehiclePaintPage)
            .Describe("Primary, secondary, pearlescent, wheel, interior and dashboard colors.");
        m_VehicleCustomizationPage.AddSubmenu("Lights & Neon", m_VehicleLightsPage)
            .Describe("Xenon headlights, neon zones/colors and tire-smoke RGB.");
        m_VehicleCustomizationPage.AddSubmenu("Extras", m_VehicleExtrasPage)
            .Describe("Toggles supported vehicle extras 1 through 14.");
        m_VehicleCustomizationPage.AddAction("Max Vehicle", [command]() mutable {
            command(VehicleCustomizationUiCommand::MaxVehicle);
        }).Describe("Applies the highest available mod in each supported slot, enables turbo/xenon and bulletproof tires.");

        for (const auto& spec : VehicleModSlots)
        {
            m_VehicleModsPage.AddInteger(
                spec.label,
                m_State.vehicleMods[static_cast<std::size_t>(spec.slot)],
                -1,
                100,
                1,
                [command, slot = spec.slot](int index) mutable {
                    command(VehicleCustomizationUiCommand::SetMod, slot, index, 0, 0);
                })
                .Describe("-1 restores the stock part; values above the vehicle's real mod count are safely clamped by the backend.");
        }
        m_VehicleModsPage.AddToggle("Turbo", m_State.vehicleTurbo, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 18, enabled ? 1 : 0, 0, 0);
        });

        m_VehicleWheelsPage.AddChoice("Wheel Type", m_State.vehicleWheelType, WheelTypes, [command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetWheelType, static_cast<int>(index), 0, 0, 0);
        });
        m_VehicleWheelsPage.AddInteger("Front Wheel", m_State.vehicleMods[23], -1, 100, 1, [command](int index) mutable {
            command(VehicleCustomizationUiCommand::SetMod, 23, index, 0, 0);
        });
        m_VehicleWheelsPage.AddInteger("Rear Wheel", m_State.vehicleMods[24], -1, 100, 1, [command](int index) mutable {
            command(VehicleCustomizationUiCommand::SetMod, 24, index, 0, 0);
        });
        m_VehicleWheelsPage.AddToggle("Bulletproof Tires", m_State.vehicleBulletproofTires, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::SetBulletproofTires, enabled ? 1 : 0, 0, 0, 0);
        });

        m_VehiclePaintPage.AddChoice("Primary Paint Type", m_State.vehiclePrimaryPaintType, PaintTypes, [this, command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetPrimaryModColor, static_cast<int>(index), m_State.vehiclePrimaryColor, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Primary Color", m_State.vehiclePrimaryColor, 0, 160, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetPrimaryModColor, static_cast<int>(m_State.vehiclePrimaryPaintType), color, 0, 0);
        });
        m_VehiclePaintPage.AddChoice("Secondary Paint Type", m_State.vehicleSecondaryPaintType, PaintTypes, [this, command](std::size_t index) mutable {
            command(VehicleCustomizationUiCommand::SetSecondaryModColor, static_cast<int>(index), m_State.vehicleSecondaryColor, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Secondary Color", m_State.vehicleSecondaryColor, 0, 160, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetSecondaryModColor, static_cast<int>(m_State.vehicleSecondaryPaintType), color, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Pearlescent Color", m_State.vehiclePearlescentColor, 0, 160, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetExtraColours, color, m_State.vehicleWheelColor, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Wheel Color", m_State.vehicleWheelColor, 0, 160, 1, [this, command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetExtraColours, m_State.vehiclePearlescentColor, color, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Interior Color", m_State.vehicleInteriorColor, 0, 160, 1, [command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetInteriorColor, color, 0, 0, 0);
        });
        m_VehiclePaintPage.AddInteger("Dashboard Color", m_State.vehicleDashboardColor, 0, 160, 1, [command](int color) mutable {
            command(VehicleCustomizationUiCommand::SetDashboardColor, color, 0, 0, 0);
        });

        m_VehicleLightsPage.AddToggle("Xenon Headlights", m_State.vehicleXenon, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 22, enabled ? 1 : 0, 0, 0);
        });
        m_VehicleLightsPage.AddChoice("Xenon Color", m_State.vehicleXenonColor, XenonColors, [command](std::size_t index) mutable {
            const int nativeIndex = index + 1 == XenonColors.size() ? -1 : static_cast<int>(index);
            command(VehicleCustomizationUiCommand::SetXenonColor, nativeIndex, 0, 0, 0);
        });
        constexpr std::array<const char*, 4> NeonLabels{"Neon Left", "Neon Right", "Neon Front", "Neon Back"};
        for (std::size_t index = 0; index < NeonLabels.size(); ++index)
        {
            m_VehicleLightsPage.AddToggle(NeonLabels[index], m_State.vehicleNeonEnabled[index], [command, index](bool enabled) mutable {
                command(VehicleCustomizationUiCommand::SetNeonEnabled, static_cast<int>(index), enabled ? 1 : 0, 0, 0);
            });
        }
        m_VehicleLightsPage.AddInteger("Neon Red", m_State.vehicleNeonRed, 0, 255);
        m_VehicleLightsPage.AddInteger("Neon Green", m_State.vehicleNeonGreen, 0, 255);
        m_VehicleLightsPage.AddInteger("Neon Blue", m_State.vehicleNeonBlue, 0, 255);
        m_VehicleLightsPage.AddAction("Apply Neon Color", [this, command]() mutable {
            command(VehicleCustomizationUiCommand::SetNeonColor, m_State.vehicleNeonRed, m_State.vehicleNeonGreen, m_State.vehicleNeonBlue, 0);
        });
        m_VehicleLightsPage.AddToggle("Tire Smoke", m_State.vehicleTireSmoke, [command](bool enabled) mutable {
            command(VehicleCustomizationUiCommand::ToggleMod, 20, enabled ? 1 : 0, 0, 0);
        });
        m_VehicleLightsPage.AddInteger("Smoke Red", m_State.vehicleSmokeRed, 0, 255);
        m_VehicleLightsPage.AddInteger("Smoke Green", m_State.vehicleSmokeGreen, 0, 255);
        m_VehicleLightsPage.AddInteger("Smoke Blue", m_State.vehicleSmokeBlue, 0, 255);
        m_VehicleLightsPage.AddAction("Apply Tire Smoke Color", [this, command]() mutable {
            command(VehicleCustomizationUiCommand::SetTireSmokeColor, m_State.vehicleSmokeRed, m_State.vehicleSmokeGreen, m_State.vehicleSmokeBlue, 0);
        });

        for (std::size_t index = 0; index < m_State.vehicleExtras.size(); ++index)
        {
            const int extraId = static_cast<int>(index) + 1;
            m_VehicleExtrasPage.AddToggle("Extra " + std::to_string(extraId), m_State.vehicleExtras[index], [command, extraId](bool enabled) mutable {
                command(VehicleCustomizationUiCommand::SetExtra, extraId, enabled ? 1 : 0, 0, 0);
            });
        }
    }

'''
text = replace_once(text, anchor, method + anchor, "BuildVehicleCustomizationPages implementation")
write(path, text)


# 9) Frontend callback -> backend API.
path = "src/frontend/FrontendCore.cpp"
text = read(path)
anchor = '''        callbacks.putVehicleOnGround = [] { Sick::Backend::BackendApi::Get().PutVehicleOnGround(); };
'''
addition = '''        callbacks.customizeVehicle = [](std::int32_t command, int a, int b, int c, int d) {
            static_cast<void>(Sick::Backend::BackendApi::Get().CustomizeCurrentVehicle(
                static_cast<Sick::Backend::VehicleCustomizationCommand>(command), a, b, c, d));
        };
'''
text = replace_once(text, anchor, anchor + addition, "frontend customization callback")
write(path, text)


# 10) Menu tests: account for the new Customization submenu and exercise it.
path = "tests/menu_tests.cpp"
text = read(path)
text = replace_once(text, '        CHECK(vehicleOptions.size() == 10);\n', '        CHECK(vehicleOptions.size() == 11);\n', "vehicle menu size test")
old = '''        CHECK(vehicleOptions[6].LabelText() == "Handling");
        CHECK(vehicleOptions[7].LabelText() == "Repair Vehicle");
        CHECK(vehicleOptions[8].LabelText() == "Clean Vehicle");
        CHECK(vehicleOptions[9].LabelText() == "Put On Ground");'''
new = '''        CHECK(vehicleOptions[6].LabelText() == "Handling");
        CHECK(vehicleOptions[7].LabelText() == "Customization");
        CHECK(vehicleOptions[8].LabelText() == "Repair Vehicle");
        CHECK(vehicleOptions[9].LabelText() == "Clean Vehicle");
        CHECK(vehicleOptions[10].LabelText() == "Put On Ground");'''
text = replace_once(text, old, new, "vehicle menu labels test")
text = replace_once(text, '        CHECK(menu.Controller().SelectionCounter().total == 10);\n', '        CHECK(menu.Controller().SelectionCounter().total == 11);\n', "vehicle selection count test")
old = '''        CHECK(menu.Controller().SelectOption(7));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(repairCallbacks == 1);'''
new = '''        CHECK(menu.Controller().SelectOption(7));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(menu.Controller().CurrentPage()->Title() == "VEHICLE / CUSTOMIZATION");
        CHECK(menu.Controller().SelectionCounter().total == 6);
        CHECK(menu.Handle(Reaper::UI::MenuInput::Back));
        CHECK(menu.Controller().SelectOption(8));
        CHECK(menu.Handle(Reaper::UI::MenuInput::Select));
        CHECK(repairCallbacks == 1);'''
text = replace_once(text, old, new, "vehicle customization navigation test")
write(path, text)


# Patch sanity checks.
required = {
    "src/ui/menu/Menu.cpp": ["return (*sharedValues)[index];"],
    "src/ui/menu/ImGuiMenuBackend.hpp": ["backAlternate", "ImGuiKey_Insert"],
    "src/backend/BackendCore.cpp": ["CustomizeCurrentVehicle", "0xB5AD06DDA85E2E8F", "MaxVehicle"],
    "src/ui/menu/SickMenu.cpp": ["BuildVehicleCustomizationPages", "Apply Neon Color", "Max Vehicle"],
    "src/frontend/FrontendCore.cpp": ["callbacks.customizeVehicle"],
}
for filename, needles in required.items():
    body = read(filename)
    for needle in needles:
        if needle not in body:
            raise RuntimeError(f"sanity check failed: {needle} missing from {filename}")

print("Sick Menu vehicle customization patch applied successfully")
