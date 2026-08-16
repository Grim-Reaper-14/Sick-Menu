#pragma once

#include "ScriptTypes.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace Sick::Game::Scripts
{
    class ScriptProgramView final
    {
    public:
        using CodeAddressFn = const std::uint8_t* (*)(const void* program, std::uint32_t index) noexcept;

        constexpr ScriptProgramView() noexcept = default;

        constexpr ScriptProgramView(
            const void* program,
            std::uint32_t codeSize,
            CodeAddressFn codeAddress) noexcept
            : m_Program(program),
              m_CodeSize(codeSize),
              m_CodeAddress(codeAddress)
        {
        }

        [[nodiscard]] constexpr bool Valid() const noexcept
        {
            return m_Program != nullptr && m_CodeSize != 0 && m_CodeAddress != nullptr;
        }

        [[nodiscard]] constexpr std::uint32_t CodeSize() const noexcept
        {
            return m_CodeSize;
        }

        [[nodiscard]] const std::uint8_t* CodeAddress(std::uint32_t index) const noexcept
        {
            if (!Valid() || index >= m_CodeSize)
                return nullptr;

            return m_CodeAddress(m_Program, index);
        }

        [[nodiscard]] constexpr const void* Handle() const noexcept
        {
            return m_Program;
        }

    private:
        const void* m_Program{};
        std::uint32_t m_CodeSize{};
        CodeAddressFn m_CodeAddress{};
    };

    class ScriptRuntime final
    {
    public:
        using ProgramResolverFn = ScriptProgramView (*)(ScriptHash script) noexcept;
        using InvokeFn = bool (*)(
            ScriptHash script,
            std::uint32_t programCounter,
            const std::uint64_t* arguments,
            std::size_t argumentCount,
            void* returnValue,
            std::size_t returnSize) noexcept;

        static ScriptRuntime& Get() noexcept;

        bool Configure(ProgramResolverFn programResolver, InvokeFn invoker) noexcept;
        void Reset() noexcept;

        [[nodiscard]] bool Ready() const noexcept;
        [[nodiscard]] ScriptProgramView ResolveProgram(ScriptHash script) const noexcept;
        [[nodiscard]] bool Invoke(
            ScriptHash script,
            std::uint32_t programCounter,
            const std::uint64_t* arguments,
            std::size_t argumentCount,
            void* returnValue,
            std::size_t returnSize) const noexcept;
        [[nodiscard]] std::uint64_t Generation() const noexcept;

    private:
        ScriptRuntime() = default;

        mutable std::mutex m_Mutex;
        ProgramResolverFn m_ProgramResolver{};
        InvokeFn m_Invoker{};
        std::atomic<std::uint64_t> m_Generation{1};
    };
}
