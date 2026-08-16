#include "EnhancedGame.hpp"
#include "backend/BackendCore.hpp"
#include "game/natives/NativeSystem.hpp"

namespace Sick::Game::Enhanced
{
    bool EnhancedGame::Initialize(
        BuildId build,
        NativeTable::LookupFn lookup,
        NativeTable::HashMapperFn mapper) noexcept
    {
        // Do not clear backend commands here. Native initialization can happen
        // after the frontend has already submitted desired feature state.
        EnhancedScriptHost::Shutdown();
        const auto initialized = Natives::NativeSystem::Initialize(build, lookup, mapper);
        if (initialized)
            static_cast<void>(EnhancedScriptHost::Initialize());
        return initialized;
    }

    bool EnhancedGame::InitializeIndexed(
        BuildId build,
        NativeBootstrap::ProviderFn provider,
        NativeBootstrap::HashMapperFn mapper) noexcept
    {
        // Deferred backend work survives late/repeated native initialization.
        EnhancedScriptHost::Shutdown();
        const auto initialized = Natives::NativeSystem::InitializeIndexed(build, provider, mapper);
        if (initialized)
            static_cast<void>(EnhancedScriptHost::Initialize());
        return initialized;
    }

    void EnhancedGame::BindScriptGlobalResolver(ScriptGlobal::ResolverFn resolver) noexcept
    {
        ScriptGlobal::BindResolver(resolver);
    }

    bool EnhancedGame::InitializeScriptHost() noexcept
    {
        return EnhancedScriptHost::Initialize();
    }

    bool EnhancedGame::BindScriptHost(EnhancedScriptHost::Bindings bindings) noexcept
    {
        return EnhancedScriptHost::Bind(bindings);
    }

    bool EnhancedGame::BindScriptRuntime(
        Scripts::ScriptRuntime::ProgramResolverFn programResolver,
        Scripts::ScriptRuntime::InvokeFn invoker) noexcept
    {
        EnhancedScriptHost::Shutdown();
        return Scripts::ScriptRuntime::Get().Configure(programResolver, invoker);
    }

    bool EnhancedGame::ScriptFunctionsReady() noexcept
    {
        return Scripts::ScriptRuntime::Get().Ready();
    }

    void EnhancedGame::Shutdown() noexcept
    {
        Backend::BackendCore::Get().ResetGameState();
        ScriptGlobal::ResetResolver();
        EnhancedScriptHost::Shutdown();
        Natives::NativeSystem::Shutdown();
    }

    void EnhancedGame::Tick()
    {
        Backend::BackendCore::Get().TickGame();
    }

    bool EnhancedGame::Ready() noexcept
    {
        return Natives::NativeSystem::Ready();
    }
}
