# Feature layout

Feature code is organized by the matching Sick Menu root category.

- `player/`
- `vehicle/`
- `weapons/`
- `world/`
- `teleport/`
- `tunables/`
- `unlocks/`
- `online_services/`
- `online_vehicle_spawner/`
- `online_protection/`
- `menu_settings/`

Within a category, each selectable gameplay feature gets its own focused file (for example `player/GodMode.hpp`). A category coordinator such as `player/PlayerFeatures.cpp` is reserved for lifecycle handling, snapshots, and interactions between features.

Keep GTA/native calls behind `src/game/services`; frontend/menu code must not call GTA directly. Persistent features are ticked by `FeatureManager` on the game thread before arbitrary queued game calls.
