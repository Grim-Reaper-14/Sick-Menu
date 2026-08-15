#pragma once

#include "Sink.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace Sick::Core::Logging
{
    class FileSink final : public Sink
    {
    public:
        FileSink(
            std::filesystem::path path,
            std::size_t maxBytes = 8 * 1024 * 1024,
            std::size_t maxFiles = 5,
            bool jsonLines = false,
            Level minimum = Level::Trace);

        void Write(const LogRecord& record) noexcept override;
        void Flush() noexcept override;

        [[nodiscard]] const std::filesystem::path& Path() const noexcept { return m_Path; }

    private:
        bool OpenLocked(bool truncate = false);
        void RotateLocked();

        std::filesystem::path m_Path;
        std::size_t m_MaxBytes{};
        std::size_t m_MaxFiles{};
        bool m_JsonLines{};
        Level m_Minimum{Level::Trace};
        std::size_t m_CurrentBytes{};
        std::ofstream m_Stream;
        std::mutex m_Mutex;
    };
}
