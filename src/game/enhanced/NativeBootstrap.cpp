#include "NativeBootstrap.hpp"
#include "core/diagnostics/Metrics.hpp"
#include "core/logging/Logger.hpp"
#include "game/natives/NativeHandlerTable.hpp"
#include "game/natives/generated/NativeHashes.hpp"

namespace Sick::Game::Enhanced
{
    NativeBootstrap& NativeBootstrap::Get() noexcept
    {
        static NativeBootstrap bootstrap;
        return bootstrap;
    }

    bool NativeBootstrap::Initialize(ProviderFn provider, HashMapperFn mapper) noexcept
    {
        Shutdown();

        const auto build = BuildManager::Current();
        if (!provider || build == UnknownBuild)
        {
            try
            {
                Core::Logging::Logger::Get().Submit(
                    Core::Logging::Level::Error,
                    "NativeBootstrap",
                    "Native bootstrap rejected an invalid provider or Enhanced build",
                    Core::Logging::EventId::NativeBootstrapPartial,
                    {
                        Core::Logging::MakeField("provider", provider != nullptr),
                        Core::Logging::MakeField("build", build)
                    });
            }
            catch (...)
            {
            }
            return false;
        }

        auto& handlers = Natives::NativeHandlerTable::Get();
        std::size_t resolved{};

        for (std::size_t i = 0; i < Natives::NativeCount; ++i)
        {
            const auto index = static_cast<Natives::NativeIndex>(i);
            const auto originalHash = Natives::Generated::HashFor(index);
            const auto mappedHash = mapper ? mapper(originalHash, build) : originalHash;
            const auto handler = provider(mappedHash, build);

            handlers.Set(index, handler);
            if (handler)
                ++resolved;
        }

        {
            std::scoped_lock lock(m_Mutex);
            m_ResolvedCount = resolved;
            m_Ready = resolved != 0;
        }

        const auto missing = resolved < Natives::NativeCount ? Natives::NativeCount - resolved : 0;
        Core::Metrics::SetGauge("native.bootstrap.resolved", static_cast<double>(resolved));
        Core::Metrics::SetGauge("native.bootstrap.missing", static_cast<double>(missing));

        try
        {
            Core::Logging::Logger::Get().Submit(
                missing == 0 ? Core::Logging::Level::Info : Core::Logging::Level::Warn,
                "NativeBootstrap",
                missing == 0
                    ? "Enhanced native handler table fully populated"
                    : "Enhanced native handler table is only partially populated",
                missing == 0
                    ? Core::Logging::EventId::NativeBootstrapCompleted
                    : Core::Logging::EventId::NativeBootstrapPartial,
                {
                    Core::Logging::MakeField("build", build),
                    Core::Logging::MakeField("resolved", resolved),
                    Core::Logging::MakeField("missing", missing),
                    Core::Logging::MakeField("total", Natives::NativeCount)
                });
        }
        catch (...)
        {
        }

        return resolved != 0;
    }

    void NativeBootstrap::Shutdown() noexcept
    {
        Natives::NativeHandlerTable::Get().Clear();

        std::scoped_lock lock(m_Mutex);
        m_Ready = false;
        m_ResolvedCount = 0;
    }

    bool NativeBootstrap::Ready() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Ready;
    }

    std::size_t NativeBootstrap::ResolvedCount() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_ResolvedCount;
    }

    std::size_t NativeBootstrap::MissingCount() const noexcept
    {
        const auto resolved = ResolvedCount();
        return resolved < Natives::NativeCount ? Natives::NativeCount - resolved : 0;
    }

    Natives::NativeHandler NativeBootstrap::Resolve(NativeHash hash) const noexcept
    {
        const auto index = Natives::Generated::IndexForHash(hash);
        if (index == Natives::NativeIndex::Count)
            return nullptr;

        return Natives::NativeHandlerTable::Get().Get(index);
    }
}
