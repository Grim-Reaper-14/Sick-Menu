#pragma once

#include "backend/tasking/ThreadPool.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>

namespace Sick::Backend::System
{
    // Compatibility facade. New code should use LoggerApi directly.
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
    };
}
