#pragma once

#include "core/hooking/HookManager.hpp"
#include "core/memory/MemoryManager.hpp"
#include "game/enhanced/BuildManager.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/NativeBootstrap.hpp"
#include "game/enhanced/NativeCrossmap.hpp"
#include "game/enhanced/NativeTable.hpp"
#include "game/enhanced/ScriptGlobal.hpp"
#include "game/natives/NativeBackend.hpp"
#include "game/natives/NativeHandlerTable.hpp"
#include "game/natives/generated/NativeIndex.hpp"
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
    using ScriptGlobal = Sick::Game::Enhanced::ScriptGlobal;

    namespace PLAYER = Sick::Game::Natives::PLAYER;
    namespace ENTITY = Sick::Game::Natives::ENTITY;

    namespace Hooking
    {
        using Detour = Sick::Hooking::DetourHook;
        using Manager = Sick::Hooking::HookManager;
        using Diagnostic = Sick::Hooking::HookDiagnostic;
        using Operation = Sick::Hooking::HookOperation;
    }

    namespace Memory
    {
        using AddressId = Sick::Memory::AddressId;
        using Manager = Sick::Memory::MemoryManager;
        using Module = Sick::Memory::Module;
        using ModuleManager = Sick::Memory::ModuleManager;
        using Pattern = Sick::Memory::Pattern;
        using Scanner = Sick::Memory::PatternScanner;
        using Pointer = Sick::Memory::PointerCalculator;
        using ScanDiagnostic = Sick::Memory::ScanDiagnostic;
        using ScanSummary = Sick::Memory::ScanSummary;
    }

    namespace Native
    {
        using NativeIndex = Sick::Game::Natives::NativeIndex;
        using Context = Sick::Game::Natives::NativeCallContext;
        using Crossmap = Sick::Game::Enhanced::NativeCrossmap;
        using CrossmapEntry = Sick::Game::Enhanced::NativeCrossmapEntry;
        using CallFrame = Sick::Game::Natives::NativeCallFrame;
        using Handler = Sick::Game::Natives::NativeHandler;
        using HandlerTable = Sick::Game::Natives::NativeHandlerTable;
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
        using NativeBootstrap = Sick::Game::Enhanced::NativeBootstrap;
        using NativeCrossmap = Sick::Game::Enhanced::NativeCrossmap;
        using NativeTable = Sick::Game::Enhanced::NativeTable;
        using Game = Sick::Game::Enhanced::EnhancedGame;
    }

    using Scheduler = Sick::Game::GameScheduler;
}
