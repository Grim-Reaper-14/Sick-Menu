#include "sick/core/logger.hpp"

#include <chrono>
#include <format>

namespace sick::core
{
    bool Logger::open(const std::filesystem::path& path)
    {
        std::scoped_lock lock(m_mutex);
        m_stream.open(path, std::ios::out | std::ios::trunc);
        return m_stream.is_open();
    }

    void Logger::info(const std::string_view message)
    {
        write("INFO", message);
    }

    void Logger::error(const std::string_view message)
    {
        write("ERROR", message);
    }

    void Logger::close()
    {
        std::scoped_lock lock(m_mutex);
        if (m_stream.is_open())
            m_stream.close();
    }

    void Logger::write(const std::string_view level, const std::string_view message)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_stream.is_open())
            return;

        const auto now = std::chrono::system_clock::now();
        m_stream << std::format("[{:%F %T}] [{}] {}\n", now, level, message);
        m_stream.flush();
    }
}
