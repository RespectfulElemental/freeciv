# Changelog

Changes this fork makes to Freeciv 3.2 (upstream `S3_2`). Newest first.
Unless noted, game changes are in the **gtk4 client**.

## 2026-11-10

### Added
- **Mouse controls** presets (Game → Options → Local Client → Interface):
  - **Classic**: Freeciv's own controls, as before.
  - **Modern** (the default): left-drag scrolls the map, right-click sends
    the selected units to a tile, right-drag selects an area. Right-click
    no longer centers the map; in go-to mode it cancels, like Esc.
  - **Command**: left chooses and views, right gives orders. Left-drag
    scrolls, Ctrl+left-drag selects an area. Hold or drag the right button
    to start a go-to and let go on the destination; a left-click cancels.
    Middle-click centers the map there and shows tile info.
  - In Modern and Command, left-clicks act when you let go.

## 2026-11-09

### Added
- **Load Game** in the Game menu (Ctrl+L). Works from inside a running game.
- **Quicksave** (F5) and **Quickload** (F9).
- A loaded game starts right away instead of stopping in the pregame lobby,
  if the save has a player with your username. Tick **Load to lobby** on the
  load page for the old behaviour.
- Clicking a city's name label on the map acts like clicking the city.

### Changed
- Save Game As: Ctrl+Shift+S.
- Report keys moved to free F5 and F9: Economy F6, Research F7, Wonders F8,
  Messages F11, Demographics F12, Top Cities Shift+F11, Spaceship Shift+F12.
- Connect With Rail: Ctrl+L → Ctrl+Alt+L.
- In-game help lists the new keys (shared by all clients).

### Fixed
- Windows: the game no longer fails to start with "missing DLL" errors
  (libwavpack-1.dll, libgstallocators-1.0-0.dll and others).

### Project
- Windows test builds: pushing a `test-*` tag builds the gtk4 client on
  GitHub and publishes an installer and a `.zip` on the Releases page.
- The Windows build now ships every DLL the programs need, found by reading
  their imports (`windows-bundle-dlls.sh`), and CI checks that the programs
  start. Image loaders for formats Freeciv doesn't use (HEIF, AVIF, JPEG XL,
  WMF) are left out, which keeps about 60 MB of codecs out of the download.

## 2026-11-08

### Project
- Forked from upstream 3.2.7 and moved the source tree into `src/`, so
  upstream fixes still merge in.
- Added `run-client.sh` to start the client from the build directory.
