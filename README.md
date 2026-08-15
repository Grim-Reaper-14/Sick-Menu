# Sick-Menu

Sick Menu targets **Grand Theft Auto V Enhanced** and is being structured as a layered C++ backend rather than putting gameplay logic directly in the menu renderer.

## Backend layout

```text
UI / Lua
   |
Features
   |
Services
   |
Typed Natives
   |
NativeInvoker
   |
NativeResolver + diagnostics + registry
   |
Enhanced NativeTable
   |
GTA V Enhanced host adapter
```

### Enhanced adapter

`src/game/enhanced/` owns edition/build-specific integration:

- `BuildManager` stores the Enhanced build ID supplied by the host.
- `NativeTable` owns the Enhanced native lookup callback and optional build-aware hash mapper.
- `EnhancedGame` coordinates startup, shutdown, and game-thread scheduler ticks.

The low-level lookup implementation is intentionally injected by the in-process host. This keeps signatures, build mappings, and native-table discovery out of feature code.

### Native subsystem

`src/game/natives/` contains:

- ABI-shaped `NativeCallContext` and isolated argument/return storage.
- Typed `NativeInvoker` calls.
- Thread-safe `NativeResolver` handler cache and overrides.
- `NativeRegistry` metadata lookup by hash or name.
- `NativeDiagnostics` call success/failure counters.
- `NativeSystem` lifecycle management.
- Typed wrappers in `Natives.hpp`, designed to be generator-friendly.

Example feature-side usage:

```cpp
#include "game/services/PlayerService.hpp"

Sick::Game::PlayerService player;
player.SetInvincible(true);
```

### Scheduler

`GameScheduler` queues gameplay jobs separately from UI/render callbacks. The in-process Enhanced host calls `EnhancedGame::Tick()` from the appropriate game execution context.

```cpp
Sick::Game::GameScheduler::Get().Queue([]
{
    Sick::Game::PlayerService{}.SetInvincible(true);
});
```

## Host integration boundary

The host supplies a build ID and native lookup callback:

```cpp
using namespace Sick::Game;
using namespace Sick::Game::Enhanced;
using namespace Sick::Game::Natives;

NativeHandler LookupEnhancedNative(NativeHash hash, BuildId build)
{
    // Resolve through Sick's GTA V Enhanced native-table adapter.
    return nullptr;
}

bool StartBackend(BuildId build)
{
    return EnhancedGame::Initialize(build, &LookupEnhancedNative);
}
```

An optional hash-mapper callback can be supplied when a particular Enhanced build needs remapping without changing the public wrappers.

## Tests

```text
cmake -S . -B build -DSICK_NATIVE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Tests use mock native handlers and do not require GTA to be running.

This project targets local/single-player mod development. Do not use it to bypass multiplayer protections or interfere with other players.
