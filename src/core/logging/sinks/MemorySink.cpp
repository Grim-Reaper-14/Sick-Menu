#include "MemorySink.hpp"

namespace Sick::Core::Logging
{
    void MemorySink::Write(const LogRecord& record) noexcept
    {
        try
        {
            std::scoped_lock lock(m_Mutex);
            if (m_Capacity == 0)
                return;

            while (m_Records.size() >= m_Capacity)
                m_Records.pop_front();

            m_Records.push_back(record);
        }
        catch (...)
        {
        }
    }

    std::vector<LogRecord> MemorySink::Snapshot(std::size_t maxRecords) const
    {
        std::scoped_lock lock(m_Mutex);

        const auto count = maxRecords == 0 || maxRecords > m_Records.size()
            ? m_Records.size()
            : maxRecords;
        const auto start = m_Records.size() - count;

        return std::vector<LogRecord>{m_Records.begin() + static_cast<std::ptrdiff_t>(start), m_Records.end()};
    }

    void MemorySink::Clear() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Records.clear();
    }
}
