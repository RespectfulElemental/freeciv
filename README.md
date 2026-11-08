# Freeciv (home build)

A personal fork of [Freeciv](https://www.freeciv.org/), the open-source
empire-building strategy game, carrying our own modifications.

It tracks the upstream **stable 3.2 branch** (`S3_2`). The original upstream
README, documentation and license notices are in [`src/`](src/README.md).

## Layout

| Path | What |
| --- | --- |
| `src/` | The full Freeciv source tree, laid out as upstream ships it. The build system (meson and autotools) runs from here. |
| `src/doc/` | Upstream documentation, including `INSTALL.meson`. |
| `COPYING` | GPL v2, a copy of `src/COPYING` placed here so GitHub detects the license. |
| `.mailmap` | Upstream's author mapping. Git reads it only from the repository root. |

## Building

Freeciv builds with meson and ninja. Build from the repository root into a
separate directory and point meson at `src/`:

```sh
meson setup build src              # optional flags, e.g. -Dclients=gtk4,qt -Dprefix=$HOME/.local
ninja -C build
```

Run straight from the build directory, without installing:

```sh
cd build
./run.sh freeciv-server
./run.sh freeciv-gtk4
```

To install instead, run `ninja -C build install`. `meson configure build` lists
every build option. `src/doc/INSTALL.meson` has the full build instructions
and dependency list.

## Pulling in upstream fixes

This repo keeps upstream's history, so upstream fixes can be merged in. The
code lives under `src/` here but at the top level upstream, so pass
`-X subtree=src` so that git maps the paths:

```sh
git remote add upstream https://github.com/freeciv/freeciv   # once
git fetch upstream S3_2
git merge -X subtree=src upstream/S3_2
```

## Notes

- The Windows cross-build installer scripts in `src/windows/installer_cross/`
  look for `.git` next to `src/` and so leave the git revision out of their
  version strings. The main meson/autotools builds are unaffected.
- Upstream's GitHub Actions CI is turned off. Its workflows sit in
  `src/.github/`, and GitHub only runs workflows from a root `.github/`, so
  they never run. They're kept there rather than deleted so that upstream's
  edits to them merge without conflicts.
