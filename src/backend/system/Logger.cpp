#include "Logger.hpp"

#include "LoggerApi.hpp"

namespace Sick::Backend::System
{
    Logger& Logger::Get() noexcept
    {
        static Logger logger;
        return logger;
    }

    bool Logger::Initialize(Tasking::ThreadPool&, std::filesystem::path path) noexcept
    {
        const auto directory = path.parent_path();
        return LoggerApi::Get().InitializePaths(
            path,
            directory / "SickMenu.jsonl",
            directory.parent_path() / "crashes" / "emergency.log");
    }

    void Logger::Shutdown() noexcept
    {
        LoggerApi::Get().Shutdown();
    }

    void Logger::Write(std::string_view message) noexcept
    {
        LoggerApi::Get().Info("legacy", message);
    }

    void Logger::WriteImmediate(std::string_view message) noexcept
    {
        LoggerApi::Get().Emergency(message);
    }

    bool Logger::Ready() const noexcept
    {
        return LoggerApi::Get().Ready();
    }

    std::size_t Logger::Dropped() const noexcept
    {
        return static_cast<std::size_t>(LoggerApi::Get().Snapshot().dropped);
    }

    std::size_t Logger::Pending() const noexcept
    {
        return LoggerApi::Get().Snapshot().pending;
    }
}
