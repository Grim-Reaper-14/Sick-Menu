#pragma once

#include "backend/BackendTypes.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Sick::Backend::System
{
    enum class FileArea : std::uint8_t
    {
        Root,
        Logs,
        Configs,
        Themes,
        Scripts,
        Cache,
        Temp,
        Crashes,
    };

    struct FileEntry
    {
        std::filesystem::path relativePath;
        std::uintmax_t size{};
        bool directory{};
    };

    class FileSystem final
    {
    public:
        bool Initialize(const std::filesystem::path& moduleDirectory) noexcept;

        [[nodiscard]] std::optional<std::filesystem::path> Resolve(
            FileArea area,
            const std::filesystem::path& relative = {}) const noexcept;
        [[nodiscard]] std::optional<std::string> ReadText(
            FileArea area,
            const std::filesystem::path& relative) noexcept;
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadBinary(
            FileArea area,
            const std::filesystem::path& relative) noexcept;
        [[nodiscard]] bool AtomicWriteText(
            FileArea area,
            const std::filesystem::path& relative,
            std::string_view contents) noexcept;
        [[nodiscard]] bool AtomicWriteBinary(
            FileArea area,
            const std::filesystem::path& relative,
            std::span<const std::uint8_t> contents) noexcept;
        [[nodiscard]] bool Exists(
            FileArea area,
            const std::filesystem::path& relative) noexcept;
        [[nodiscard]] std::vector<FileEntry> List(
            FileArea area,
            const std::filesystem::path& relative = {}) noexcept;
        [[nodiscard]] bool Remove(
            FileArea area,
            const std::filesystem::path& relative) noexcept;

        [[nodiscard]] FileSystemSnapshot Snapshot() const noexcept;

        [[nodiscard]] const std::filesystem::path& Root() const noexcept { return m_Root; }
        [[nodiscard]] const std::filesystem::path& Logs() const noexcept { return m_Logs; }
        [[nodiscard]] const std::filesystem::path& Configs() const noexcept { return m_Configs; }
        [[nodiscard]] const std::filesystem::path& Themes() const noexcept { return m_Themes; }
        [[nodiscard]] const std::filesystem::path& Scripts() const noexcept { return m_Scripts; }
        [[nodiscard]] const std::filesystem::path& Cache() const noexcept { return m_Cache; }
        [[nodiscard]] const std::filesystem::path& Temp() const noexcept { return m_Temp; }
        [[nodiscard]] const std::filesystem::path& Crashes() const noexcept { return m_Crashes; }
        [[nodiscard]] const std::filesystem::path& LogFile() const noexcept { return m_LogFile; }
        [[nodiscard]] const std::filesystem::path& StructuredLogFile() const noexcept { return m_StructuredLogFile; }
        [[nodiscard]] const std::filesystem::path& EmergencyLogFile() const noexcept { return m_EmergencyLogFile; }
        [[nodiscard]] const std::filesystem::path& SettingsFile() const noexcept { return m_SettingsFile; }

    private:
        [[nodiscard]] const std::filesystem::path& AreaPath(FileArea area) const noexcept;
        [[nodiscard]] bool IoAllowed() const noexcept;
        [[nodiscard]] bool ValidRelativePath(const std::filesystem::path& relative) const noexcept;
        [[nodiscard]] bool CommitAtomic(
            const std::filesystem::path& temporary,
            const std::filesystem::path& destination) noexcept;

        std::filesystem::path m_Root;
        std::filesystem::path m_Logs;
        std::filesystem::path m_Configs;
        std::filesystem::path m_Themes;
        std::filesystem::path m_Scripts;
        std::filesystem::path m_Cache;
        std::filesystem::path m_Temp;
        std::filesystem::path m_Crashes;
        std::filesystem::path m_LogFile;
        std::filesystem::path m_StructuredLogFile;
        std::filesystem::path m_EmergencyLogFile;
        std::filesystem::path m_SettingsFile;

        mutable std::atomic_uint64_t m_Reads{};
        mutable std::atomic_uint64_t m_Writes{};
        mutable std::atomic_uint64_t m_BytesRead{};
        mutable std::atomic_uint64_t m_BytesWritten{};
        mutable std::atomic_uint64_t m_Failures{};
        mutable std::atomic_uint64_t m_RejectedPaths{};
        mutable std::atomic_uint64_t m_RejectedAffinity{};
        std::atomic_uint64_t m_TemporarySequence{};
    };
}
