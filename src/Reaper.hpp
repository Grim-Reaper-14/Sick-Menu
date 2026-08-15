#pragma once

#include "game/enhanced/BuildManager.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/NativeTable.hpp"
#include "game/natives/NativeBackend.hpp"
#include "game/scheduler/GameScheduler.hpp"

namespace Reaper
{
    using NativeHash = Sick::Game::NativeHash;
    using Hash = Sick::Game::Hash;
    using Entity = Sick::Game::Entity;
    using Ped = Sick::Game::Ped;
    using Vehicle = Sick::Game::Vehicle;
    using Object = Sick::Game::Object;
    using Player = Sick::Game::Player;

    namespace PLAYER = Sick::Game::Natives::PLAYER;
    namespace ENTITY = Sick::Game::Natives::ENTITY;

    namespace Native
    {
        using Context = Sick::Game::Natives::NativeCallContext;
        using CallFrame = Sick::Game::Natives::NativeCallFrame;
        using Handler = Sick::Game::Natives::NativeHandler;
        using Invoker = Sick::Game::Natives::NativeInvoker;
        using Resolver = Sick::Game::Natives::NativeResolver;
        using Registry = Sick::Game::Natives::NativeRegistry;
        using Diagnostics = Sick::Game::Natives::NativeDiagnostics;
        using System = Sick::Game::Natives::NativeSystem;
        namespace Hashes = Sick::Game::Natives::Hashes;
    }

    namespace Enhanced
    {
        using BuildId = Sick::Game::Enhanced::BuildId;
        inline constexpr BuildId UnknownBuild = Sick::Game::Enhanced::UnknownBuild;

        using BuildManager = Sick::Game::Enhanced::BuildManager;
        using NativeTable = Sick::Game::Enhanced::NativeTable;
        using Game = Sick::Game::Enhanced::EnhancedGame;
    }

    using Scheduler = Sick::Game::GameScheduler;
}
