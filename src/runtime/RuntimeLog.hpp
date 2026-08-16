#pragma once

#include <string_view>
#include <windows.h>

namespace Sick::Runtime::Log
{
    bool Initialize(HMODULE module) noexcept;
    void Shutdown() noexcept;
    void Write(std::string_view message) noexcept;
}
