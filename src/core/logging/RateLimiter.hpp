#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Sick::Core::Logging
{
    class RateLimiter final
    {
    public:
        static RateLimiter& Global() noexcept;

        bool Allow(
            std::string_view key,
            std::chrono::milliseconds interval,
            std::size_t burst = 1);

        bool First(std::string_view key);
        void Reset() noexcept;

    private:
        struct Entry
        {
            std::chrono::steady_clock::time_point windowStart{};
            std::size_t count{};
            bool seen{};
        };

        std::mutex m_Mutex;
        std::unordered_map<std::string, Entry> m_Entries;
    };
}
