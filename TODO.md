# TODO

## Resume here

2026-11-09: Batches 1–3 are built and tested. First, the **first Windows test build** under Blocked (it waits on a push from the host). Then **Batch 4** (modern mouse controls) under Active. The roadmap came from `INITIAL-GOALS.md`; its decisions are listed at the top of Active. Target is the **gtk4 client**. Build and run it on the host in the `freeciv-dev` distrobox (`dnf builddep freeciv` done 2026-11-08): `distrobox enter freeciv-dev -- bash -c 'cd ~/projects/mods/workshop-garage/freeciv && ninja -C build'`. The first full build succeeded on 2026-11-08. Run the client with `./run-client.sh`, from the host or inside the box. The sandbox uses its own `build-sandbox/` for server-only checks, so it never touches `build/`. The sandbox has meson and ninja from Homebrew but no GTK4, so it can only do server-only compile checks.

## Active

**Roadmap (2026-11-09).** Easy wins come first. Each batch is one build-and-test cycle in the distrobox, because the client can't be built in the agent sandbox. Paths are under `src/client/gui-gtk-4.0/` unless noted. To keep upstream merges clean, put larger new code in new files and keep edits to upstream files small.

Decisions already made (2026-11-09):
- F-keys: F1–F4 stay; Economy F6, Research F7, Wonders F8, Messages F11, Demographics F12, Top Cities Shift+F11, Spaceship Shift+F12. F5 = quicksave, F9 = quickload, F10 left alone (GTK menubar).
- Connect With Rail moves from Ctrl+L to Ctrl+Alt+L, freeing Ctrl+L for Load Game. The user confirmed on 2026-11-09 that the desktop doesn't take Ctrl+Alt+L.
- Compression: zstd (`.sav.zst`, the build default) is fine. No `.zip` work.
- Mouse: a new client option "Modern mouse controls", **on** by default. Left-drag pans, right-click moves the selected unit, right-drag still draws the selection box, and right-click no longer recenters.

### Batch 4: modern mouse controls
- [ ] Add the client option (`client/options.c`, gtk4 group). Conflict to handle: upstream keyboardless goto (always on; left-press on the focused unit plus a drag = goto, `client/mapctrl_common.c:508`, triggered from `mapctrl.c:443`) uses the same gesture. Rule: a drag that starts on the focused unit's tile stays a goto; any other drag pans. Left-click actions currently fire on *press* (`mapctrl.c:208`), so in modern mode they move to release. Left button: a drag threshold of about 5 px separates a click (select, as now) from a drag (pan with `set_mapview_scroll_pos`). Right button: a click with a focused unit sends it to the tile (goto); past the threshold it is the existing selection box (`mapctrl.c:263-325`, `client/mapctrl_common.c:322`). Done when: dragging pans without selecting, right-click moves the unit, and turning the option off restores the old behaviour.

### Batch 5: keybinding menu (largest)
- [ ] A dialog that lists every `menu_entries[]` action with its current accelerator, captures a new key, warns on conflicts, and applies the change with `gtk_application_set_accels_for_action`. It also lets you give a key to entries that have none. Save overrides by action name in the client options file. Respect `MGROUP_CHAR` (keys that must not fire while a text entry has focus). Put it in a new file, for example `keybindings.c`. It changes only menu actions; the hard-coded map keys in `gui_main.c` are out of scope for now. Done when: you rebind Save As, restart, and the new key still works.

## Blocked / waiting on human

### First Windows test build (2026-11-09)
`.github/workflows/windows.yml` is written but has never run. Only its YAML syntax was checked locally; the sandbox can't push or watch GitHub runs (`gh` isn't logged in).
1. [ ] Push main and a test tag from the host: `git push origin main && git tag test-2026-11-09 && git push origin test-2026-11-09`.
2. [ ] Watch https://github.com/RespectfulElemental/freeciv/actions. If it fails, copy the failing step's log into the chat. The likeliest failure is a DLL `cp` in the packaging step (`install-env-*` in `src/windows/installer_msys2/Makefile.meson`) because an MSYS2 package version moved away from the hardcoded DLL names. Don't pre-fix that.
3. [ ] When it's green, check that https://github.com/RespectfulElemental/freeciv/releases has the `-setup.exe`, `.7z` and `.zip`, then send the link to the Windows tester.
4. [ ] The tester starts a game, quicksaves (F5), quickloads (F9) and loads with Ctrl+L. Watch `gameload.c`'s local-server restart: it has never run on Windows.

Done when: the tester has played a game from the release. Next: back to Batch 4.

## Done (recent)

- 2026-11-09 — Batches 1–3 built and passed the user's 9-step test in the distrobox. Batch 1: Save As Ctrl+Shift+S; reports F6 Economy, F7 Research, F8 Wonders, F11 Messages, F12 Demographics, Shift+F11 Top Cities, Shift+F12 Spaceship; Connect With Rail Ctrl+Alt+L; `data/helpdata.txt` updated (shared by all clients). Batches 2–3: new `src/client/gui-gtk-4.0/gameload.c` with Load Game (Ctrl+L), quicksave F5 (`/save quicksave`), quickload F9, and lobby skip after load (it arms only if the save has a player with our username; otherwise it stays in the lobby). A "Load to lobby" checkbox keeps the old flow. City-name click: label size kept in `pcity->client.label_width/height` (`common/city.h`), hit-test `city_label_at()` in `mapctrl.c`. The label wins over a unit below it; the user is fine with that.
- 2026-11-09 — Closed: empty Save-dialog list on Windows. It was a Qt-client problem and doesn't reproduce in gtk4, so it's out of scope for now.
- 2026-11-08 — Moved the GitHub repo to a standalone (non-fork) repository, `RespectfulElemental/freeciv`.
- 2026-11-08 — First full build succeeded in `freeciv-dev`. Meson had to be added on top of `dnf builddep freeciv`. The suggested command was `dnf install meson ninja-build gtk4-devel SDL2_mixer-devel`; whether all four were needed wasn't checked. `build/` contains `freeciv-gtk4`, `freeciv-server`, `freeciv-mp-gtk4`, `freeciv-ruledit`, `freeciv-ruleup` and `freeciv-manual`.
- 2026-11-08 — Decision: move the GitHub repo to a standalone (non-fork) repository instead of staying a fork of freeciv/freeciv. Side effect: GitHub's "Contribute" button and gh's default PR target stop pointing at upstream.
- 2026-11-08 — Added local guards against PRs or pushes to upstream. `upstream` still fetches, for `git merge -X subtree=src upstream/S3_2`, but its push URL is set to `DISABLED--fork-is-decoupled-from-upstream`, so `git push upstream` fails. `remote.origin.gh-resolved=base` pins gh's default repo to `RespectfulElemental/freeciv`. To undo: `git remote set-url --push upstream https://github.com/freeciv/freeciv` and `git config --unset remote.origin.gh-resolved`.
- 2026-11-08 — Toolchain: meson and ninja are installed via Homebrew on the host. The `freeciv-dev` distrobox, with `dnf builddep freeciv`, is set up for gtk4 client builds.
- 2026-11-08 — Reset `main` to upstream stable `S3_2` (3.2.x, at `87d501f326`) and moved the tree into `src/` as a pure-rename commit (6721 files, all R100). `.github` went into `src/` too, so CI is inert. Added a root README/COPYING/.gitignore. The `git merge -X subtree=src upstream/S3_2` recipe was tested: a simulated 28-commit upstream merge came out clean and `src/` matched upstream blob-for-blob. `meson setup <build> src` parsed fine but stopped on sqlite3 missing in the sandbox, so a full build is still unverified.
