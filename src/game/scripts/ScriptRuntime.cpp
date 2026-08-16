#include "ScriptRuntime.hpp"

namespace Sick::Game::Scripts
{
    ScriptRuntime& ScriptRuntime::Get() noexcept
    {
        static ScriptRuntime runtime;
        return runtime;
    }

    bool ScriptRuntime::Configure(ProgramResolverFn programResolver, InvokeFn invoker) noexcept
    {
        const bool valid = programResolver != nullptr && invoker != nullptr;

        {
            std::scoped_lock lock(m_Mutex);
            m_ProgramResolver = valid ? programResolver : nullptr;
            m_Invoker = valid ? invoker : nullptr;
        }

        m_Generation.fetch_add(1, std::memory_order_release);
        return valid;
    }

    void ScriptRuntime::Reset() noexcept
    {
        {
            std::scoped_lock lock(m_Mutex);
            m_ProgramResolver = nullptr;
            m_Invoker = nullptr;
        }

        m_Generation.fetch_add(1, std::memory_order_release);
    }

    bool ScriptRuntime::Ready() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_ProgramResolver != nullptr && m_Invoker != nullptr;
    }

    ScriptProgramView ScriptRuntime::ResolveProgram(ScriptHash script) const noexcept
    {
        ProgramResolverFn resolver{};

        {
            std::scoped_lock lock(m_Mutex);
            resolver = m_ProgramResolver;
        }

        return resolver ? resolver(script) : ScriptProgramView{};
    }

    bool ScriptRuntime::Invoke(
        ScriptHash script,
        std::uint32_t programCounter,
        const std::uint64_t* arguments,
        std::size_t argumentCount,
        void* returnValue,
        std::size_t returnSize) const noexcept
    {
        if ((argumentCount != 0 && arguments == nullptr) ||
            (returnSize != 0 && returnValue == nullptr))
        {
            return false;
        }

        InvokeFn invoker{};

        {
            std::scoped_lock lock(m_Mutex);
            invoker = m_Invoker;
        }

        return invoker && invoker(
            script,
            programCounter,
            arguments,
            argumentCount,
            returnValue,
            returnSize);
    }

    std::uint64_t ScriptRuntime::Generation() const noexcept
    {
        return m_Generation.load(std::memory_order_acquire);
    }
}
