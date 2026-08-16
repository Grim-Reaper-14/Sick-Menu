#pragma once

#include "DetourHook.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Sick::Hooking
{
    enum class HookOperation
    {
        Initialize,
        Create,
        QueueEnable,
        ApplyEnable,
        QueueDisable,
        ApplyDisable,
        Remove,
        Uninitialize
    };

    struct HookDiagnostic
    {
        std::string name;
        HookOperation operation{};
        int backendStatus{};
    };

    class HookManager final
    {
    public:
        static HookManager& Get() noexcept;

        HookManager(const HookManager&) = delete;
        HookManager& operator=(const HookManager&) = delete;

        [[nodiscard]] bool Initialize() noexcept;
        void Shutdown() noexcept;

        template <typename T>
        [[nodiscard]] bool AddDetour(std::string name, void* target, T detour) noexcept
        {
            static_assert(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>,
                "HookManager::AddDetour requires a function pointer");
            return AddDetourRaw(std::move(name), target, reinterpret_cast<void*>(detour));
        }

        [[nodiscard]] bool AddDetourRaw(std::string name, void* target, void* detour) noexcept;
        [[nodiscard]] bool EnableAll() noexcept;
        [[nodiscard]] bool DisableAll() noexcept;

        template <typename T>
        [[nodiscard]] T Original(std::string_view name) const noexcept
        {
            static_assert(std::is_pointer_v<T>, "HookManager::Original<T>() requires a pointer type");
            return reinterpret_cast<T>(OriginalRaw(name));
        }

        [[nodiscard]] void* OriginalRaw(std::string_view name) const noexcept;
        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] bool Enabled() const noexcept;
        [[nodiscard]] std::size_t Count() const noexcept;
        [[nodiscard]] std::vector<HookDiagnostic> Diagnostics() const;
        void ClearDiagnostics() noexcept;

    private:
        HookManager() = default;
        ~HookManager();

        [[nodiscard]] bool DisableAllLocked() noexcept;
        void Record(std::string_view name, HookOperation operation, int backendStatus) noexcept;

        mutable std::mutex m_Mutex;
        std::unordered_map<std::string, std::unique_ptr<DetourHook>> m_Hooks;
        std::vector<HookDiagnostic> m_Diagnostics;
        bool m_Initialized{};
        bool m_OwnsRuntime{};
        bool m_Enabled{};
    };
}
