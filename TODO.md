# TODO

## Resume here

2026-11-10: **Batch 4** (mouse control presets: Classic / Modern / Command) is written but not yet built: build it in the distrobox and run the test list under Blocked. Then **Batch 5** (keybinding menu) under Active. The roadmap came from `INITIAL-GOALS.md`; its decisions are listed at the top of Active. Target is the **gtk4 client**. Build and run it on the host in the `freeciv-dev` distrobox: `distrobox enter freeciv-dev -- bash -c 'cd ~/projects/mods/workshop-garage/freeciv && ninja -C build'`. Run the client with `./run-client.sh`, from the host or inside the box. The sandbox has no GTK4, so it can only syntax-check client code (against a stub `gtk.h`, using `build/`'s generated headers read-only); it never writes to `build/`.

## Active

**Roadmap (2026-11-09).** Easy wins come first. Each batch is one build-and-test cycle in the distrobox, because the client can't be built in the agent sandbox. Paths are under `src/client/gui-gtk-4.0/` unless noted. To keep upstream merges clean, put larger new code in new files and keep edits to upstream files small.

Decisions already made (2026-11-09):
- F-keys: F1–F4 stay; Economy F6, Research F7, Wonders F8, Messages F11, Demographics F12, Top Cities Shift+F11, Spaceship Shift+F12. F5 = quicksave, F9 = quickload, F10 left alone (GTK menubar).
- Connect With Rail moves from Ctrl+L to Ctrl+Alt+L, freeing Ctrl+L for Load Game. The user confirmed on 2026-11-09 that the desktop doesn't take Ctrl+Alt+L.
- Compression: zstd (`.sav.zst`, the build default) is fine. No `.zip` work.
- Mouse: a new client option "Modern mouse controls", **on** by default. Left-drag pans, right-click moves the selected unit, right-drag still draws the selection box, and right-click no longer recenters. (2026-11-10: became the "Mouse controls" preset option — Classic / Modern / Command — with Modern the default.)

### Batch 5: keybinding menu (largest)
- [ ] A dialog that lists every `menu_entries[]` action with its current accelerator, captures a new key, warns on conflicts, and applies the change with `gtk_application_set_accels_for_action`. It also lets you give a key to entries that have none. Save overrides by action name in the client options file. Respect `MGROUP_CHAR` (keys that must not fire while a text entry has focus). Put it in a new file, for example `keybindings.c`. It changes only menu actions; the hard-coded map keys in `gui_main.c` are out of scope for now. Done when: you rebind Save As, restart, and the new key still works.

## Blocked / waiting on human

### Batch 4: build and test mouse control presets (2026-11-10)
Code: new `src/client/gui-gtk-4.0/modernmouse.c` (all the logic; the comment at its top describes each preset), hooks in `mapctrl.c` (left/right press and release, motion, middle-click), option `gui_gtk4_mouse_controls` (Classic / Modern / Command, default Modern; enum `GUI_GTK_MOUSE_*` in `client/options.h`), help text in `data/helpdata.txt`. Syntax-checked only. Command decisions (2026-11-10): middle-click centers on the mouse and shows tile info; `c` keeps centering on the selected unit; Ctrl+Shift+left click = adjust workers, drag = append to selection; plain right-click gives no order (clears city highlights); right hold 0.25 s or drag starts a go-to, release on the units' own tile does nothing; in keyboard order modes right confirms and any left (except Alt+left) cancels; left-drag never does a go-to; other modifier clicks unchanged.
1. [ ] Build: `distrobox enter freeciv-dev -- bash -c 'cd ~/projects/mods/workshop-garage/freeciv && ninja -C build'`. If it fails, paste the first error.
2. [ ] Start a game with `./run-client.sh`. The preset is under Game → Options → Local Client → Interface → "Mouse controls". **Modern** (default):
   - a. Left-drag on empty land, and from a city: the map scrolls; no city dialog opens.
   - b. Left-click a unit, a city, and a city's name label: they select/open as before (on release).
   - c. Select a unit, left-click an empty tile, then just move the mouse: **no** go-to line appears.
   - d. Left-drag starting on the selected unit (alone, outside a city): it's a go-to (line follows, unit moves on release).
   - d2. Select a unit inside a city, left-drag from it: the map scrolls. A plain click there still opens the city.
   - e. Right-click a tile with a unit selected: the unit goes there. Right-click with nothing selected: nothing happens (no recentering).
   - f. Right-drag: the selection box still works. A tiny right-click wobble still counts as a move.
   - g. Press `g` then right-click: go-to mode is cancelled. Press `g` then left-click: go-to as before.
   - h. Shift/Ctrl/Alt clicks behave as before (e.g. Ctrl+left quickselect, Shift+right copy production).
3. [ ] Switch to **Command**:
   - j. Left-click selects/opens (on release); left-drag scrolls, also from the selected unit.
   - k. Plain right-click with a unit selected: nothing moves.
   - k2. A deliberately *slow* right-click (hold ~half a second, don't move) on another tile: this counts as a hold, so the unit goes there. Judge whether 0.25 s is too short; the knob is `RIGHT_HOLD_MS` in `modernmouse.c`.
   - l. Right-drag from anywhere: a go-to line follows the mouse; release sends the unit there. Same with holding right still for a moment, then moving.
   - m. While right-dragging, left-click: the order is cancelled; releasing right then does nothing.
   - n. Right-drag and release back on the unit's own tile: nothing happens, goto mode ends.
   - o. Ctrl+left-click: quickselect a sea unit. Ctrl+left-drag: selection box. Ctrl+Shift+left-drag: adds to the selection. Ctrl+Shift+left-click near a city: adjusts a worker.
   - p. Press `g`, then right-click a tile: the unit goes there. Press `g`, then left-click: cancelled.
   - q. Middle-click a tile: the map centers there and the tile info popup appears on it. Ctrl+middle still wakes sentries. `c` still centers on the selected unit.
4. [ ] Switch to **Classic**: right-click recenters, left-drag from a unit does a go-to, middle-click shows tile info without centering.
5. [ ] Report which letters failed (if any).

Done when: everything passes. Next: Batch 5.

## Done (recent)

- 2026-11-10 — Windows test build `test-2026-11-09.2` works: the user reported the tester ran it and liked the changes. (Whether F5/F9/Ctrl+L load was exercised on Windows wasn't reported.)
- 2026-11-09 — Batches 1–3 built and passed the user's 9-step test in the distrobox. Batch 1: Save As Ctrl+Shift+S; reports F6 Economy, F7 Research, F8 Wonders, F11 Messages, F12 Demographics, Shift+F11 Top Cities, Shift+F12 Spaceship; Connect With Rail Ctrl+Alt+L; `data/helpdata.txt` updated (shared by all clients). Batches 2–3: new `src/client/gui-gtk-4.0/gameload.c` with Load Game (Ctrl+L), quicksave F5 (`/save quicksave`), quickload F9, and lobby skip after load (it arms only if the save has a player with our username; otherwise it stays in the lobby). A "Load to lobby" checkbox keeps the old flow. City-name click: label size kept in `pcity->client.label_width/height` (`common/city.h`), hit-test `city_label_at()` in `mapctrl.c`. The label wins over a unit below it; the user is fine with that.
- 2026-11-09 — Closed: empty Save-dialog list on Windows. It was a Qt-client problem and doesn't reproduce in gtk4, so it's out of scope for now.
- 2026-11-08 — Moved the GitHub repo to a standalone (non-fork) repository, `RespectfulElemental/freeciv`.
- 2026-11-08 — First full build succeeded in `freeciv-dev`. Meson had to be added on top of `dnf builddep freeciv`. The suggested command was `dnf install meson ninja-build gtk4-devel SDL2_mixer-devel`; whether all four were needed wasn't checked. `build/` contains `freeciv-gtk4`, `freeciv-server`, `freeciv-mp-gtk4`, `freeciv-ruledit`, `freeciv-ruleup` and `freeciv-manual`.
- 2026-11-08 — Decision: move the GitHub repo to a standalone (non-fork) repository instead of staying a fork of freeciv/freeciv. Side effect: GitHub's "Contribute" button and gh's default PR target stop pointing at upstream.
- 2026-11-08 — Added local guards against PRs or pushes to upstream. `upstream` still fetches, for `git merge -X subtree=src upstream/S3_2`, but its push URL is set to `DISABLED--fork-is-decoupled-from-upstream`, so `git push upstream` fails. `remote.origin.gh-resolved=base` pins gh's default repo to `RespectfulElemental/freeciv`. To undo: `git remote set-url --push upstream https://github.com/freeciv/freeciv` and `git config --unset remote.origin.gh-resolved`.
- 2026-11-08 — Toolchain: meson and ninja are installed via Homebrew on the host. The `freeciv-dev` distrobox, with `dnf builddep freeciv`, is set up for gtk4 client builds.
- 2026-11-08 — Reset `main` to upstream stable `S3_2` (3.2.x, at `87d501f326`) and moved the tree into `src/` as a pure-rename commit (6721 files, all R100). `.github` went into `src/` too, so CI is inert. Added a root README/COPYING/.gitignore. The `git merge -X subtree=src upstream/S3_2` recipe was tested: a simulated 28-commit upstream merge came out clean and `src/` matched upstream blob-for-blob. `meson setup <build> src` parsed fine but stopped on sqlite3 missing in the sandbox, so a full build is still unverified.
