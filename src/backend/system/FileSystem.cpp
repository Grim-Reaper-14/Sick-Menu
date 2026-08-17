#include "FileSystem.hpp"

#include "backend/tasking/TaskAffinity.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System
{
    bool FileSystem::Initialize(const std::filesystem::path& moduleDirectory) noexcept
    {
        std::error_code error;
        auto base = moduleDirectory;
        if (base.empty())
            base = std::filesystem::current_path(error);
        if (error || base.empty())
            return false;

        m_Root = base / "SickMenu";
        m_Logs = m_Root / "logs";
        m_Configs = m_Root / "configs";
        m_Themes = m_Root / "themes";
        m_Images = m_Root / "images";
        m_Fonts = m_Root / "fonts";
        m_Scripts = m_Root / "scripts";
        m_Cache = m_Root / "cache";
        m_Temp = m_Root / "temp";
        m_Crashes = m_Root / "crashes";
        m_LogFile = m_Logs / "SickMenu.log";
        m_StructuredLogFile = m_Logs / "SickMenu.jsonl";
        m_EmergencyLogFile = m_Crashes / "emergency.log";
        m_SettingsFile = m_Root / "settings.cfg";

        const std::array directories{
            m_Root, m_Logs, m_Configs, m_Themes, m_Images, m_Fonts,
            m_Scripts, m_Cache, m_Temp, m_Crashes};
        for (const auto& directory : directories)
        {
            error.clear();
            std::filesystem::create_directories(directory, error);
            if (error)
                return false;
        }
        return true;
    }

    std::optional<std::filesystem::path> FileSystem::Resolve(
        FileArea area,
        const std::filesystem::path& relative) const noexcept
    {
        if (!ValidRelativePath(relative))
        {
            m_RejectedPaths.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        return (AreaPath(area) / relative).lexically_normal();
    }

    std::optional<std::string> FileSystem::ReadText(FileArea area, const std::filesystem::path& relative) noexcept
    {
        if (!IoAllowed())
            return std::nullopt;
        const auto path = Resolve(area, relative);
        if (!path)
            return std::nullopt;
        try
        {
            std::ifstream input(*path, std::ios::binary);
            if (!input)
            {
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }
            std::string contents{
                std::istreambuf_iterator<char>{input},
                std::istreambuf_iterator<char>{}};
            if (!input.good() && !input.eof())
            {
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }
            m_Reads.fetch_add(1, std::memory_order_relaxed);
            m_BytesRead.fetch_add(contents.size(), std::memory_order_relaxed);
            return contents;
        }
        catch (...)
        {
            m_Failures.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
    }

    std::optional<std::vector<std::uint8_t>> FileSystem::ReadBinary(FileArea area, const std::filesystem::path& relative) noexcept
    {
        if (!IoAllowed())
            return std::nullopt;
        const auto path = Resolve(area, relative);
        if (!path)
            return std::nullopt;
        try
        {
            std::ifstream input(*path, std::ios::binary | std::ios::ate);
            if (!input)
            {
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }
            const auto end = input.tellg();
            if (end < 0)
                return std::nullopt;
            std::vector<std::uint8_t> contents(static_cast<std::size_t>(end));
            input.seekg(0, std::ios::beg);
            if (!contents.empty())
                input.read(reinterpret_cast<char*>(contents.data()), static_cast<std::streamsize>(contents.size()));
            if (!input)
            {
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }
            m_Reads.fetch_add(1, std::memory_order_relaxed);
            m_BytesRead.fetch_add(contents.size(), std::memory_order_relaxed);
            return contents;
        }
        catch (...)
        {
            m_Failures.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
    }

    bool FileSystem::AtomicWriteText(
        FileArea area,
        const std::filesystem::path& relative,
        std::string_view contents) noexcept
    {
        if (!IoAllowed())
            return false;
        const auto destination = Resolve(area, relative);
        if (!destination)
            return false;
        try
        {
            std::error_code error;
            std::filesystem::create_directories(destination->parent_path(), error);
            if (error)
            {
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return false;
            }

            auto temporary = *destination;
            temporary += ".tmp." + std::to_string(m_TemporarySequence.fetch_add(1, std::memory_order_relaxed));
            {
                std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
                if (!output)
                {
                    m_Failures.fetch_add(1, std::memory_order_relaxed);
                    return false;
                }
                output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
                output.flush();
                if (!output)
                {
                    output.close();
                    std::filesystem::remove(temporary, error);
                    m_Failures.fetch_add(1, std::memory_order_relaxed);
                    return false;
                }
            }

            if (!CommitAtomic(temporary, *destination))
            {
                std::filesystem::remove(temporary, error);
                m_Failures.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            m_Writes.fetch_add(1, std::memory_order_relaxed);
            m_BytesWritten.fetch_add(contents.size(), std::memory_order_relaxed);
            return true;
        }
        catch (...)
        {
            m_Failures.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
    }

    bool FileSystem::AtomicWriteBinary(
        FileArea area,
        const std::filesystem::path& relative,
        std::span<const std::uint8_t> contents) noexcept
    {
        if (contents.empty())
            return AtomicWriteText(area, relative, {});
        return AtomicWriteText(area, relative, std::string_view(
            reinterpret_cast<const char*>(contents.data()), contents.size()));
    }

    bool FileSystem::Exists(FileArea area, const std::filesystem::path& relative) noexcept
    {
        if (!IoAllowed())
            return false;
        const auto path = Resolve(area, relative);
        if (!path)
            return false;
        std::error_code error;
        const bool exists = std::filesystem::exists(*path, error);
        if (error)
            m_Failures.fetch_add(1, std::memory_order_relaxed);
        return exists && !error;
    }

    std::vector<FileEntry> FileSystem::List(FileArea area, const std::filesystem::path& relative) noexcept
    {
        std::vector<FileEntry> entries;
        if (!IoAllowed())
            return entries;
        const auto directory = Resolve(area, relative);
        if (!directory)
            return entries;
        std::error_code error;
        for (std::filesystem::directory_iterator it(*directory, error), end; !error && it != end; it.increment(error))
        {
            FileEntry entry{};
            entry.relativePath = std::filesystem::relative(it->path(), AreaPath(area), error);
            if (error)
                break;
            entry.directory = it->is_directory(error);
            if (error)
                break;
            if (!entry.directory)
            {
                entry.size = it->file_size(error);
                if (error)
                    break;
            }
            entries.push_back(std::move(entry));
        }
        if (error)
            m_Failures.fetch_add(1, std::memory_order_relaxed);
        else
            m_Reads.fetch_add(1, std::memory_order_relaxed);
        return entries;
    }

    bool FileSystem::Remove(FileArea area, const std::filesystem::path& relative) noexcept
    {
        if (!IoAllowed())
            return false;
        const auto path = Resolve(area, relative);
        if (!path)
            return false;
        std::error_code error;
        const bool removed = std::filesystem::remove(*path, error);
        if (error)
            m_Failures.fetch_add(1, std::memory_order_relaxed);
        return removed && !error;
    }

    FileSystemSnapshot FileSystem::Snapshot() const noexcept
    {
        return {
            .reads = m_Reads.load(std::memory_order_acquire),
            .writes = m_Writes.load(std::memory_order_acquire),
            .bytesRead = m_BytesRead.load(std::memory_order_acquire),
            .bytesWritten = m_BytesWritten.load(std::memory_order_acquire),
            .failures = m_Failures.load(std::memory_order_acquire),
            .rejectedPaths = m_RejectedPaths.load(std::memory_order_acquire),
            .rejectedAffinity = m_RejectedAffinity.load(std::memory_order_acquire),
        };
    }

    const std::filesystem::path& FileSystem::AreaPath(FileArea area) const noexcept
    {
        switch (area)
        {
        case FileArea::Logs: return m_Logs;
        case FileArea::Configs: return m_Configs;
        case FileArea::Themes: return m_Themes;
        case FileArea::Images: return m_Images;
        case FileArea::Fonts: return m_Fonts;
        case FileArea::Scripts: return m_Scripts;
        case FileArea::Cache: return m_Cache;
        case FileArea::Temp: return m_Temp;
        case FileArea::Crashes: return m_Crashes;
        case FileArea::Root:
        default: return m_Root;
        }
    }

    bool FileSystem::IoAllowed() const noexcept
    {
        const auto affinity = Tasking::CurrentTaskAffinity();
        if (affinity != Tasking::TaskAffinity::Game && affinity != Tasking::TaskAffinity::Render)
            return true;
        m_RejectedAffinity.fetch_add(1, std::memory_order_relaxed);
        m_Failures.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    bool FileSystem::ValidRelativePath(const std::filesystem::path& relative) const noexcept
    {
        if (relative.is_absolute() || relative.has_root_name() || relative.has_root_directory())
            return false;
        for (const auto& part : relative)
        {
            if (part == "..")
                return false;
        }
        return true;
    }

    bool FileSystem::CommitAtomic(
        const std::filesystem::path& temporary,
        const std::filesystem::path& destination) noexcept
    {
#if defined(_WIN32)
        return MoveFileExW(temporary.c_str(), destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
        std::error_code error;
        std::filesystem::rename(temporary, destination, error);
        if (!error)
            return true;
        error.clear();
        std::filesystem::remove(destination, error);
        error.clear();
        std::filesystem::rename(temporary, destination, error);
        return !error;
#endif
    }
}
