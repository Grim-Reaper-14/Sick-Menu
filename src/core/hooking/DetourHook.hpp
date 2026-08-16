#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace Sick::Hooking
{
    class DetourHook final
    {
    public:
        DetourHook(std::string name, void* target, void* detour) noexcept;
        ~DetourHook();

        DetourHook(const DetourHook&) = delete;
        DetourHook& operator=(const DetourHook&) = delete;
        DetourHook(DetourHook&&) = delete;
        DetourHook& operator=(DetourHook&&) = delete;

        [[nodiscard]] bool Create() noexcept;
        [[nodiscard]] bool QueueEnable() noexcept;
        [[nodiscard]] bool QueueDisable() noexcept;
        [[nodiscard]] bool Remove() noexcept;

        [[nodiscard]] std::string_view Name() const noexcept { return m_Name; }
        [[nodiscard]] void* Target() const noexcept { return m_Target; }
        [[nodiscard]] void* Detour() const noexcept { return m_Detour; }
        [[nodiscard]] void* OriginalRaw() const noexcept { return m_Original; }
        [[nodiscard]] bool Created() const noexcept { return m_Created; }
        [[nodiscard]] bool Enabled() const noexcept { return m_Enabled; }
        [[nodiscard]] int LastBackendStatus() const noexcept { return m_LastBackendStatus; }

        template <typename T>
        [[nodiscard]] T Original() const noexcept
        {
            static_assert(std::is_pointer_v<T>, "DetourHook::Original<T>() requires a pointer type");
            return reinterpret_cast<T>(m_Original);
        }

    private:
        friend class HookManager;
        void SetEnabled(bool enabled) noexcept { m_Enabled = enabled; }

        std::string m_Name;
        void* m_Target{};
        void* m_Detour{};
        void* m_Original{};
        bool m_Created{};
        bool m_Enabled{};
        int m_LastBackendStatus{};
    };
}
