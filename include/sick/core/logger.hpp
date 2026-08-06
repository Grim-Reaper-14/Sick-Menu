#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace sick::core
{
    class Logger final
    {
    public:
        bool open(const std::filesystem::path& path);
        void info(std::string_view message);
        void error(std::string_view message);
        void close();

    private:
        void write(std::string_view level, std::string_view message);

        std::mutex m_mutex;
        std::ofstream m_stream;
    };
}
