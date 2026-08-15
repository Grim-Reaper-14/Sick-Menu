#include "FileSink.hpp"
#include "../Formatter.hpp"

#include <system_error>

namespace Sick::Core::Logging
{
    FileSink::FileSink(
        std::filesystem::path path,
        std::size_t maxBytes,
        std::size_t maxFiles,
        bool jsonLines,
        Level minimum) :
        m_Path(std::move(path)),
        m_MaxBytes(maxBytes),
        m_MaxFiles(maxFiles),
        m_JsonLines(jsonLines),
        m_Minimum(minimum)
    {
        std::scoped_lock lock(m_Mutex);
        OpenLocked(false);
    }

    bool FileSink::OpenLocked(bool truncate)
    {
        std::error_code ec;
        if (const auto parent = m_Path.parent_path(); !parent.empty())
            std::filesystem::create_directories(parent, ec);

        const auto mode = std::ios::out | (truncate ? std::ios::trunc : std::ios::app);
        m_Stream.open(m_Path, mode);
        if (!m_Stream.is_open())
            return false;

        if (truncate)
        {
            m_CurrentBytes = 0;
        }
        else
        {
            m_CurrentBytes = std::filesystem::exists(m_Path, ec)
                ? static_cast<std::size_t>(std::filesystem::file_size(m_Path, ec))
                : 0;
        }

        return true;
    }

    void FileSink::RotateLocked()
    {
        if (m_Stream.is_open())
        {
            m_Stream.flush();
            m_Stream.close();
        }

        std::error_code ec;
        if (m_MaxFiles == 0)
        {
            std::filesystem::remove(m_Path, ec);
            OpenLocked(true);
            return;
        }

        for (std::size_t index = m_MaxFiles; index > 1; --index)
        {
            const auto older = std::filesystem::path{m_Path.string() + "." + std::to_string(index - 1)};
            const auto newer = std::filesystem::path{m_Path.string() + "." + std::to_string(index)};

            std::filesystem::remove(newer, ec);
            ec.clear();
            if (std::filesystem::exists(older, ec))
            {
                ec.clear();
                std::filesystem::rename(older, newer, ec);
            }
            ec.clear();
        }

        const auto first = std::filesystem::path{m_Path.string() + ".1"};
        std::filesystem::remove(first, ec);
        ec.clear();
        if (std::filesystem::exists(m_Path, ec))
        {
            ec.clear();
            std::filesystem::rename(m_Path, first, ec);
        }

        OpenLocked(true);
    }

    void FileSink::Write(const LogRecord& record) noexcept
    {
        if (!Enabled(record.level, m_Minimum))
            return;

        try
        {
            const auto line = m_JsonLines ? FormatJson(record) : FormatText(record);
            std::scoped_lock lock(m_Mutex);

            if (!m_Stream.is_open() && !OpenLocked(false))
                return;

            const auto required = line.size() + 1;
            if (m_MaxBytes != 0 && m_CurrentBytes + required > m_MaxBytes)
                RotateLocked();

            if (!m_Stream.is_open())
                return;

            m_Stream << line << '\n';
            m_CurrentBytes += required;

            if (record.level >= Level::Error)
                m_Stream.flush();
        }
        catch (...)
        {
        }
    }

    void FileSink::Flush() noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            if (m_Stream.is_open())
                m_Stream.flush();
        }
        catch (...)
        {
        }
    }
}
