#include "NativeSystem.hpp"
#include "NativeResolver.hpp"

namespace
{
    Sick::Game::Natives::NativeHandler ResolveEnhancedNative(Sick::Game::NativeHash hash)
    {
        return Sick::Game::Enhanced::NativeTable::Get().Resolve(hash);
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

    void NativeSystem::Shutdown() noexcept
    {
        NativeResolver::Get().Reset();
        NativeRegistry::Get().Clear();
        NativeDiagnostics::Reset();
        Enhanced::NativeTable::Get().Reset();
        Enhanced::BuildManager::SetBuild(Enhanced::UnknownBuild);
    }

    bool NativeSystem::Ready() noexcept
    {
        return Enhanced::NativeTable::Get().Ready() && NativeResolver::Get().IsReady();
    }

    NativeStats NativeSystem::Stats() noexcept
    {
        return NativeDiagnostics::Snapshot();
    }
}
