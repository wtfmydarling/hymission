# AGENTS.md

## Hymission Plugin Reload

- Hyprpm uses this repository as a local source. Commit changes before updating.
- Preferred reload/update path for the current machine:
  1. Commit the intended changes in this repo.
  2. Run `hyprpm update`.
- Do not mix a hyprpm-managed hymission instance with manual `hyprctl plugin load` / `hyprctl plugin unload` for local build outputs in the same live session. Duplicate plugin instances can fight over hooks and destabilize Hyprland.
- Manual `hyprctl plugin load` / `hyprctl plugin unload` should only be used when explicitly requested for a non-hyprpm debugging session, and both commands require an absolute path.
- The active Hyprland config for this setup is in `~/.config/HyprV/hypr/hyprland-plugins.conf`, and it already contains the `plugin { hymission { ... } }` runtime config block.
