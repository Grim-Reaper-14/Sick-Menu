#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace sick::core
{
    class Settings final
    {
    public:
        bool load(const std::filesystem::path& path);
        bool save(const std::filesystem::path& path) const;

        void set_bool(std::string key, bool value);
        [[nodiscard]] bool get_bool(const std::string& key, bool fallback = false) const;

        void clear() noexcept;

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, bool> m_bool_values;
    };
}
