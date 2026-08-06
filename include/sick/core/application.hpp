#pragma once

#include "sick/core/core.hpp"

#include <Windows.h>

namespace sick::core
{
    class Application final
    {
    public:
        explicit Application(HMODULE module) noexcept;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        DWORD run();

    private:
        HMODULE m_module{};
        Core m_core;
    };
}
