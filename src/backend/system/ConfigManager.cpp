#include "ConfigManager.hpp"

#include <cctype>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System
{
    namespace
    {
        void RemoveTemporary(const std::filesystem::path& path) noexcept
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }

        bool CommitConfigFile(
            const std::filesystem::path& temporary,
            const std::filesystem::path& destination) noexcept
        {
#if defined(_WIN32)
            return MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
            std::error_code error;
            std::filesystem::rename(temporary, destination, error);
            if (!error)
                return true;

            std::filesystem::remove(destination, error);
            error.clear();
            std::filesystem::rename(temporary, destination, error);
            return !error;
#endif
        }

        bool WriteProfile(
            const std::filesystem::path& path,
            const FeatureProfile& profile) noexcept
        {
            try
            {
                nlohmann::json root{
                    {"version", profile.version},
                    {"player", {
                        {"god_mode", profile.player.godMode},
                    }},
                };

                auto temporary = path;
                temporary += ".tmp";
                bool writeSucceeded{};
                {
                    std::ofstream output(temporary, std::ios::out | std::ios::trunc);
                    if (output)
                    {
                        output << root.dump(2) << '\n';
                        output.flush();
                        writeSucceeded = static_cast<bool>(output);
                    }
                }

                if (!writeSucceeded)
                {
                    RemoveTemporary(temporary);
                    return false;
                }

                if (CommitConfigFile(temporary, path))
                    return true;

                RemoveTemporary(temporary);
                return false;
            }
            catch (...)
            {
                return false;
            }
        }

        std::optional<FeatureProfile> ReadProfile(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::ifstream input(path);
                if (!input)
                    return std::nullopt;

                nlohmann::json root;
                input >> root;
                if (!root.is_object())
                    return std::nullopt;

                FeatureProfile profile{};
                profile.version = root.at("version").get<std::uint32_t>();
                if (profile.version != FeatureProfile::CurrentVersion)
                    return std::nullopt;

                const auto& player = root.at("player");
                if (!player.is_object())
                    return std::nullopt;
                profile.player.godMode = player.at("god_mode").get<bool>();
                return profile;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }

    bool ConfigManager::Initialize(
        Tasking::ThreadPool& pool,
        const std::filesystem::path& directory) noexcept
    {
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error)
            return false;

        std::scoped_lock lock(m_Mutex);
        m_Pool = &pool;
        m_Directory = directory;
        m_PendingProfile.reset();
        m_LoadGeneration = 0;
        m_Enabled = true;
        return true;
    }

    void ConfigManager::Shutdown() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Enabled = false;
        ++m_LoadGeneration;
        m_PendingProfile.reset();
        m_Pool = nullptr;
    }

    bool ConfigManager::Save(std::string_view name, FeatureProfile profile)
    {
        if (!ValidName(name) || profile.version != FeatureProfile::CurrentVersion)
            return false;

        Tasking::ThreadPool* pool{};
        std::filesystem::path path;
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Pool)
                return false;
            pool = m_Pool;
            path = m_Directory / (std::string(name) + ".json");
        }

        return pool->Submit([path = std::move(path), profile]() {
            static_cast<void>(WriteProfile(path, profile));
        });
    }

    bool ConfigManager::Load(std::string_view name)
    {
        if (!ValidName(name))
            return false;

        Tasking::ThreadPool* pool{};
        std::filesystem::path path;
        std::uint64_t generation{};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Pool)
                return false;
            pool = m_Pool;
            path = m_Directory / (std::string(name) + ".json");
            generation = ++m_LoadGeneration;
        }

        return pool->Submit([this, path = std::move(path), generation]() {
            const auto profile = ReadProfile(path);
            if (!profile)
                return;

            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || generation != m_LoadGeneration)
                return;
            m_PendingProfile = *profile;
        });
    }

    std::optional<FeatureProfile> ConfigManager::TakePendingProfile() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        auto profile = std::move(m_PendingProfile);
        m_PendingProfile.reset();
        return profile;
    }

    bool ConfigManager::ValidName(std::string_view name) noexcept
    {
        if (name.empty() || name.size() > 64)
            return false;

        for (const char character : name)
        {
            const auto value = static_cast<unsigned char>(character);
            if (std::isalnum(value) == 0 && character != '_' && character != '-')
                return false;
        }
        return true;
    }
}
