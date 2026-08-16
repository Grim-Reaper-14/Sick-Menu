#include "DetourHook.hpp"

#include <utility>

#if defined(_WIN32)
#include <MinHook.h>
#endif

namespace Sick::Hooking
{
    DetourHook::DetourHook(std::string name, void* target, void* detour) noexcept :
        m_Name(std::move(name)),
        m_Target(target),
        m_Detour(detour)
    {
    }

    DetourHook::~DetourHook()
    {
        (void)Remove();
    }

    bool DetourHook::Create() noexcept
    {
        if (m_Created)
            return true;

        if (m_Target == nullptr || m_Detour == nullptr)
            return false;

#if defined(_WIN32)
        const auto status = MH_CreateHook(m_Target, m_Detour, &m_Original);
        m_LastBackendStatus = static_cast<int>(status);
        if (status != MH_OK)
            return false;

        m_Created = true;
        return true;
#else
        m_LastBackendStatus = -1;
        return false;
#endif
    }

    bool DetourHook::QueueEnable() noexcept
    {
        if (!m_Created || m_Enabled)
            return m_Created;

#if defined(_WIN32)
        const auto status = MH_QueueEnableHook(m_Target);
        m_LastBackendStatus = static_cast<int>(status);
        return status == MH_OK;
#else
        m_LastBackendStatus = -1;
        return false;
#endif
    }

    bool DetourHook::QueueDisable() noexcept
    {
        if (!m_Created || !m_Enabled)
            return m_Created;

#if defined(_WIN32)
        const auto status = MH_QueueDisableHook(m_Target);
        m_LastBackendStatus = static_cast<int>(status);
        return status == MH_OK;
#else
        m_LastBackendStatus = -1;
        return false;
#endif
    }

    bool DetourHook::Remove() noexcept
    {
        if (!m_Created)
            return true;

#if defined(_WIN32)
        if (m_Enabled)
        {
            const auto disableStatus = MH_DisableHook(m_Target);
            m_LastBackendStatus = static_cast<int>(disableStatus);
            if (disableStatus != MH_OK && disableStatus != MH_ERROR_DISABLED)
                return false;
            m_Enabled = false;
        }

        const auto status = MH_RemoveHook(m_Target);
        m_LastBackendStatus = static_cast<int>(status);
        if (status != MH_OK && status != MH_ERROR_NOT_CREATED)
            return false;

        m_Created = false;
        m_Original = nullptr;
        return true;
#else
        m_LastBackendStatus = -1;
        return false;
#endif
    }
}
