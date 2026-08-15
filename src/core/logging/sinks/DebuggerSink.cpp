#include "DebuggerSink.hpp"
#include "../Formatter.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Sick::Core::Logging
{
    void DebuggerSink::Write(const LogRecord& record) noexcept
    {
        if (!Enabled(record.level, m_Minimum))
            return;

#ifdef _WIN32
        try
        {
            const auto line = FormatText(record) + "\n";
            OutputDebugStringA(line.c_str());
        }
        catch (...)
        {
        }
#else
        (void)record;
#endif
    }
}
