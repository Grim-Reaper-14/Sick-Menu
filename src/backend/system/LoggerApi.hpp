#pragma once

#include "backend/BackendTypes.hpp"
#include "backend/system/logging/EmergencyLogWriter.hpp"
#include "backend/system/logging/LoggingEngine.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <source_location>
#include <string_view>
#include <vector>

namespace Sick::Backend::System
{
    class FileSystem;

    class LoggerApi final
    {
    public:
        using LogLevel = Logging::LogLevel;
        using LogField = Logging::LogField;
        using Fields = std::vector<LogField>;

        static LoggerApi& Get() noexcept;

        bool Initialize(const FileSystem& files) noexcept;
        bool InitializePaths(
            const std::filesystem::path& textPath,
            const std::filesystem::path& structuredPath,
            const std::filesystem::path& emergencyPath) noexcept;
        void Shutdown() noexcept;
        void Flush() noexcept;

        void Write(
            LogLevel level,
            std::string_view channel,
            std::string_view message,
            Fields fields = {},
            const std::source_location& source = std::source_location::current()) noexcept;

        void Trace(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;
        void Debug(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;
        void Info(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;
        void Warn(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;
        void Error(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;
        void Critical(std::string_view channel, std::string_view message,
            Fields fields = {}, const std::source_location& source = std::source_location::current()) noexcept;

        void Emergency(std::string_view message) noexcept;
        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] LoggerSnapshot Snapshot() const noexcept;
        [[nodiscard]] std::vector<Logging::LogRecord> Recent(std::size_t maximum = 100) const;

    private:
        LoggerApi() = default;

        Logging::LoggingEngine m_Engine;
        Logging::EmergencyLogWriter m_Emergency;
        std::atomic_uint64_t m_Sequence{1};
        std::chrono::steady_clock::time_point m_Start{std::chrono::steady_clock::now()};
    };
}
