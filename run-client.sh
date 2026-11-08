#!/usr/bin/env bash
# Run the gtk4 client from the meson build tree.
#
# Wraps build/run.sh, which meson generates. run.sh sets FREECIV_DATA_PATH
# (and the scenario and save paths) to point at src/data, then runs the
# binary from build/. There the debug build's ./freeciv-server lookup finds
# the server for "Start new game".
#
# Run it on the host or inside the distrobox. On the host it re-runs itself
# inside the freeciv-dev distrobox. Extra arguments go to the client.
set -euo pipefail

box=freeciv-dev
root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

if [[ ! -e /run/.containerenv && -z ${CONTAINER_ID:-} ]]; then
  command -v distrobox >/dev/null || {
    echo "run-client.sh: not in a container and distrobox isn't on PATH" >&2
    exit 1
  }
  exec distrobox enter "$box" -- "$root/run-client.sh" "$@"
fi

for f in run.sh freeciv-gtk4 freeciv-server; do
  [[ -x $root/build/$f ]] || {
    echo "run-client.sh: build/$f is missing." >&2
    echo "Build first: distrobox enter $box -- ninja -C '$root/build'" >&2
    exit 1
  }
done

cd "$root/build"
exec ./run.sh freeciv-gtk4 "$@"
