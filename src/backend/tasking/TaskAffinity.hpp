#pragma once

#include <cstdint>

namespace Sick::Backend::Tasking
{
    enum class TaskAffinity : std::uint8_t
    {
        Unspecified,
        Game,
        Background,
        Render,
    };

    inline thread_local TaskAffinity g_CurrentTaskAffinity{TaskAffinity::Unspecified};

    [[nodiscard]] inline TaskAffinity CurrentTaskAffinity() noexcept
    {
        return g_CurrentTaskAffinity;
    }

    [[nodiscard]] inline bool HasTaskAffinity(TaskAffinity affinity) noexcept
    {
        return CurrentTaskAffinity() == affinity;
    }

    class ScopedTaskAffinity final
    {
    public:
        explicit ScopedTaskAffinity(TaskAffinity affinity) noexcept
            : m_Previous(g_CurrentTaskAffinity)
        {
            g_CurrentTaskAffinity = affinity;
        }

        ~ScopedTaskAffinity()
        {
            g_CurrentTaskAffinity = m_Previous;
        }

        ScopedTaskAffinity(const ScopedTaskAffinity&) = delete;
        ScopedTaskAffinity& operator=(const ScopedTaskAffinity&) = delete;

    private:
        TaskAffinity m_Previous{TaskAffinity::Unspecified};
    };
}
