#pragma once

#include "ModuleManager.hpp"
#include "PatternScanner.hpp"
#include "PointerCalculator.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Sick::Memory
{
    enum class AddressId : std::uint16_t
    {
        NativeRegistrationTable = 0,
        ScriptGlobals,
        RunScriptThreads,
        Count
    };

    class MemoryManager final
    {
    public:
        using ResolveCallback = std::function<void(MemoryManager&, PointerCalculator)>;

        static MemoryManager& Get() noexcept;

        bool RegisterModule(Module module);
        bool LoadModule(std::string_view name = {});

        bool AddPattern(
            std::string_view moduleName,
            Pattern pattern,
            ResolveCallback callback,
            bool required = true);

        [[nodiscard]] bool Scan();
        void ClearResolved() noexcept;
        void Reset() noexcept;

        void Set(AddressId id, PointerCalculator pointer) noexcept;
        [[nodiscard]] std::uintptr_t Address(AddressId id) const noexcept;
        [[nodiscard]] bool Has(AddressId id) const noexcept;

        template <typename T>
        [[nodiscard]] T Get(AddressId id) const noexcept
        {
            static_assert(std::is_pointer_v<T> || std::is_same_v<T, std::uintptr_t>,
                "MemoryManager::Get<T>() requires a pointer type or uintptr_t");

            return PointerCalculator{Address(id)}.template As<T>();
        }

        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] std::size_t PatternCount() const noexcept;
        [[nodiscard]] std::size_t ModuleCount() const noexcept;
        [[nodiscard]] std::vector<ScanDiagnostic> Diagnostics() const;

    private:
        struct Binding
        {
            std::string module;
            Pattern pattern;
            ResolveCallback callback;
            bool required{};
        };

        static constexpr std::size_t AddressCount = static_cast<std::size_t>(AddressId::Count);
        [[nodiscard]] static std::size_t Offset(AddressId id) noexcept;

        mutable std::shared_mutex m_Mutex;
        ModuleManager m_Modules;
        std::vector<Binding> m_Bindings;
        std::array<std::uintptr_t, AddressCount> m_Addresses{};
        std::vector<ScanDiagnostic> m_Diagnostics;
        bool m_Ready{};
    };
}
