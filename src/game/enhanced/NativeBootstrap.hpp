#pragma once

#include "BuildManager.hpp"
#include "game/natives/NativeContext.hpp"

#include <cstddef>
#include <mutex>

namespace Sick::Game::Enhanced
{
    class NativeBootstrap final
    {
    public:
        using ProviderFn = Natives::NativeHandler (*)(NativeHash hash, BuildId build);
        using HashMapperFn = NativeHash (*)(NativeHash originalHash, BuildId build);

        static NativeBootstrap& Get() noexcept;

        bool Initialize(ProviderFn provider, HashMapperFn mapper = nullptr) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] std::size_t ResolvedCount() const noexcept;
        [[nodiscard]] std::size_t MissingCount() const noexcept;
        [[nodiscard]] Natives::NativeHandler Resolve(NativeHash hash) const noexcept;

    private:
        NativeBootstrap() = default;

        mutable std::mutex m_Mutex;
        bool m_Ready{};
        std::size_t m_ResolvedCount{};
    };
}
