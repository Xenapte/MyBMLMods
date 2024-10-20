# CameraInversionFixer

The mod for fixing ingame camera inversions.

Ballance has a glitch where the camera can be inverted (upside-down) when it moves backwards too fast. This mod fixes that by flipping the camera back when it detects an inversion.

## Configs

- `Enabled` - Pretty self-explanatory, enables or disables the mod.
- `EnsureFix` - If enabled, the mod will always try to fix the camera inversion whenever the latter is detected. If disabled, you will have to fix the inversion manually by pressing the fix key (see below).
- `FixKey` - Key for fixing the camera inversion manually. Default `B`.
- `Debug_ToggleInversion` - **Debug settings**. If enabled, you can manually toggle the camera inversion by pressing the fix key.
