#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sick::memory
{
    class Module final
    {
    public:
        explicit Module(std::wstring_view name = {}) noexcept;

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] HMODULE handle() const noexcept;
        [[nodiscard]] std::uintptr_t base() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        HMODULE m_handle{};
        std::uintptr_t m_base{};
        std::size_t m_size{};
    };
}
