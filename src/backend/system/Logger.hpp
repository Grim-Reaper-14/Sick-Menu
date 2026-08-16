#pragma once

#include "backend/tasking/ThreadPool.hpp"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <mutex>
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

    private:
        Logger() = default;

        mutable std::mutex m_StateMutex;
        std::mutex m_FileMutex;
        Tasking::ThreadPool* m_Pool{};
        std::filesystem::path m_Path;
        std::atomic_size_t m_Dropped{};
        bool m_Ready{};
    };
}
