#pragma once

#include "sick/core/logger.hpp"
#include "sick/core/scheduler.hpp"

#include <Windows.h>

#include <atomic>

namespace sick::core
{
    class Core final
    {
    public:
        explicit Core(HMODULE module) noexcept;

        bool initialize();
        void run();
        void request_stop() noexcept;
        void shutdown() noexcept;

    private:
        HMODULE m_module{};
        Logger m_logger;
        Scheduler m_scheduler;
        std::atomic_bool m_running{false};
    };
}
