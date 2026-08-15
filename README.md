# Sick-Menu

Sick Menu targets **Grand Theft Auto V Enhanced** and exposes its gameplay API through the public `Reaper` namespace.

## Reaper native backend

Feature code stays small and typed:

```cpp
#include "Reaper.hpp"

const Reaper::Ped ped = Reaper::PLAYER::PLAYER_PED_ID();
Reaper::ENTITY::SET_ENTITY_INVINCIBLE(ped, true);
```

The preferred Enhanced path preloads native handlers once and then invokes them by generated `NativeIndex`:

```text
UI / Lua / Features
        |
        v
Reaper::PLAYER / ENTITY / ...
        |
        v
NativeInvoker::Invoke<NativeIndex>()
        |
        v
NativeHandlerTable[index]
        |
        v
Enhanced native handler
```

`NativeBootstrap` receives a host-provided native provider and warms the indexed handler table during startup. The menu does not perform a hash lookup for every normal typed native call. A raw hash resolver remains available for diagnostics and developer tooling.

```cpp
Reaper::Native::Handler ResolveNative(
    Reaper::NativeHash hash,
    Reaper::Enhanced::BuildId build)
{
    // Supply this from the allowed GTA V Enhanced host/runtime adapter.
    return nullptr;
}

bool StartBackend(Reaper::Enhanced::BuildId build)
{
    return Reaper::Enhanced::Game::InitializeIndexed(
        build,
        &ResolveNative
    );
}
```

The generated-native layer currently starts with a small verified set and is structured for a generator to expand `NativeIndex.hpp`, `NativeHashes.hpp`, metadata, and typed namespace wrappers together.

## Reaper observability

The backend includes an asynchronous structured logging and diagnostics pipeline rather than a simple console logger.

```cpp
Reaper::Log::LoggerConfig logging;
logging.minimumLevel = Reaper::Log::Level::Debug;
logging.filePath = "logs/reaper.log";
logging.rotateBytes = 16 * 1024 * 1024;
logging.rotateFiles = 6;

Reaper::Log::Initialize(logging);
Reaper::Crash::Install();
Reaper::Health::Start();

REAPER_INFO("Enhanced", "Backend started on build {}", build);
REAPER_WARN_EVERY(std::chrono::seconds{5}, "Native", "Native provider is degraded");
```

The logger records sequence numbers, wall/monotonic time, severity, category, event ID, thread identity/name, correlation/span IDs, source location, messages, and structured fields. Producers write into a bounded asynchronous queue; file/console/debugger I/O is handled away from normal gameplay work. Lower-priority records are discarded first under pressure, error records are preserved preferentially, critical records use an emergency synchronous path, and queue drops are exposed through logger statistics.

Built-in diagnostics include:

- rotating text or JSONL file sinks, console sink, Windows debugger sink, memory/ring sink, and an always-available recent-event buffer;
- thread-local structured context and correlation IDs;
- rate limiting (`REAPER_WARN_EVERY` / `REAPER_WARN_ONCE`);
- burst/anomaly detection for repeated warning/error events, tagged directly on structured records;
- RAII tracing (`REAPER_TRACE_SCOPE` / `REAPER_WATCH_SCOPE`) with execution-budget warnings;
- counters, gauges, and distribution metrics;
- subsystem heartbeat monitoring with stall/recovery events;
- scheduler timing, slow-job detection, and scheduled-job exception reporting;
- native call failure / argument-overflow reporting with rate limiting;
- native bootstrap completeness metrics and events;
- script-global resolution failure detection;
- assertion/verification diagnostics with stack capture (`REAPER_ASSERT` / `REAPER_VERIFY`);
- crash reports containing logger state, recent events, metrics, and a raw stack trace;
- Windows minidump output through DbgHelp when crash reporting is installed.

Structured context follows work automatically on the current thread:

```cpp
Reaper::CorrelationScope request{42};
Reaper::LogContext context{
    Reaper::Log::MakeField("feature", "Teleport"),
    Reaper::Log::MakeField("script", "teleport.lua")
};

REAPER_ERROR("Teleport", "Destination validation failed");
```

Long-running work can be watched without manually writing timers:

```cpp
{
    REAPER_WATCH_SCOPE("Streaming", "LoadModel", std::chrono::milliseconds{50});
    // work
}
```

Crash reporting is intentionally explicit so the host controls when process-level handlers are installed:

```cpp
Reaper::Crash::Install({
    .directory = "logs/crashes",
    .recentEvents = 256,
    .writeMiniDump = true
});
```

## Script globals

`Reaper::ScriptGlobal` provides `At()` traversal and typed `As<T>()` access while keeping address resolution behind a host callback:

```cpp
Reaper::Enhanced::Game::BindScriptGlobalResolver(&ResolveScriptGlobal);

auto value = Reaper::ScriptGlobal(123456)
    .At(5)
    .As<std::int64_t&>();
```

This keeps game-version-specific global resolution outside feature code. Failed global resolution is also surfaced through the observability pipeline when logging is enabled.

## Backend components

- `src/core/logging/` - asynchronous logger, structured records, anomaly detection, context, rate limiting, and sinks.
- `src/core/diagnostics/` - assertions, tracing, metrics, health monitoring, stack capture, and crash reporting.
- `src/game/enhanced/` - Enhanced build integration, indexed bootstrap, script-global facade, and lifecycle.
- `src/game/natives/` - call context, invoker, handler table, resolver, registry, diagnostics, and generated native IDs/hashes.
- `src/game/scheduler/` - gameplay job queue with timing and exception diagnostics.
- `src/game/services/` - higher-level gameplay services built on typed natives.
- `src/Reaper.hpp` - public API facade.

## Tests

```text
cmake -S . -B build -DSICK_NATIVE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The tests use mock native handlers/providers and do not require GTA to be running.

This project targets local/single-player mod development. Do not use it to bypass multiplayer protections or interfere with other players.
