#pragma once

#include "NativeContext.hpp"

#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace Sick::Game::Natives
{
    class NativeResolver final
    {
    public:
        using ResolverFn = NativeHandler (*)(NativeHash);

        static NativeResolver& Get() noexcept;

        void SetResolver(ResolverFn resolver) noexcept;
        [[nodiscard]] ResolverFn GetResolver() const noexcept;
        [[nodiscard]] bool IsReady() const noexcept;

        [[nodiscard]] NativeHandler Resolve(NativeHash hash) noexcept;

        void RegisterOverride(NativeHash hash, NativeHandler handler) noexcept;
        void RemoveOverride(NativeHash hash) noexcept;

        void ClearCache() noexcept;
        void Reset() noexcept;

        [[nodiscard]] std::size_t CachedHandlerCount() const noexcept;

    private:
        NativeResolver() = default;

        mutable std::mutex m_Mutex;
        ResolverFn m_Resolver{};
        std::unordered_map<NativeHash, NativeHandler> m_Cache;
        std::unordered_map<NativeHash, NativeHandler> m_Overrides;
    };
}
