#include "ConsoleSink.hpp"
#include "../Formatter.hpp"

#include <iostream>

namespace Sick::Core::Logging
{
    void ConsoleSink::Write(const LogRecord& record) noexcept
    {
        if (!Enabled(record.level, m_Minimum))
            return;

        try
        {
            std::scoped_lock lock(m_Mutex);
            auto& stream = record.level >= Level::Error ? std::cerr : std::clog;
            stream << FormatText(record) << '\n';
        }
        catch (...)
        {
        }
    }

    void ConsoleSink::Flush() noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            std::clog.flush();
            std::cerr.flush();
        }
        catch (...)
        {
        }
    }
}
