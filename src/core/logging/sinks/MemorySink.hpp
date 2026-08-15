#pragma once

#include "Sink.hpp"

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace Sick::Core::Logging
{
    class MemorySink final : public Sink
    {
    public:
        explicit MemorySink(std::size_t capacity = 1024) : m_Capacity(capacity) {}

        void Write(const LogRecord& record) noexcept override;
        void Flush() noexcept override {}

        [[nodiscard]] std::vector<LogRecord> Snapshot(std::size_t maxRecords = 0) const;
        void Clear() noexcept;

    private:
        std::size_t m_Capacity{};
        mutable std::mutex m_Mutex;
        std::deque<LogRecord> m_Records;
    };
}
