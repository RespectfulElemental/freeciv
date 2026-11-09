#!/usr/bin/env bash
# Fork-only: drop gdk-pixbuf image loaders that Freeciv doesn't need.
#
# Usage: windows-trim-pixbuf-loaders.sh <install dir>
#
# Upstream's Makefile.meson copies every gdk-pixbuf loader MSYS2 has. The
# HEIF, AVIF, JPEG XL and WMF loaders pull in about 60 MB of codec DLLs
# (x265, aom, SVT-AV1, ...) once windows-bundle-dlls.sh adds their
# dependencies, and Freeciv's data uses none of those formats (it has PNG,
# SVG and JPEG). This removes those loaders and their entries in
# loaders.cache. Run it before windows-bundle-dlls.sh.
#
# Fails if any loaders.cache entry then points at a missing file.

set -euo pipefail

inst=${1:?usage: $0 <install dir>}
dir="$inst/lib/gdk-pixbuf-2.0/2.10.0"
cache="$dir/loaders.cache"
[ -f "$cache" ] || { echo "$0: not found: $cache" >&2; exit 1; }

drop=(
  io-wmf.dll
  libpixbufloader-avif.dll
  libpixbufloader-heif.dll
  libpixbufloader-jxl.dll
)

for f in "${drop[@]}"; do
  if [ -e "$dir/loaders/$f" ]; then
    rm "$dir/loaders/$f"
    echo "removed loader $f"
  else
    echo "loader $f not present"
  fi
done

# The cache has one blank-line-separated block per loader; its first line is
# the loader's quoted path. Keep the header and every block whose loader
# still exists.
tmp="$cache.tmp"
awk -v drop="${drop[*]}" '
  BEGIN { n = split(drop, d, " "); for (i = 1; i <= n; i++) skip[tolower(d[i])] = 1 }
  /^"/ && !inblock {
    inblock = 1
    p = $0; gsub(/"/, "", p); sub(/.*\\/, "", p)
    keep = !(tolower(p) in skip)
  }
  !inblock || keep { print }
  /^$/ { inblock = 0 }
' "$cache" > "$tmp"
mv "$tmp" "$cache"

# Every remaining entry must point at a file that is there.
missing=0 entries=0
while IFS= read -r line; do
  entries=$((entries + 1))
  rel=${line//\"/}
  rel=${rel//\\\\/\/}
  [ -e "$inst/$rel" ] || { echo "$0: loaders.cache lists missing $rel" >&2; missing=1; }
done < <(grep '^"lib' "$cache")
echo "$0: loaders.cache has $entries loaders"
[ "$entries" -gt 0 ] || { echo "$0: loaders.cache lists no loaders" >&2; exit 1; }
exit "$missing"
