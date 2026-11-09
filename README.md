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
| `CHANGELOG.md` | What this fork changes compared to upstream. |
| `.github/workflows/windows.yml` | Builds the gtk4 client for Windows and publishes test builds. |
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

## Windows test builds

GitHub builds the gtk4 client for Windows when you push a tag whose name
starts with `test-`:

```sh
git push origin main
git tag test-2026-11-09            # add .2, .3 ... for more builds that day
git push origin test-2026-11-09
```

The build runs under the repo's **Actions** tab. When it finishes, a
pre-release appears on the **Releases** page with an installer (`-setup.exe`)
and a no-install `.zip`. The release notes are the newest `CHANGELOG.md`
section. Send testers the release link. Downloading needs no GitHub account.

Upstream's packaging lists the DLLs to ship by name, and those lists fall
behind MSYS2. So `src/windows/installer_msys2/Makefile.meson` runs two
fork scripts after its own copying:

1. `windows-trim-pixbuf-loaders.sh` removes the gdk-pixbuf loaders for HEIF,
   AVIF, JPEG XL and WMF. Freeciv doesn't use those formats, and their
   codecs would add about 60 MB.
2. `windows-bundle-dlls.sh` reads every program's imports and copies
   whatever DLLs are missing.

Afterwards, CI starts the server and client with `--version` and a bare
`PATH`. If a DLL is missing, the build fails there, before anything is
published.

To build without publishing, open Actions → *windows test build* → **Run
workflow**. The files are then attached to the run (downloading those needs
a sign-in).

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
- Upstream's GitHub Actions CI is turned off. Only the fork's own
  `.github/workflows/windows.yml` runs. Its workflows sit in
  `src/.github/`, and GitHub only runs workflows from a root `.github/`, so
  they never run. They're kept there rather than deleted so that upstream's
  edits to them merge without conflicts.
