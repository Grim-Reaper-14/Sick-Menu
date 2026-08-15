#pragma once

#include <atomic>
#include <cstdint>

namespace Sick::Game::Natives
{
    struct NativeStats
    {
        std::uint64_t calls{};
        std::uint64_t succeeded{};
        std::uint64_t failed{};
    };

    class NativeDiagnostics final
    {
    public:
        static void RecordCall(bool success) noexcept
        {
            s_Calls.fetch_add(1, std::memory_order_relaxed);

            if (success)
                s_Succeeded.fetch_add(1, std::memory_order_relaxed);
            else
                s_Failed.fetch_add(1, std::memory_order_relaxed);
        }

        [[nodiscard]] static NativeStats Snapshot() noexcept
        {
            return {
                s_Calls.load(std::memory_order_relaxed),
                s_Succeeded.load(std::memory_order_relaxed),
                s_Failed.load(std::memory_order_relaxed)
            };
        }

        static void Reset() noexcept
        {
            s_Calls.store(0, std::memory_order_relaxed);
            s_Succeeded.store(0, std::memory_order_relaxed);
            s_Failed.store(0, std::memory_order_relaxed);
        }

    private:
        static inline std::atomic_uint64_t s_Calls{};
        static inline std::atomic_uint64_t s_Succeeded{};
        static inline std::atomic_uint64_t s_Failed{};
    };
}
