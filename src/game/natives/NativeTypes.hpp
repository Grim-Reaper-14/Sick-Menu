#pragma once

#include <cstdint>

namespace Sick::Game
{
    using NativeHash = std::uint64_t;
    using Hash = std::uint32_t;

    using Entity = std::int32_t;
    using Ped = std::int32_t;
    using Vehicle = std::int32_t;
    using Object = std::int32_t;
    using Player = std::int32_t;
}

namespace Sick::Game::Natives
{
    using NativeHash = Sick::Game::NativeHash;
}
