#include "sick/core/core.hpp"

#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <thread>

namespace sick::core
{
    Core::Core(const HMODULE module) noexcept
        : m_module(module)
    {
    }

    bool Core::initialize()
    {
        wchar_t module_path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(m_module, module_path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
            return false;

        const std::filesystem::path log_path =
            std::filesystem::path(module_path).parent_path() / L"SickMenu.log";

        if (!m_logger.open(log_path))
            return false;

        m_logger.info("Sick-Menu core initialized");
        m_logger.info("Press END to unload");
        m_running.store(true, std::memory_order_release);
        return true;
    }

    void Core::run()
    {
        using namespace std::chrono_literals;

        while (m_running.load(std::memory_order_acquire))
        {
            if ((GetAsyncKeyState(VK_END) & 1) != 0)
                request_stop();

            m_scheduler.tick();
            std::this_thread::sleep_for(10ms);
        }
    }

    void Core::request_stop() noexcept
    {
        m_running.store(false, std::memory_order_release);
    }

    void Core::shutdown() noexcept
    {
        m_scheduler.clear();
        m_logger.info("Sick-Menu core shutting down");
        m_logger.close();
    }
}
