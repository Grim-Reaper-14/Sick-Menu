#include "sick/memory/module.hpp"

namespace sick::memory
{
    Module::Module(const std::wstring_view name) noexcept
    {
        m_handle = name.empty()
            ? GetModuleHandleW(nullptr)
            : GetModuleHandleW(name.data());

        if (m_handle == nullptr)
            return;

        const auto base = reinterpret_cast<std::uintptr_t>(m_handle);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            m_handle = nullptr;
            return;
        }

        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            m_handle = nullptr;
            return;
        }

        m_base = base;
        m_size = static_cast<std::size_t>(nt->OptionalHeader.SizeOfImage);
    }

    bool Module::valid() const noexcept
    {
        return m_handle != nullptr && m_base != 0 && m_size != 0;
    }

    HMODULE Module::handle() const noexcept
    {
        return m_handle;
    }

    std::uintptr_t Module::base() const noexcept
    {
        return m_base;
    }

    std::size_t Module::size() const noexcept
    {
        return m_size;
    }
}
