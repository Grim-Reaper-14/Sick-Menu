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

## Script globals

`Reaper::ScriptGlobal` provides `At()` traversal and typed `As<T>()` access while keeping address resolution behind a host callback:

```cpp
Reaper::Enhanced::Game::BindScriptGlobalResolver(&ResolveScriptGlobal);

auto value = Reaper::ScriptGlobal(123456)
    .At(5)
    .As<std::int64_t&>();
```

This keeps game-version-specific global resolution outside feature code.

## Script functions

The backend now supports YimMenuV2-style script functions: identify a loaded
script by its JOAAT hash, scan its bytecode for a signature, optionally adjust
or dereference the match with `Add()`, `Sub()`, and `Rip()`, then invoke the
resolved program counter with typed arguments.

```cpp
Reaper::Enhanced::Game::BindScriptRuntime(
    &ResolveLoadedScript,
    &InvokeThroughScriptVm);

static Reaper::ScriptFunction function{
    Reaper::Joaat("freemode"),
    Reaper::ScriptPointer{"ExampleFunction", "5D ? ? ? 38 2A 71"}
        .Add(1)
        .Rip()};

if (const auto value = function.TryCall<int>(123, true))
{
    // Use *value.
}
```

`ResolveLoadedScript` returns a `Reaper::ScriptProgramView` for a live Enhanced
script program. `InvokeThroughScriptVm` is the host adapter that finds the live
thread, installs its TLS context, pushes the supplied 64-bit argument slots and
return slot, runs the script VM at the supplied program counter, restores TLS,
and copies the result. Keeping those callbacks outside this library prevents
unverified game offsets from being baked into the public backend.

Script signatures are build-sensitive. The
[GTA V Enhanced decompiled scripts](https://github.com/acidlabsdev/gtav-enhanced-scripts)
are useful for locating functions and reviewing behavior after game updates;
they are reference data and are not compiled or vendored by this project.
Resolve and invoke script functions only from the game/scheduler thread while
the target script is loaded.

## Backend components

- `src/game/enhanced/` - Enhanced build integration, indexed bootstrap, script-global facade, and lifecycle.
- `src/game/natives/` - call context, invoker, handler table, resolver, registry, diagnostics, and generated native IDs/hashes.
- `src/game/scripts/` - JOAAT hashing, bytecode patterns, script pointers, runtime binding, and typed script-function calls.
- `src/game/scheduler/` - gameplay job queue separated from UI/render callbacks.
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
