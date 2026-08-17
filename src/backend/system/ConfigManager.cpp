#include "ConfigManager.hpp"

#include "FileSystem.hpp"
#include "IoService.hpp"
#include "LoggerApi.hpp"

#include <cctype>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace Sick::Backend::System
{
    namespace
    {
        std::string ProfileName(std::string_view name)
        {
            return std::string(name) + ".json";
        }

        std::string SerializeProfile(const FeatureProfile& profile)
        {
            return nlohmann::json{
                {"version", profile.version},
                {"player", {{"god_mode", profile.player.godMode}}},
            }.dump(2) + '\n';
        }

        std::optional<FeatureProfile> ParseProfile(std::string_view contents) noexcept
        {
            try
            {
                const auto root = nlohmann::json::parse(contents);
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

    bool ConfigManager::Initialize(IoService& io, FileSystem& files) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Io = &io;
        m_Files = &files;
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
        m_Io = nullptr;
        m_Files = nullptr;
    }

    bool ConfigManager::Save(std::string_view name, FeatureProfile profile)
    {
        if (!ValidName(name) || profile.version != FeatureProfile::CurrentVersion)
            return false;

        IoService* io{};
        FileSystem* files{};
        std::string filename = ProfileName(name);
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
        }

        return io->Submit(IoPriority::Normal, [files, filename = std::move(filename), profile]() {
            try
            {
                if (!files->AtomicWriteText(FileArea::Configs, filename, SerializeProfile(profile)))
                    LoggerApi::Get().Error("config", "Profile save failed", {{"file", filename}});
            }
            catch (...)
            {
                LoggerApi::Get().Error("config", "Profile serialization failed", {{"file", filename}});
            }
        }).has_value();
    }

    bool ConfigManager::Load(std::string_view name)
    {
        if (!ValidName(name))
            return false;

        IoService* io{};
        FileSystem* files{};
        std::string filename = ProfileName(name);
        std::uint64_t generation{};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
            generation = ++m_LoadGeneration;
        }

        return io->Submit(IoPriority::Critical, [this, files, filename = std::move(filename), generation]() {
            const auto contents = files->ReadText(FileArea::Configs, filename);
            const auto profile = contents ? ParseProfile(*contents) : std::nullopt;
            if (!profile)
            {
                LoggerApi::Get().Warn("config", "Profile load rejected", {{"file", filename}});
                return;
            }

            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || generation != m_LoadGeneration)
                return;
            m_PendingProfile = *profile;
        }).has_value();
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
