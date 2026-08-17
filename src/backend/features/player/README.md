# Player features

Player menu backend modules live here. Keep one gameplay operation per feature header (for example `GodMode.hpp` or `FastRun.hpp`) and keep shared/cross-feature coordination in `PlayerFeatures.cpp`.

GTA natives remain behind `game/services/PlayerService`; feature modules must not invoke the native invoker directly.
