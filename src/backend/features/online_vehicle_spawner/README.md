# Online Vehicle Spawner features

Backend modules for the Online Vehicle Spawner menu belong here. Vehicle model/loading/spawn work should be split into focused feature files and executed through game-thread services.

- `VehicleSpawner` owns request gating, status, model-load timeout behavior and the cooperative game-thread spawn flow.
- `VehicleSpawnerService` owns the raw model streaming and vehicle creation native calls.
- The UI catalog is data-only and lives under `ui/menu/categories/online_vehicle_spawner`.
