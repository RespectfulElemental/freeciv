# TODO

## Resume here

2026-11-09: Batch 1 (accelerator edits in `menu.c`) is written. Next: the user builds and checks it in the distrobox, then Batch 2. The roadmap came from `INITIAL-GOALS.md`; its decisions are listed at the top of Active. The empty save list happens only on Windows (see Blocked). Target is the **gtk4 client**. Build and run it on the host in the `freeciv-dev` distrobox (`dnf builddep freeciv` done 2026-11-08): `distrobox enter freeciv-dev -- bash -c 'cd ~/projects/mods/workshop-garage/freeciv && ninja -C build'`. The first full build succeeded on 2026-11-08. Run the client with `./run-client.sh`, from the host or inside the box. The sandbox uses its own `build-sandbox/` for server-only checks, so it never touches `build/`. The sandbox has meson and ninja from Homebrew but no GTK4, so it can only do server-only compile checks.

## Active

**Roadmap (2026-11-09).** Easy wins come first. Each batch is one build-and-test cycle in the distrobox, because the client can't be built in the agent sandbox. Paths are under `src/client/gui-gtk-4.0/` unless noted. To keep upstream merges clean, put larger new code in new files and keep edits to upstream files small.

Decisions already made (2026-11-09):
- F-keys: F1–F4 stay; Economy F6, Research F7, Wonders F8, Messages F11, Demographics F12, Top Cities Shift+F11, Spaceship Shift+F12. F5 = quicksave, F9 = quickload, F10 left alone (GTK menubar).
- Connect With Rail moves from Ctrl+L to Ctrl+Alt+L, freeing Ctrl+L for Load Game. The user confirmed on 2026-11-09 that the desktop doesn't take Ctrl+Alt+L.
- Compression: zstd (`.sav.zst`, the build default) is fine. No `.zip` work.
- Mouse: a new client option "Modern mouse controls", **on** by default. Left-drag pans, right-click moves the selected unit, right-drag still draws the selection box, and right-click no longer recenters.

### Batch 1: accelerator table edits (easy, `menu.c` data only)
- [ ] Save Game As → Ctrl+Shift+S. Edit `GAME_SAVE_AS` in `menu_entries[]` (`menu.c:581`). Ctrl+Shift+S is unused; plain Shift+S is "unsentry all", which is a different binding. Done when: the menu shows Ctrl+Shift+S and pressing it opens the Save dialog.
- [ ] Move the report F-keys to the agreed layout (`menu.c:858-899`). Done when: the Reports menu shows the new keys, and F5/F9 do nothing yet.
- [ ] Connect With Rail → Ctrl+Alt+L (`menu.c:827`). Done when: the Work menu shows Ctrl+Alt+L.

### Batch 2: quicksave and Load Game menu item
- [ ] Quicksave on F5: a new menu entry that sends `/save quicksave`. The server strips and re-adds the suffix (`server/savegame/savemain.c:189-230`), so the file is `~/.freeciv/saves/quicksave.sav.zst`. Only offer it with a local server and hack access. Done when: F5 writes that file and the chat says it was saved.
- [ ] Shared helper "restart the local server and load file X". The server refuses `/load` once a game is running (`server/stdinhand.c:3807`), so in-game loading must restart the local server. Don't use `client_start_server()`'s `--file` path (`client/connectdlg_common.c:344`): the server loads that file at startup (`server/srv_main.c:3110`), before any client connects, so the `game_load` packet (`server/stdinhand.c:3946`) reaches nobody and `handle_game_load()` never runs. Start a plain server instead, wait for the connection, then send `/load <file>`. Batch 2's two later items and Batch 3's quickload all use it.
- [ ] Game → "Load Game..." on Ctrl+L. In the pregame lobby it goes to `PAGE_LOAD`; in a game it confirms, then picks a file and calls the helper. Done when: Ctrl+L from inside a running game ends up in the loaded game.
- [ ] Single-player load skips the lobby. In `handle_game_load()` (`pages.c:2968`), send `/start` once our connection is attached to a player. If no player matches the username, stay in the lobby so the user can pick one. Add a "Load to lobby" checkbox on the load page for the old behaviour. Done when: double-clicking a save on the load page drops you straight into the game.

### Batch 3: quickload, city-name click
- [ ] Quickload on F9: load `quicksave.sav.zst` through the helper, after a confirmation step. Do nothing if no quicksave exists. Done when: F5, move a unit, F9 brings back the earlier position.
- [ ] Clicking a city name label on the map opens the city. The label is drawn *below* the city tile, so a tile hit-test misses it. Record label rectangles when labels are drawn (gtk4 `mapview.c` `show_city_desc`, or `client/mapview_common.c`, keeping the edit small) and check them first on a left click (`mapctrl.c:208`). Assumes "city name" means the map label. Done when: clicking the name text opens the city dialog.

### Batch 4: modern mouse controls
- [ ] Add the client option (`client/options.c`, gtk4 group). Conflict to handle: upstream keyboardless goto (always on; left-press on the focused unit plus a drag = goto, `client/mapctrl_common.c:508`, triggered from `mapctrl.c:443`) uses the same gesture. Rule: a drag that starts on the focused unit's tile stays a goto; any other drag pans. Left-click actions currently fire on *press* (`mapctrl.c:208`), so in modern mode they move to release. Left button: a drag threshold of about 5 px separates a click (select, as now) from a drag (pan with `set_mapview_scroll_pos`). Right button: a click with a focused unit sends it to the tile (goto); past the threshold it is the existing selection box (`mapctrl.c:263-325`, `client/mapctrl_common.c:322`). Done when: dragging pans without selecting, right-click moves the unit, and turning the option off restores the old behaviour.

### Batch 5: keybinding menu (largest)
- [ ] A dialog that lists every `menu_entries[]` action with its current accelerator, captures a new key, warns on conflicts, and applies the change with `gtk_application_set_accels_for_action`. It also lets you give a key to entries that have none. Save overrides by action name in the client options file. Respect `MGROUP_CHAR` (keys that must not fire while a text entry has focus). Put it in a new file, for example `keybindings.c`. It changes only menu actions; the hard-coded map keys in `gui_main.c` are out of scope for now. Done when: you rebind Save As, restart, and the new key still works.

## Blocked / waiting on human

- 2026-11-09 — **Empty save list in the Save dialog: Windows only.** On Linux, Save As and the Load page both list saves correctly (the user checked on 2026-11-09). The problem shows up only for a Windows player. Steps:
  1. Ask the Windows player which client they run (gtk4, gtk3, Qt or SDL) and its version (Help → About), and whether it's an official upstream installer or a build of this fork.
  2. Ask them to save once (Game → Save Game), then look for the newest `.sav*` file under `%USERPROFILE%\.freeciv\saves` and `%APPDATA%\freeciv`, and send the full path where it turns up.
  3. Ask whether **Load Saved Game** on their start screen lists that save.
  - Done when: the agent has those three answers. Next: compare where the Windows server writes saves (`client/connectdlg_common.c:311`, `freeciv_storage_dir()`) with what the dialog lists (`get_save_dirs()`, `DEFAULT_SAVE_PATH` in `utility/shared.c:73`).

## Done (recent)

- 2026-11-08 — Moved the GitHub repo to a standalone (non-fork) repository, `RespectfulElemental/freeciv`.
- 2026-11-08 — First full build succeeded in `freeciv-dev`. Meson had to be added on top of `dnf builddep freeciv`. The suggested command was `dnf install meson ninja-build gtk4-devel SDL2_mixer-devel`; whether all four were needed wasn't checked. `build/` contains `freeciv-gtk4`, `freeciv-server`, `freeciv-mp-gtk4`, `freeciv-ruledit`, `freeciv-ruleup` and `freeciv-manual`.
- 2026-11-08 — Decision: move the GitHub repo to a standalone (non-fork) repository instead of staying a fork of freeciv/freeciv. Side effect: GitHub's "Contribute" button and gh's default PR target stop pointing at upstream.
- 2026-11-08 — Added local guards against PRs or pushes to upstream. `upstream` still fetches, for `git merge -X subtree=src upstream/S3_2`, but its push URL is set to `DISABLED--fork-is-decoupled-from-upstream`, so `git push upstream` fails. `remote.origin.gh-resolved=base` pins gh's default repo to `RespectfulElemental/freeciv`. To undo: `git remote set-url --push upstream https://github.com/freeciv/freeciv` and `git config --unset remote.origin.gh-resolved`.
- 2026-11-08 — Toolchain: meson and ninja are installed via Homebrew on the host. The `freeciv-dev` distrobox, with `dnf builddep freeciv`, is set up for gtk4 client builds.
- 2026-11-08 — Reset `main` to upstream stable `S3_2` (3.2.x, at `87d501f326`) and moved the tree into `src/` as a pure-rename commit (6721 files, all R100). `.github` went into `src/` too, so CI is inert. Added a root README/COPYING/.gitignore. The `git merge -X subtree=src upstream/S3_2` recipe was tested: a simulated 28-commit upstream merge came out clean and `src/` matched upstream blob-for-blob. `meson setup <build> src` parsed fine but stopped on sqlite3 missing in the sandbox, so a full build is still unverified.
