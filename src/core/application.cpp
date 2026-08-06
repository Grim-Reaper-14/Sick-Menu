#include "sick/core/application.hpp"

namespace sick::core
{
    Application::Application(const HMODULE module) noexcept
        : m_module(module), m_core(module)
    {
    }

    DWORD Application::run()
    {
        if (!m_core.initialize())
            return ERROR_DLL_INIT_FAILED;

        m_core.run();
        m_core.shutdown();
        return ERROR_SUCCESS;
    }
}
