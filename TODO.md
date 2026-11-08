# TODO

## Resume here

Waiting on the user to gather the requested changes, then start on them. Target is the **gtk4 client**. Build and run it on the host in the `freeciv-dev` distrobox (`dnf builddep freeciv` done 2026-11-08): `distrobox enter freeciv-dev -- bash -c 'cd ~/projects/mods/workshop-garage/freeciv && ninja -C build'`. The first full build succeeded on 2026-11-08. Run the client with `./run-client.sh`, from the host or inside the box. The sandbox uses its own `build-sandbox/` for server-only checks, so it never touches `build/`. The sandbox has meson and ninja from Homebrew but no GTK4, so it can only do server-only compile checks.

## Active

## Blocked / waiting on human

(none)

## Done (recent)

- 2026-11-08 — Moved the GitHub repo to a standalone (non-fork) repository, `RespectfulElemental/freeciv`.
- 2026-11-08 — First full build succeeded in `freeciv-dev`. Meson had to be added on top of `dnf builddep freeciv`. The suggested command was `dnf install meson ninja-build gtk4-devel SDL2_mixer-devel`; whether all four were needed wasn't checked. `build/` contains `freeciv-gtk4`, `freeciv-server`, `freeciv-mp-gtk4`, `freeciv-ruledit`, `freeciv-ruleup` and `freeciv-manual`.
- 2026-11-08 — Decision: move the GitHub repo to a standalone (non-fork) repository instead of staying a fork of freeciv/freeciv. Side effect: GitHub's "Contribute" button and gh's default PR target stop pointing at upstream.
- 2026-11-08 — Added local guards against PRs or pushes to upstream. `upstream` still fetches, for `git merge -X subtree=src upstream/S3_2`, but its push URL is set to `DISABLED--fork-is-decoupled-from-upstream`, so `git push upstream` fails. `remote.origin.gh-resolved=base` pins gh's default repo to `RespectfulElemental/freeciv`. To undo: `git remote set-url --push upstream https://github.com/freeciv/freeciv` and `git config --unset remote.origin.gh-resolved`.
- 2026-11-08 — Toolchain: meson and ninja are installed via Homebrew on the host. The `freeciv-dev` distrobox, with `dnf builddep freeciv`, is set up for gtk4 client builds.
- 2026-11-08 — Reset `main` to upstream stable `S3_2` (3.2.x, at `87d501f326`) and moved the tree into `src/` as a pure-rename commit (6721 files, all R100). `.github` went into `src/` too, so CI is inert. Added a root README/COPYING/.gitignore. The `git merge -X subtree=src upstream/S3_2` recipe was tested: a simulated 28-commit upstream merge came out clean and `src/` matched upstream blob-for-blob. `meson setup <build> src` parsed fine but stopped on sqlite3 missing in the sandbox, so a full build is still unverified.
