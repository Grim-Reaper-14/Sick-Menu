#pragma once

#include <array>
#include <string_view>

namespace Sick::Ui::OnlineVehicleSpawner
{
    struct VehicleCatalogEntry
    {
        std::string_view category;
        std::string_view label;
        std::string_view model;
    };

    inline constexpr std::array<std::string_view, 7> Categories{
        "Super",
        "Sports",
        "Muscle",
        "Off-Road",
        "SUVs",
        "Sedans",
        "Motorcycles",
    };

    inline constexpr std::array<VehicleCatalogEntry, 51> Vehicles{{
        {"Super", "Adder", "adder"},
        {"Super", "Autarch", "autarch"},
        {"Super", "Entity XF", "entityxf"},
        {"Super", "Krieger", "krieger"},
        {"Super", "Osiris", "osiris"},
        {"Super", "T20", "t20"},
        {"Super", "Turismo R", "turismor"},
        {"Super", "Zentorno", "zentorno"},

        {"Sports", "Comet Retro Custom", "comet3"},
        {"Sports", "Elegy RH8", "elegy2"},
        {"Sports", "Feltzer", "feltzer2"},
        {"Sports", "Jester", "jester"},
        {"Sports", "Kuruma", "kuruma"},
        {"Sports", "Massacro", "massacro"},
        {"Sports", "Pariah", "pariah"},
        {"Sports", "Sultan", "sultan"},

        {"Muscle", "Buffalo STX", "buffalo4"},
        {"Muscle", "Dominator", "dominator"},
        {"Muscle", "Gauntlet", "gauntlet"},
        {"Muscle", "Gauntlet Hellfire", "gauntlet4"},
        {"Muscle", "Phoenix", "phoenix"},
        {"Muscle", "Sabre Turbo", "sabregt"},
        {"Muscle", "Vigero", "vigero"},

        {"Off-Road", "Bifta", "bifta"},
        {"Off-Road", "Dubsta 6x6", "dubsta3"},
        {"Off-Road", "Insurgent", "insurgent"},
        {"Off-Road", "Kamacho", "kamacho"},
        {"Off-Road", "Mesa", "mesa"},
        {"Off-Road", "Sandking XL", "sandking"},
        {"Off-Road", "Trophy Truck", "trophytruck"},

        {"SUVs", "Baller", "baller"},
        {"SUVs", "Cavalcade", "cavalcade"},
        {"SUVs", "Granger", "granger"},
        {"SUVs", "Huntley S", "huntley"},
        {"SUVs", "Rebla GTS", "rebla"},
        {"SUVs", "Toros", "toros"},
        {"SUVs", "XLS", "xls"},

        {"Sedans", "Fugitive", "fugitive"},
        {"Sedans", "Premier", "premier"},
        {"Sedans", "Schafter", "schafter2"},
        {"Sedans", "Stanier", "stanier"},
        {"Sedans", "Super Diamond", "superd"},
        {"Sedans", "Tailgater", "tailgater"},
        {"Sedans", "Tailgater S", "tailgater2"},

        {"Motorcycles", "Akuma", "akuma"},
        {"Motorcycles", "Bati 801", "bati"},
        {"Motorcycles", "Hakuchou Drag", "hakuchou2"},
        {"Motorcycles", "Oppressor", "oppressor"},
        {"Motorcycles", "Oppressor Mk II", "oppressor2"},
        {"Motorcycles", "Sanchez", "sanchez"},
        {"Motorcycles", "Shotaro", "shotaro"},
    }};
}
