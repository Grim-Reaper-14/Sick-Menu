#include "Module.hpp"

#include <utility>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Memory
{
    Module::Module(std::string name, std::uintptr_t base, std::size_t size) :
        m_Name(std::move(name)),
        m_Base(base),
        m_Size(size)
    {
    }

    std::optional<Module> Module::FromLoaded(std::string_view name)
    {
#if defined(_WIN32)
        HMODULE handle{};
        if (name.empty())
        {
            handle = ::GetModuleHandleW(nullptr);
        }
        else
        {
            const std::string moduleName{name};
            handle = ::GetModuleHandleA(moduleName.c_str());
        }

        if (!handle)
            return std::nullopt;

        const auto base = reinterpret_cast<std::uintptr_t>(handle);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
            return std::nullopt;

        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
            return std::nullopt;

        return Module{
            name.empty() ? std::string{MainModuleName} : std::string{name},
            base,
            static_cast<std::size_t>(nt->OptionalHeader.SizeOfImage)};
#else
        (void)name;
        return std::nullopt;
#endif
    }

    const std::string& Module::Name() const noexcept
    {
        return m_Name;
    }

    std::uintptr_t Module::Base() const noexcept
    {
        return m_Base;
    }

    std::size_t Module::Size() const noexcept
    {
        return m_Size;
    }

    std::uintptr_t Module::End() const noexcept
    {
        return m_Base + m_Size;
    }

    bool Module::Valid() const noexcept
    {
        return m_Base != 0 && m_Size != 0;
    }

    bool Module::Contains(std::uintptr_t address, std::size_t length) const noexcept
    {
        if (!Valid() || address < m_Base || length > m_Size)
            return false;

        const auto offset = address - m_Base;
        return offset <= m_Size - length;
    }

    std::span<const std::byte> Module::Bytes() const noexcept
    {
        if (!Valid())
            return {};

        return {
            reinterpret_cast<const std::byte*>(m_Base),
            m_Size};
    }
}
