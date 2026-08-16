#pragma once

#include "game/enhanced/BuildManager.hpp"
#include "game/enhanced/EnhancedGame.hpp"
#include "game/enhanced/EnhancedScriptHost.hpp"
#include "game/enhanced/NativeBootstrap.hpp"
#include "game/enhanced/NativeTable.hpp"
#include "game/enhanced/ScriptGlobal.hpp"
#include "game/natives/NativeBackend.hpp"
#include "game/natives/NativeHandlerTable.hpp"
#include "game/natives/generated/NativeIndex.hpp"
#include "game/scheduler/GameScheduler.hpp"
#include "game/scripts/ScriptFunction.hpp"
#include "game/scripts/ScriptFunctionCatalog.hpp"
#include "game/scripts/ScriptPointer.hpp"
#include "game/scripts/ScriptRuntime.hpp"
#include "game/scripts/ScriptTypes.hpp"
#include "ui/menu/Menu.hpp"
#include "ui/menu/MenuRenderer.hpp"
#include "ui/menu/SickMenu.hpp"

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
    using ScriptHash = Sick::Game::Scripts::ScriptHash;
    using ScriptFunction = Sick::Game::Scripts::ScriptFunction;
    using ScriptFunctionSpec = Sick::Game::Scripts::ScriptFunctionSpec;
    using KnownScriptFunction = Sick::Game::Scripts::KnownScriptFunction;
    using ScriptPattern = Sick::Game::Scripts::ScriptPattern;
    using ScriptPointer = Sick::Game::Scripts::ScriptPointer;
    using ScriptProgramView = Sick::Game::Scripts::ScriptProgramView;
    using ScriptVector3 = Sick::Game::Scripts::ScriptVector3;

    namespace ScriptFunctions = Sick::Game::Scripts::ScriptFunctionCatalog;

    [[nodiscard]] constexpr ScriptHash Joaat(std::string_view value) noexcept
    {
        return Sick::Game::Scripts::Joaat(value);
    }

    namespace PLAYER = Sick::Game::Natives::PLAYER;
    namespace ENTITY = Sick::Game::Natives::ENTITY;

    namespace Native
    {
        using NativeIndex = Sick::Game::Natives::NativeIndex;
        using Context = Sick::Game::Natives::NativeCallContext;
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
        using NativeTable = Sick::Game::Enhanced::NativeTable;
        using Game = Sick::Game::Enhanced::EnhancedGame;
        using ScriptHost = Sick::Game::Enhanced::EnhancedScriptHost;
        using ScriptHostError = Sick::Game::Enhanced::EnhancedScriptHostError;
        using ScriptRuntime = Sick::Game::Scripts::ScriptRuntime;
    }

    using Scheduler = Sick::Game::GameScheduler;

    namespace UI
    {
        using MenuInput = Sick::Ui::MenuInput;
        using MenuOption = Sick::Ui::MenuOption;
        using MenuPage = Sick::Ui::MenuPage;
        using MenuController = Sick::Ui::MenuController;
        using MenuRenderer = Sick::Ui::MenuRenderer;
        using MenuStyle = Sick::Ui::MenuStyle;
        using MenuViewport = Sick::Ui::MenuViewport;
        using MenuDrawList = Sick::Ui::MenuDrawList;
        using MenuTexture = Sick::Ui::MenuTexture;
        using SickMenu = Sick::Ui::SickMenu;
        using SickMenuState = Sick::Ui::SickMenuState;
        using SickMenuCallbacks = Sick::Ui::SickMenuCallbacks;
    }
}
