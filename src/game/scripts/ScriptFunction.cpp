#include "ScriptFunction.hpp"

#include <utility>

namespace Sick::Game::Scripts
{
    ScriptFunction::ScriptFunction(ScriptHash script, ScriptPointer pointer)
        : m_Script(script),
          m_Pointer(std::move(pointer))
    {
    }

    bool ScriptFunction::Invoke(
        std::span<const std::uint64_t> arguments,
        void* returnValue,
        std::size_t returnSize) const noexcept
    {
        const auto programCounter = ResolveProgramCounter();
        if (!programCounter)
            return false;

        return ScriptRuntime::Get().Invoke(
            m_Script,
            *programCounter,
            arguments.data(),
            arguments.size(),
            returnValue,
            returnSize);
    }

    void ScriptFunction::Invalidate() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_ProgramCounter.reset();
        m_Generation = 0;
    }

    bool ScriptFunction::Resolved() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_ProgramCounter.has_value() &&
            m_Generation == ScriptRuntime::Get().Generation();
    }

    std::optional<std::uint32_t> ScriptFunction::ProgramCounter() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        if (m_Generation != ScriptRuntime::Get().Generation())
            return std::nullopt;

        return m_ProgramCounter;
    }

    ScriptHash ScriptFunction::Script() const noexcept
    {
        return m_Script;
    }

    std::string_view ScriptFunction::Name() const noexcept
    {
        return m_Pointer.Name();
    }

    std::optional<std::uint32_t> ScriptFunction::ResolveProgramCounter() const noexcept
    {
        auto& runtime = ScriptRuntime::Get();

        for (int attempt = 0; attempt < 2; ++attempt)
        {
            const auto generation = runtime.Generation();

            {
                std::scoped_lock lock(m_Mutex);
                if (m_Generation == generation && m_ProgramCounter)
                    return m_ProgramCounter;
            }

            if (!runtime.Ready() || !m_Pointer.Valid())
                return std::nullopt;

            const auto program = runtime.ResolveProgram(m_Script);
            const auto resolved = m_Pointer.Resolve(program);
            if (!resolved)
                return std::nullopt;

            if (generation != runtime.Generation())
                continue;

            {
                std::scoped_lock lock(m_Mutex);
                m_ProgramCounter = resolved;
                m_Generation = generation;
            }

            return resolved;
        }

        return std::nullopt;
    }
}
