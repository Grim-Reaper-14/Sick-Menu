#pragma once

#include "NativeTypes.hpp"

#include <cstdint>
#include <string>

namespace Sick::Game::Natives
{
    struct NativeMetadata
    {
        NativeHash hash{};
        std::string name;
        std::string nameSpace;
        std::uint8_t argumentCount{};
        bool vectorFixup{};
    };
}
