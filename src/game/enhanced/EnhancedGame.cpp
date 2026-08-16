#include "EnhancedGame.hpp"
#include "game/natives/NativeSystem.hpp"
#include "game/scheduler/GameScheduler.hpp"
#include "game/services/PlayerService.hpp"

namespace Sick::Game::Enhanced
{
    bool EnhancedGame::Initialize(
        BuildId build,
        NativeTable::LookupFn lookup,
        NativeTable::HashMapperFn mapper) noexcept
    {
        GameScheduler::Get().Clear();
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
        GameScheduler::Get().Clear();
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
        GameScheduler::Get().Clear();
        PlayerService{}.Reset();
        ScriptGlobal::ResetResolver();
        EnhancedScriptHost::Shutdown();
        Natives::NativeSystem::Shutdown();
    }

    void EnhancedGame::Tick()
    {
        // Script-function callbacks and native callbacks share this queue. The
        // injected host owns the game-thread tick even while either backend is
        // still coming online, and each backend independently fails closed.
        GameScheduler::Get().Tick();
        if (Natives::NativeSystem::Ready())
            PlayerService{}.Tick();
    }

    bool EnhancedGame::Ready() noexcept
    {
        return Natives::NativeSystem::Ready();
    }
}
