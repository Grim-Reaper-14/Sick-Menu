#pragma once

#include "backend/tasking/ThreadPool.hpp"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

namespace Sick::Backend::System
{
    class Logger final
    {
    public:
        static Logger& Get() noexcept;

        bool Initialize(Tasking::ThreadPool& pool, std::filesystem::path path) noexcept;
        void Shutdown() noexcept;
        void Write(std::string_view message) noexcept;
        void WriteImmediate(std::string_view message) noexcept;

        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] std::size_t Dropped() const noexcept;
        [[nodiscard]] std::size_t Pending() const noexcept;

    private:
        Logger() = default;

        void ScheduleDrain() noexcept;
        void Drain() noexcept;
        void FlushQueued() noexcept;

        static constexpr std::size_t MaxPending = 1024;
        static constexpr std::size_t BatchSize = 64;
        static constexpr std::size_t MaxBatchesPerDrain = 4;

        mutable std::mutex m_StateMutex;
        mutable std::mutex m_QueueMutex;
        std::mutex m_FileMutex;
        Tasking::ThreadPool* m_Pool{};
        std::filesystem::path m_Path;
        std::ofstream m_File;
        std::queue<std::string> m_Messages;
        std::atomic_bool m_DrainScheduled{};
        std::atomic_size_t m_Dropped{};
        bool m_Ready{};
    };
}
