# Vehicle handling

Handling editor coordination lives here. Field definitions and safe ranges are centralized in `shared/HandlingTypes.hpp`; live game access is routed through the build-gated `game/handling/HandlingBackend`.
