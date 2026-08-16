#include "EnhancedGame.hpp"
#include "game/natives/NativeSystem.hpp"
#include "game/scheduler/GameScheduler.hpp"

namespace Sick::Game::Enhanced
{
    bool EnhancedGame::Initialize(
        BuildId build,
        NativeTable::LookupFn lookup,
        NativeTable::HashMapperFn mapper) noexcept
    {
        GameScheduler::Get().Clear();
        Scripts::ScriptRuntime::Get().Reset();
        return Natives::NativeSystem::Initialize(build, lookup, mapper);
    }

    bool EnhancedGame::InitializeIndexed(
        BuildId build,
        NativeBootstrap::ProviderFn provider,
        NativeBootstrap::HashMapperFn mapper) noexcept
    {
        GameScheduler::Get().Clear();
        Scripts::ScriptRuntime::Get().Reset();
        return Natives::NativeSystem::InitializeIndexed(build, provider, mapper);
    }

    void EnhancedGame::BindScriptGlobalResolver(ScriptGlobal::ResolverFn resolver) noexcept
    {
        ScriptGlobal::BindResolver(resolver);
    }

    bool EnhancedGame::BindScriptRuntime(
        Scripts::ScriptRuntime::ProgramResolverFn programResolver,
        Scripts::ScriptRuntime::InvokeFn invoker) noexcept
    {
        return Scripts::ScriptRuntime::Get().Configure(programResolver, invoker);
    }

    bool EnhancedGame::ScriptFunctionsReady() noexcept
    {
        return Scripts::ScriptRuntime::Get().Ready();
    }

    void EnhancedGame::Shutdown() noexcept
    {
        GameScheduler::Get().Clear();
        ScriptGlobal::ResetResolver();
        Scripts::ScriptRuntime::Get().Reset();
        Natives::NativeSystem::Shutdown();
    }

    void EnhancedGame::Tick()
    {
        if (!Ready())
            return;

        GameScheduler::Get().Tick();
    }

    bool EnhancedGame::Ready() noexcept
    {
        return Natives::NativeSystem::Ready();
    }
}
