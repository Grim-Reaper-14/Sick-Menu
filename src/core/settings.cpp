#include "sick/core/settings.hpp"

#include <fstream>
#include <string>
#include <utility>

namespace sick::core
{
    bool Settings::load(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
            return false;

        std::unordered_map<std::string, bool> values;
        std::string line;

        while (std::getline(stream, line))
        {
            if (line.empty() || line.front() == '#')
                continue;

            const std::size_t separator = line.find('=');
            if (separator == std::string::npos)
                continue;

            std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);

            if (key.empty())
                continue;

            if (value == "true" || value == "1")
                values.insert_or_assign(std::move(key), true);
            else if (value == "false" || value == "0")
                values.insert_or_assign(std::move(key), false);
        }

        std::scoped_lock lock(m_mutex);
        m_bool_values = std::move(values);
        return true;
    }

    bool Settings::save(const std::filesystem::path& path) const
    {
        std::scoped_lock lock(m_mutex);

        std::ofstream stream(path, std::ios::out | std::ios::trunc);
        if (!stream.is_open())
            return false;

        stream << "# Sick-Menu settings\n";
        for (const auto& [key, value] : m_bool_values)
            stream << key << '=' << (value ? "true" : "false") << '\n';

        return stream.good();
    }

    void Settings::set_bool(std::string key, const bool value)
    {
        if (key.empty())
            return;

        std::scoped_lock lock(m_mutex);
        m_bool_values.insert_or_assign(std::move(key), value);
    }

    bool Settings::get_bool(const std::string& key, const bool fallback) const
    {
        std::scoped_lock lock(m_mutex);
        const auto found = m_bool_values.find(key);
        return found == m_bool_values.end() ? fallback : found->second;
    }

    void Settings::clear() noexcept
    {
        std::scoped_lock lock(m_mutex);
        m_bool_values.clear();
    }
}
