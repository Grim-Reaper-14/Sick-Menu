#pragma once

#include <cstdint>

namespace Sick::Core::Logging
{
    enum class EventId : std::uint64_t
    {
        None = 0,
        LoggerStarted,
        LoggerStopped,
        QueueOverflow,
        NativeBootstrapCompleted,
        NativeBootstrapPartial,
        NativeCallFailure,
        NativeArgumentOverflow,
        ScriptGlobalResolutionFailure,
        SchedulerSlowJob,
        SchedulerJobException,
        HealthStall,
        HealthRecovered,
        TraceBudgetExceeded,
        CrashCaptured,
        AssertionFailed
    };
}
