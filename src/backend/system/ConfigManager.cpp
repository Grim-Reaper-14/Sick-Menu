#include "ConfigManager.hpp"

#include "FileSystem.hpp"
#include "IoService.hpp"
#include "LoggerApi.hpp"

#include <algorithm>
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
                {"player", {
                    {"god_mode", profile.player.godMode},
                    {"infinite_oxygen", profile.player.infiniteOxygen},
                    {"no_ragdoll", profile.player.noRagdoll},
                    {"super_jump", profile.player.superJump},
                    {"seat_belt", profile.player.seatBelt},
                    {"no_wanted_level", profile.player.noWantedLevel},
                    {"wanted_level", profile.player.wantedLevel},
                    {"fast_run", profile.player.fastRun},
                    {"fast_swim", profile.player.fastSwim},
                    {"keep_player_clean", profile.player.keepPlayerClean},
                    {"aqualung", profile.player.aqualung},
                    {"no_gravity", profile.player.noGravity},
                    {"waterproof", profile.player.waterproof},
                }},
                {"vehicle", {
                    {"god_mode", profile.vehicle.godMode},
                    {"auto_repair", profile.vehicle.autoRepair},
                    {"keep_clean", profile.vehicle.keepClean},
                    {"engine_always_on", profile.vehicle.engineAlwaysOn},
                    {"no_gravity", profile.vehicle.noGravity},
                    {"no_collision", profile.vehicle.noCollision},
                }},
            }.dump(2) + '\n';
        }

        std::optional<FeatureProfile> ParseProfile(std::string_view contents) noexcept
        {
            try
            {
                const auto root = nlohmann::json::parse(contents);
                if (!root.is_object())
                    return std::nullopt;

                const auto version = root.at("version").get<std::uint32_t>();
                if (version < 1 || version > FeatureProfile::CurrentVersion)
                    return std::nullopt;

                const auto& player = root.at("player");
                if (!player.is_object())
                    return std::nullopt;

                FeatureProfile profile{};
                profile.player.godMode = player.at("god_mode").get<bool>();
                if (version == 1)
                    return profile;

                profile.player.infiniteOxygen = player.value("infinite_oxygen", false);
                profile.player.noRagdoll = player.value("no_ragdoll", false);
                profile.player.superJump = player.value("super_jump", false);
                profile.player.seatBelt = player.value("seat_belt", false);
                profile.player.noWantedLevel = player.value("no_wanted_level", false);
                profile.player.wantedLevel = std::clamp(player.value("wanted_level", 0), 0, 5);
                profile.player.fastRun = player.value("fast_run", false);
                profile.player.fastSwim = player.value("fast_swim", false);
                profile.player.keepPlayerClean = player.value("keep_player_clean", false);
                profile.player.aqualung = player.value("aqualung", false);
                profile.player.noGravity = player.value("no_gravity", false);
                profile.player.waterproof = player.value("waterproof", false);

                if (version < 3)
                    return profile;

                const auto vehicleIt = root.find("vehicle");
                if (vehicleIt == root.end() || !vehicleIt->is_object())
                    return std::nullopt;
                const auto& vehicle = *vehicleIt;
                profile.vehicle.godMode = vehicle.value("god_mode", false);
                profile.vehicle.autoRepair = vehicle.value("auto_repair", false);
                profile.vehicle.keepClean = vehicle.value("keep_clean", false);
                profile.vehicle.engineAlwaysOn = vehicle.value("engine_always_on", false);
                profile.vehicle.noGravity = vehicle.value("no_gravity", false);
                profile.vehicle.noCollision = vehicle.value("no_collision", false);
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
