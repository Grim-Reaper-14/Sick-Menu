#pragma once

#include "game/natives/NativeContext.hpp"
#include "BuildManager.hpp"

#include <mutex>

namespace Sick::Game::Enhanced
{
    class NativeTable final
    {
    public:
        using LookupFn = Natives::NativeHandler (*)(NativeHash hash, BuildId build);
        using HashMapperFn = NativeHash (*)(NativeHash originalHash, BuildId build);

        static NativeTable& Get() noexcept;

        void Configure(LookupFn lookup, HashMapperFn mapper = nullptr) noexcept;
        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] Natives::NativeHandler Resolve(NativeHash hash) const noexcept;
        void Reset() noexcept;

    private:
        NativeTable() = default;

        mutable std::mutex m_Mutex;
        LookupFn m_Lookup{};
        HashMapperFn m_Mapper{};
    };
}
