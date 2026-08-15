#include "RateLimiter.hpp"

namespace Sick::Core::Logging
{
    RateLimiter& RateLimiter::Global() noexcept
    {
        static RateLimiter limiter;
        return limiter;
    }

    bool RateLimiter::Allow(
        std::string_view key,
        std::chrono::milliseconds interval,
        std::size_t burst)
    {
        if (burst == 0)
            return false;

        const auto now = std::chrono::steady_clock::now();
        std::scoped_lock lock(m_Mutex);
        auto& entry = m_Entries[std::string{key}];

        if (!entry.seen || now - entry.windowStart >= interval)
        {
            entry.windowStart = now;
            entry.count = 1;
            entry.seen = true;
            return true;
        }

        if (entry.count < burst)
        {
            ++entry.count;
            return true;
        }

        return false;
    }

    bool RateLimiter::First(std::string_view key)
    {
        std::scoped_lock lock(m_Mutex);
        auto& entry = m_Entries[std::string{key}];
        if (entry.seen)
            return false;

        entry.seen = true;
        entry.windowStart = std::chrono::steady_clock::now();
        entry.count = 1;
        return true;
    }

    void RateLimiter::Reset() noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            m_Entries.clear();
        }
        catch (...)
        {
        }
    }
}
