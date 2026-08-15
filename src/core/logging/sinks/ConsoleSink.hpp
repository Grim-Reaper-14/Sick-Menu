#pragma once

#include "Sink.hpp"

#include <mutex>

namespace Sick::Core::Logging
{
    class ConsoleSink final : public Sink
    {
    public:
        explicit ConsoleSink(Level minimum = Level::Trace) noexcept : m_Minimum(minimum) {}

        void Write(const LogRecord& record) noexcept override;
        void Flush() noexcept override;

    private:
        Level m_Minimum;
        std::mutex m_Mutex;
    };
}
