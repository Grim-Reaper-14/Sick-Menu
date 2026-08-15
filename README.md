# Sick-Menu

Sick Menu is being built around a small, typed GTA native backend with its own call context, handler cache, resolver abstraction, and generated-wrapper-friendly API.

## Native backend

Core files live under `src/game/natives/`:

- `NativeContext.hpp` - ABI-shaped call context, argument/return storage, and vector fix-up support.
- `NativeResolver.*` - thread-safe handler resolution, caching, and per-native overrides.
- `NativeInvoker.hpp` - typed `Call`, `TryCall`, and availability checks.
- `Natives.hpp` - typed namespace wrappers. This starts with a few verified natives and is intended to be expanded/generated.
- `NativeBackend.hpp` - convenience include for the whole native layer.

The host/in-process game adapter supplies the actual handler resolver:

```cpp
#include "game/natives/NativeBackend.hpp"

using namespace Sick::Game;
using namespace Sick::Game::Natives;

NativeHandler ResolveFromGame(NativeHash hash)
{
    // Wire this to Sick's GTA-version-specific native table adapter.
    return nullptr;
}

void InitializeNativeBackend()
{
    NativeResolver::Get().SetResolver(&ResolveFromGame);
}
```

Feature code stays clean and typed:

```cpp
const auto ped = PLAYER::PLAYER_PED_ID();
ENTITY::SET_ENTITY_INVINCIBLE(ped, true);
```

The resolver boundary is intentional: GTA build/version lookup belongs in a separate adapter so the public Sick native API does not need to change when the game updates.

## Tests

```text
cmake -S . -B build -DSICK_NATIVE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

The native backend tests use mock handlers and do not require GTA to be running.

This project targets local/single-player mod development. Do not use it to bypass multiplayer protections or interfere with other players.
