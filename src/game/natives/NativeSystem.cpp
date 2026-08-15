#include "NativeSystem.hpp"
#include "NativeResolver.hpp"

namespace
{
    Sick::Game::Natives::NativeHandler ResolveEnhancedNative(Sick::Game::NativeHash hash)
    {
        return Sick::Game::Enhanced::NativeTable::Get().Resolve(hash);
    }

    Sick::Game::Natives::NativeHandler ResolveIndexedNative(Sick::Game::NativeHash hash)
    {
        return Sick::Game::Enhanced::NativeBootstrap::Get().Resolve(hash);
    }
}

namespace Sick::Game::Natives
{
    bool NativeSystem::Initialize(
        Enhanced::BuildId build,
        Enhanced::NativeTable::LookupFn lookup,
        Enhanced::NativeTable::HashMapperFn mapper) noexcept
    {
        Shutdown();

        Enhanced::BuildManager::SetBuild(build);
        Enhanced::NativeTable::Get().Configure(lookup, mapper);

        if (!Enhanced::NativeTable::Get().Ready())
        {
            Shutdown();
            return false;
        }

        NativeRegistry::Get().RegisterDefaults();
        NativeDiagnostics::Reset();
        NativeResolver::Get().SetResolver(&ResolveEnhancedNative);
        return true;
    }

    bool NativeSystem::InitializeIndexed(
        Enhanced::BuildId build,
        Enhanced::NativeBootstrap::ProviderFn provider,
        Enhanced::NativeBootstrap::HashMapperFn mapper) noexcept
    {
        Shutdown();

        Enhanced::BuildManager::SetBuild(build);
        NativeRegistry::Get().RegisterDefaults();
        NativeDiagnostics::Reset();

        if (!Enhanced::NativeBootstrap::Get().Initialize(provider, mapper))
        {
            Shutdown();
            return false;
        }

        NativeResolver::Get().SetResolver(&ResolveIndexedNative);
        return true;
    }

    void NativeSystem::Shutdown() noexcept
    {
        NativeResolver::Get().Reset();
        NativeRegistry::Get().Clear();
        NativeDiagnostics::Reset();
        Enhanced::NativeBootstrap::Get().Shutdown();
        Enhanced::NativeTable::Get().Reset();
        Enhanced::BuildManager::SetBuild(Enhanced::UnknownBuild);
    }

    bool NativeSystem::Ready() noexcept
    {
        const bool backendReady =
            Enhanced::NativeBootstrap::Get().Ready() || Enhanced::NativeTable::Get().Ready();

        return backendReady && NativeResolver::Get().IsReady();
    }

    NativeStats NativeSystem::Stats() noexcept
    {
        return NativeDiagnostics::Snapshot();
    }
}
