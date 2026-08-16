#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Sick::Memory
{
    class PointerCalculator final
    {
    public:
        constexpr PointerCalculator() noexcept = default;
        explicit constexpr PointerCalculator(std::uintptr_t address) noexcept : m_Address(address) {}

        template <typename T>
        explicit PointerCalculator(T* pointer) noexcept :
            m_Address(reinterpret_cast<std::uintptr_t>(pointer))
        {
        }

        [[nodiscard]] constexpr std::uintptr_t Address() const noexcept
        {
            return m_Address;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return m_Address != 0;
        }

        [[nodiscard]] constexpr PointerCalculator Add(std::ptrdiff_t offset) const noexcept
        {
            return PointerCalculator{static_cast<std::uintptr_t>(static_cast<std::intptr_t>(m_Address) + offset)};
        }

        [[nodiscard]] constexpr PointerCalculator Sub(std::ptrdiff_t offset) const noexcept
        {
            return Add(-offset);
        }

        template <typename T>
        [[nodiscard]] T As() const noexcept
        {
            static_assert(std::is_pointer_v<T> || std::is_same_v<T, std::uintptr_t>,
                "PointerCalculator::As<T>() requires a pointer type or uintptr_t");

            if constexpr (std::is_same_v<T, std::uintptr_t>)
                return m_Address;
            else
                return reinterpret_cast<T>(m_Address);
        }

        template <typename T>
        [[nodiscard]] T Read() const noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);

            T value{};
            if (m_Address != 0)
                std::memcpy(&value, reinterpret_cast<const void*>(m_Address), sizeof(T));
            return value;
        }

        [[nodiscard]] PointerCalculator Rip(
            std::ptrdiff_t displacementOffset = 0,
            std::size_t displacementSize = sizeof(std::int32_t)) const noexcept
        {
            if (m_Address == 0 || displacementSize != sizeof(std::int32_t))
                return {};

            const auto displacementAddress = Add(displacementOffset);
            const auto displacement = displacementAddress.Read<std::int32_t>();
            return displacementAddress
                .Add(static_cast<std::ptrdiff_t>(displacementSize))
                .Add(displacement);
        }

        [[nodiscard]] PointerCalculator Dereference(std::size_t count = 1) const noexcept
        {
            PointerCalculator current = *this;
            for (std::size_t i = 0; i < count && current; ++i)
                current = PointerCalculator{current.Read<std::uintptr_t>()};
            return current;
        }

        friend constexpr bool operator==(PointerCalculator lhs, PointerCalculator rhs) noexcept = default;

    private:
        std::uintptr_t m_Address{};
    };
}
