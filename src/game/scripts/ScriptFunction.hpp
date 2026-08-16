#pragma once

#include "ScriptPointer.hpp"
#include "ScriptRuntime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

namespace Sick::Game::Scripts
{
    class ScriptFunction final
    {
    public:
        ScriptFunction(ScriptHash script, ScriptPointer pointer);

        ScriptFunction(const ScriptFunction&) = delete;
        ScriptFunction& operator=(const ScriptFunction&) = delete;

        template <typename Return = void, typename... Args>
        Return Call(Args&&... args) const noexcept
        {
            const auto arguments = PackArguments(std::forward<Args>(args)...);

            if constexpr (std::is_void_v<Return>)
            {
                static_cast<void>(Invoke(arguments));
            }
            else
            {
                static_assert(!std::is_reference_v<Return>);
                static_assert(std::is_trivially_copyable_v<Return>);

                Return value{};
                if (!Invoke(arguments, &value, sizeof(value)))
                    return Return{};

                return value;
            }
        }

        template <typename Return, typename... Args>
        [[nodiscard]] std::optional<Return> TryCall(Args&&... args) const noexcept
        {
            static_assert(!std::is_void_v<Return>);
            static_assert(!std::is_reference_v<Return>);
            static_assert(std::is_trivially_copyable_v<Return>);

            const auto arguments = PackArguments(std::forward<Args>(args)...);
            Return value{};
            if (!Invoke(arguments, &value, sizeof(value)))
                return std::nullopt;

            return value;
        }

        template <typename... Args>
        [[nodiscard]] bool TryCallVoid(Args&&... args) const noexcept
        {
            const auto arguments = PackArguments(std::forward<Args>(args)...);
            return Invoke(arguments);
        }

        [[nodiscard]] bool Invoke(
            std::span<const std::uint64_t> arguments,
            void* returnValue = nullptr,
            std::size_t returnSize = 0) const noexcept;

        void Invalidate() const noexcept;
        [[nodiscard]] bool Resolved() const noexcept;
        [[nodiscard]] std::optional<std::uint32_t> ProgramCounter() const noexcept;
        [[nodiscard]] ScriptHash Script() const noexcept;
        [[nodiscard]] std::string_view Name() const noexcept;

    private:
        template <typename Arg>
        [[nodiscard]] static std::uint64_t PackArgument(Arg&& value) noexcept
        {
            using Raw = std::decay_t<Arg>;
            static_assert(std::is_trivially_copyable_v<Raw>);
            static_assert(sizeof(Raw) <= sizeof(std::uint64_t));

            Raw copy = std::forward<Arg>(value);
            std::uint64_t slot{};
            std::memcpy(&slot, &copy, sizeof(copy));
            return slot;
        }

        template <typename... Args>
        [[nodiscard]] static std::array<std::uint64_t, sizeof...(Args)> PackArguments(Args&&... args) noexcept
        {
            return {PackArgument(std::forward<Args>(args))...};
        }

        [[nodiscard]] std::optional<std::uint32_t> ResolveProgramCounter() const noexcept;

        ScriptHash m_Script{};
        ScriptPointer m_Pointer;

        mutable std::mutex m_Mutex;
        mutable std::optional<std::uint32_t> m_ProgramCounter;
        mutable std::uint64_t m_Generation{};
    };
}
