#include "HandlingProfileManager.hpp"

#include "FileSystem.hpp"
#include "IoService.hpp"
#include "LoggerApi.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace Sick::Backend::System
{
    namespace
    {
        std::filesystem::path ProfilePath(std::string_view name)
        {
            return std::filesystem::path{"handling"} / (std::string{name} + ".json");
        }

        std::string AutoName(std::uint64_t sequence)
        {
            const auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            return "handling-" + std::to_string(ticks) + "-" + std::to_string(sequence);
        }

        std::string Serialize(const Handling::Values& values)
        {
            nlohmann::json fields = nlohmann::json::object();
            for (std::size_t index = 0; index < Handling::FieldCount; ++index)
                fields[Handling::FieldSpecs[index].key] = values[index];
            return nlohmann::json{{"schema", 1}, {"values", std::move(fields)}}.dump(2) + '\n';
        }

        std::optional<Handling::Values> Parse(std::string_view contents)
        {
            try
            {
                const auto root = nlohmann::json::parse(contents);
                if (!root.is_object() || root.value("schema", 0) != 1 ||
                    !root.contains("values") || !root["values"].is_object())
                    return std::nullopt;

                Handling::Values values{};
                const auto& fields = root["values"];
                for (std::size_t index = 0; index < Handling::FieldCount; ++index)
                {
                    const auto& spec = Handling::FieldSpecs[index];
                    const auto it = fields.find(spec.key);
                    if (it == fields.end() || !it->is_number())
                        return std::nullopt;
                    auto value = std::clamp(it->get<float>(), spec.minimum, spec.maximum);
                    if (spec.integral)
                        value = std::round(value);
                    values[index] = value;
                }
                return values;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }

    bool HandlingProfileManager::Initialize(IoService& io, FileSystem& files) noexcept
    {
        {
            std::scoped_lock lock(m_Mutex);
            m_Io = &io;
            m_Files = &files;
            m_PendingValues.reset();
            m_Profiles.clear();
            m_LoadGeneration = 0;
            m_RefreshGeneration = 0;
            m_Enabled = true;
        }
        m_PublishedGeneration.store(0, std::memory_order_release);
        m_SaveSequence.store(0, std::memory_order_release);
        return Refresh();
    }

    void HandlingProfileManager::Shutdown() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Enabled = false;
        ++m_LoadGeneration;
        ++m_RefreshGeneration;
        m_PendingValues.reset();
        m_Profiles.clear();
        m_Io = nullptr;
        m_Files = nullptr;
    }

    bool HandlingProfileManager::Save(const Handling::Values& values)
    {
        IoService* io{};
        FileSystem* files{};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
        }

        const auto sequence = m_SaveSequence.fetch_add(1, std::memory_order_relaxed);
        const auto name = AutoName(sequence);
        return io->Submit(IoPriority::Normal, [this, files, values, name]() {
            const auto path = ProfilePath(name);
            if (!files->AtomicWriteText(FileArea::Configs, path, Serialize(values)))
            {
                LoggerApi::Get().Error("handling", "Handling profile save failed", {{"file", path.string()}});
                return;
            }

            {
                std::scoped_lock lock(m_Mutex);
                if (!m_Enabled)
                    return;
                if (std::find(m_Profiles.begin(), m_Profiles.end(), name) == m_Profiles.end())
                {
                    m_Profiles.push_back(name);
                    std::sort(m_Profiles.begin(), m_Profiles.end());
                }
            }
            m_PublishedGeneration.fetch_add(1, std::memory_order_acq_rel);
            LoggerApi::Get().Info("handling", "Handling profile saved", {{"profile", name}});
        }).has_value();
    }

    bool HandlingProfileManager::Load(std::string_view name)
    {
        if (!ValidName(name))
            return false;

        IoService* io{};
        FileSystem* files{};
        std::uint64_t generation{};
        const std::string profileName{name};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
            generation = ++m_LoadGeneration;
        }

        return io->Submit(IoPriority::Critical, [this, files, profileName, generation]() {
            const auto path = ProfilePath(profileName);
            const auto contents = files->ReadText(FileArea::Configs, path);
            const auto values = contents ? Parse(*contents) : std::nullopt;
            if (!values)
            {
                LoggerApi::Get().Warn("handling", "Handling profile load rejected", {{"file", path.string()}});
                return;
            }

            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || generation != m_LoadGeneration)
                return;
            m_PendingValues = *values;
        }).has_value();
    }

    bool HandlingProfileManager::Refresh()
    {
        IoService* io{};
        FileSystem* files{};
        std::uint64_t generation{};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
            generation = ++m_RefreshGeneration;
        }

        return io->Submit(IoPriority::Normal, [this, files, generation]() {
            std::vector<std::string> profiles;
            if (files->Exists(FileArea::Configs, "handling"))
            {
                for (const auto& entry : files->List(FileArea::Configs, "handling"))
                {
                    if (entry.directory || entry.relativePath.extension() != ".json")
                        continue;
                    profiles.push_back(entry.relativePath.stem().string());
                }
            }
            std::sort(profiles.begin(), profiles.end());
            profiles.erase(std::unique(profiles.begin(), profiles.end()), profiles.end());

            {
                std::scoped_lock lock(m_Mutex);
                if (!m_Enabled || generation != m_RefreshGeneration)
                    return;
                m_Profiles = std::move(profiles);
            }
            m_PublishedGeneration.fetch_add(1, std::memory_order_acq_rel);
        }).has_value();
    }

    std::optional<Handling::Values> HandlingProfileManager::TakePendingValues() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        auto values = std::move(m_PendingValues);
        m_PendingValues.reset();
        return values;
    }

    HandlingProfileCatalogSnapshot HandlingProfileManager::Snapshot() const
    {
        std::scoped_lock lock(m_Mutex);
        return {
            .generation = m_PublishedGeneration.load(std::memory_order_acquire),
            .names = m_Profiles,
        };
    }

    std::uint64_t HandlingProfileManager::Generation() const noexcept
    {
        return m_PublishedGeneration.load(std::memory_order_acquire);
    }

    bool HandlingProfileManager::ValidName(std::string_view name) noexcept
    {
        if (name.empty() || name.size() > 96)
            return false;
        for (const char character : name)
        {
            const auto value = static_cast<unsigned char>(character);
            if (std::isalnum(value) == 0 && character != '_' && character != '-' && character != ' ')
                return false;
        }
        return true;
    }
}
