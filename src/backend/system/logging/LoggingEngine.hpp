#pragma once

#include "backend/BackendTypes.hpp"
#include "LogTypes.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

namespace Sick::Backend::System::Logging
{
    class LoggingEngine final
    {
    public:
        bool Initialize(
            std::filesystem::path textPath,
            std::filesystem::path structuredPath,
            std::size_t maxPending = 4096,
            std::uintmax_t maxFileBytes = 4 * 1024 * 1024,
            std::size_t retainedFiles = 5) noexcept;
        void Shutdown() noexcept;
        [[nodiscard]] bool Enqueue(LogRecord record) noexcept;
        void Flush() noexcept;
        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] LoggerSnapshot Snapshot() const noexcept;
        [[nodiscard]] std::vector<LogRecord> Recent(std::size_t maximum) const;

    private:
        static constexpr std::size_t BatchSize = 128;
        static constexpr std::size_t RecentCapacity = 512;

        void Worker() noexcept;
        void WriteBatch(std::vector<LogRecord>& batch) noexcept;
        void RotateIfNeeded(std::uintmax_t incomingBytes) noexcept;
        void RotateOne(const std::filesystem::path& path) noexcept;
        [[nodiscard]] bool OpenSinks(bool truncate = false) noexcept;
        void RecordPeak(std::size_t pending) noexcept;

        mutable std::mutex m_QueueMutex;
        mutable std::mutex m_RecentMutex;
        std::condition_variable m_Condition;
        std::condition_variable m_FlushCondition;
        std::deque<LogRecord> m_Queue;
        std::deque<LogRecord> m_Recent;
        std::thread m_Worker;
        std::filesystem::path m_TextPath;
        std::filesystem::path m_StructuredPath;
        std::ofstream m_TextFile;
        std::ofstream m_StructuredFile;
        std::size_t m_MaxPending{4096};
        std::uintmax_t m_MaxFileBytes{4 * 1024 * 1024};
        std::size_t m_RetainedFiles{5};
        std::uintmax_t m_TextBytes{};
        std::uintmax_t m_StructuredBytes{};
        bool m_Stopping{true};
        bool m_Writing{};

        std::atomic_bool m_Ready{};
        std::atomic_uint64_t m_Accepted{};
        std::atomic_uint64_t m_Written{};
        std::atomic_uint64_t m_Dropped{};
        std::atomic_uint64_t m_SinkFailures{};
        std::atomic_uint64_t m_Rotations{};
        std::atomic_uint64_t m_LastDrainMicros{};
        std::atomic_size_t m_PeakPending{};
    };
}
