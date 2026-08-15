#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace Sick::Game::Enhanced
{
    class ScriptGlobal final
    {
    public:
        using ResolverFn = void* (*)(std::size_t index);

        constexpr explicit ScriptGlobal(std::size_t index) noexcept : m_Index(index) {}

        [[nodiscard]] constexpr ScriptGlobal At(std::ptrdiff_t offset) const noexcept
        {
            return ScriptGlobal(static_cast<std::size_t>(static_cast<std::ptrdiff_t>(m_Index) + offset));
        }

        [[nodiscard]] constexpr ScriptGlobal At(std::ptrdiff_t offset, std::size_t stride) const noexcept
        {
            return ScriptGlobal(static_cast<std::size_t>(static_cast<std::ptrdiff_t>(m_Index) + 1 + offset * static_cast<std::ptrdiff_t>(stride)));
        }

        static void BindResolver(ResolverFn resolver) noexcept;
        static void ResetResolver() noexcept;
        [[nodiscard]] static bool ResolverReady() noexcept;

        [[nodiscard]] bool CanAccess() const noexcept;
        [[nodiscard]] void* Address() const noexcept;
        [[nodiscard]] constexpr std::size_t Index() const noexcept { return m_Index; }

        template <typename T>
        [[nodiscard]] std::enable_if_t<std::is_pointer_v<T>, T> As() const noexcept
        {
            return static_cast<T>(Address());
        }

        template <typename T>
        [[nodiscard]] std::enable_if_t<std::is_lvalue_reference_v<T>, T> As() const
        {
            void* address = Address();
            assert(address != nullptr);
            using Value = std::remove_reference_t<T>;
            return *static_cast<Value*>(address);
        }

    private:
        std::size_t m_Index{};
        static inline std::atomic<ResolverFn> s_Resolver{nullptr};
    };
}
