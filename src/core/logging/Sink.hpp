#pragma once

#include "LogRecord.hpp"

#include <memory>

namespace Sick::Core::Logging
{
    class Sink
    {
    public:
        virtual ~Sink() = default;
        virtual void Write(const LogRecord& record) noexcept = 0;
        virtual void Flush() noexcept = 0;
    };

    using SinkPtr = std::shared_ptr<Sink>;
}
