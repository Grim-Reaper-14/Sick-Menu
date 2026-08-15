#pragma once

#include "Sink.hpp"

namespace Sick::Core::Logging
{
    class DebuggerSink final : public Sink
    {
    public:
        explicit DebuggerSink(Level minimum = Level::Trace) noexcept : m_Minimum(minimum) {}

        void Write(const LogRecord& record) noexcept override;
        void Flush() noexcept override {}

    private:
        Level m_Minimum;
    };
}
