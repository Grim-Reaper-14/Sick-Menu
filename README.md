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

The backend now uses the same live call path as YimMenuV2-style script
functions: identify a loaded script by its JOAAT hash, scan its bytecode for a
signature, resolve the function program counter, switch the active GTA script
TLS context, and execute it through the Enhanced script VM.

```cpp
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

Normal `Reaper::Enhanced::Game` initialization now attempts to locate the live
script-thread array, script-program table, script-global table, and script VM in
`GTA5_Enhanced.exe`. The resolver then finds the matching loaded program and
thread, validates stack capacity, installs the thread in TLS, invokes the VM,
restores the previous TLS state, and copies the return value. An existing
pointer manager can instead call `Game::BindScriptHost()` with resolved
addresses; `BindScriptRuntime()` remains available for a custom adapter.

`Reaper::ScriptFunctions` also exposes the concrete function definitions used
by the current Enhanced [YimMenuV2](https://github.com/YimMenu/YimMenuV2) call
sites. For example:

```cpp
const auto* spec = Reaper::ScriptFunctions::Find(
    Reaper::KnownScriptFunction::GetWeaponKills);
static auto getWeaponKills = spec->Bind();

const auto kills = getWeaponKills.TryCall<int>(weaponHash, -1);
```

The catalog is explicitly labeled for GTA Online 1.73 / Enhanced build
1158.13 and records the exact decompiled-scripts and YimMenuV2 reference
commits. Functions fail closed when their target script, thread, signature, or
VM binding is unavailable. Calls must run on the game/scheduler thread.

Script signatures are build-sensitive. The
[GTA V Enhanced decompiled scripts](https://github.com/acidlabsdev/gtav-enhanced-scripts)
are the source reference for reviewing the target functions after game
updates; the decompiled files themselves are not compiled, executed, or
vendored by this project.

## Menu UI

`Reaper::UI::SickMenu` implements the compact menu shown in the project
reference: navy branded header, black page bar and counter, dark option rows,
white selection, magenta toggle indicators, right-aligned values, and footer
chevrons. It includes wrapping keyboard/controller-style navigation and the
action, toggle, integer, choice, label, and submenu option types.

The core renderer emits dependency-free draw commands. A host that already has
a Dear ImGui frame can submit those commands and poll the default Insert,
arrow, Enter, and Backspace bindings with one call:

```cpp
#include "Reaper.hpp"
#include "ui/menu/ImGuiMenuBackend.hpp"

Reaper::UI::SickMenu menu{
    Reaper::UI::SickMenuCallbacks{
        .regularAction = [] {
            // Bind and call the appropriate Reaper::ScriptFunction here.
            // This callback is already running on the game scheduler thread.
        }
    }};

void DrawImGuiFrame()
{
    Sick::Ui::ImGuiMenuBackend::Render(menu);
}

void GameThreadTick()
{
    Reaper::Scheduler::Get().Tick();
}
```

The supplied Self page matches the reference layout and exposes callbacks for
each feature row. GodMode is connected to `PlayerService::SetInvincible` by
default; other feature callbacks can invoke typed natives or the existing
script-function catalog. Every feature callback is queued onto `GameScheduler`
instead of executing a script VM call from the render thread. A host-provided
header texture replaces the fallback Sick Menu mark through
`SickMenu::SetHeaderTexture()`.

## Backend components

- `src/game/enhanced/` - Enhanced build integration, indexed bootstrap, script-global facade, and lifecycle.
- `src/game/natives/` - call context, invoker, handler table, resolver, registry, diagnostics, and generated native IDs/hashes.
- `src/game/scripts/` - JOAAT hashing, bytecode patterns, script pointers, runtime binding, and typed script-function calls.
- `src/game/scheduler/` - gameplay job queue separated from UI/render callbacks.
- `src/game/services/` - higher-level gameplay services built on typed natives.
- `src/ui/menu/` - menu state, navigation, reference theme, draw commands, and optional ImGui adapter.
- `src/Reaper.hpp` - public API facade.

## Tests

```text
cmake -S . -B build -DSICK_NATIVE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The tests use mock native handlers/providers and do not require GTA to be running.

This project targets local/single-player mod development. Do not use it to bypass multiplayer protections or interfere with other players.
