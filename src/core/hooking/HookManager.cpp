#include "HookManager.hpp"

#if defined(_WIN32)
#include <MinHook.h>
#endif

namespace Sick::Hooking
{
    HookManager& HookManager::Get() noexcept
    {
        static HookManager instance;
        return instance;
    }

    HookManager::~HookManager()
    {
        Shutdown();
    }

    bool HookManager::Initialize() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        if (m_Initialized)
            return true;

#if defined(_WIN32)
        const auto status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
        {
            Record("MinHook", HookOperation::Initialize, static_cast<int>(status));
            return false;
        }

        m_OwnsRuntime = status == MH_OK;
        m_Initialized = true;
        return true;
#else
        Record("MinHook", HookOperation::Initialize, -1);
        return false;
#endif
    }

    void HookManager::Shutdown() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        if (!m_Initialized)
            return;

        (void)DisableAllLocked();

        for (auto& [name, hook] : m_Hooks)
        {
            if (!hook->Remove())
                Record(name, HookOperation::Remove, hook->LastBackendStatus());
        }
        m_Hooks.clear();

#if defined(_WIN32)
        if (m_OwnsRuntime)
        {
            const auto status = MH_Uninitialize();
            if (status != MH_OK && status != MH_ERROR_NOT_INITIALIZED)
                Record("MinHook", HookOperation::Uninitialize, static_cast<int>(status));
        }
#endif

        m_Enabled = false;
        m_OwnsRuntime = false;
        m_Initialized = false;
    }

    bool HookManager::AddDetourRaw(std::string name, void* target, void* detour) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        if (!m_Initialized || m_Enabled || name.empty() || target == nullptr || detour == nullptr)
            return false;

        if (m_Hooks.contains(name))
            return false;

        auto hook = std::make_unique<DetourHook>(name, target, detour);
        if (!hook->Create())
        {
            Record(name, HookOperation::Create, hook->LastBackendStatus());
            return false;
        }

        m_Hooks.emplace(std::move(name), std::move(hook));
        return true;
    }

    bool HookManager::EnableAll() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        if (!m_Initialized)
            return false;
        if (m_Enabled)
            return true;
        if (m_Hooks.empty())
            return true;

#if defined(_WIN32)
        for (auto& [name, hook] : m_Hooks)
        {
            if (!hook->QueueEnable())
            {
                Record(name, HookOperation::QueueEnable, hook->LastBackendStatus());
                for (auto& [queuedName, queuedHook] : m_Hooks)
                {
                    (void)queuedName;
                    (void)MH_QueueDisableHook(queuedHook->Target());
                }
                (void)MH_ApplyQueued();
                return false;
            }
        }

        const auto status = MH_ApplyQueued();
        if (status != MH_OK)
        {
            Record("MinHook", HookOperation::ApplyEnable, static_cast<int>(status));
            for (auto& [name, hook] : m_Hooks)
            {
                (void)MH_DisableHook(hook->Target());
                hook->SetEnabled(false);
            }
            return false;
        }

        for (auto& [name, hook] : m_Hooks)
            hook->SetEnabled(true);
        m_Enabled = true;
        return true;
#else
        return false;
#endif
    }

    bool HookManager::DisableAll() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return DisableAllLocked();
    }

    bool HookManager::DisableAllLocked() noexcept
    {
        if (!m_Initialized)
            return true;
        if (!m_Enabled)
            return true;

#if defined(_WIN32)
        bool queued = true;
        for (auto& [name, hook] : m_Hooks)
        {
            if (!hook->QueueDisable())
            {
                Record(name, HookOperation::QueueDisable, hook->LastBackendStatus());
                queued = false;
            }
        }

        const auto status = MH_ApplyQueued();
        if (status != MH_OK)
        {
            Record("MinHook", HookOperation::ApplyDisable, static_cast<int>(status));
            return false;
        }

        for (auto& [name, hook] : m_Hooks)
            hook->SetEnabled(false);
        m_Enabled = false;
        return queued;
#else
        return false;
#endif
    }

    void* HookManager::OriginalRaw(std::string_view name) const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        const auto it = m_Hooks.find(std::string{name});
        if (it == m_Hooks.end())
            return nullptr;
        return it->second->OriginalRaw();
    }

    bool HookManager::Ready() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Initialized;
    }

    bool HookManager::Enabled() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Enabled;
    }

    std::size_t HookManager::Count() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Hooks.size();
    }

    std::vector<HookDiagnostic> HookManager::Diagnostics() const
    {
        std::scoped_lock lock(m_Mutex);
        return m_Diagnostics;
    }

    void HookManager::ClearDiagnostics() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Diagnostics.clear();
    }

    void HookManager::Record(std::string_view name, HookOperation operation, int backendStatus) noexcept
    {
        try
        {
            m_Diagnostics.push_back(HookDiagnostic{std::string{name}, operation, backendStatus});
        }
        catch (...)
        {
        }
    }
}
