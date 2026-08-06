#pragma once

#include "sick/core/logger.hpp"

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
        std::atomic_bool m_running{false};
    };
}
