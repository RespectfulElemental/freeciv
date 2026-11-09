#!/usr/bin/env bash
# Fork-only: copy into a Windows install dir every DLL its programs need.
#
# Usage: windows-bundle-dlls.sh <install dir> <msys2 dll dir> [<system dir>]
#
# Upstream's src/windows/installer_msys2/Makefile.meson lists the DLLs to ship
# by name. The lists go stale when MSYS2 packages gain dependencies, and the
# build still passes: the missing DLL only shows up when a player starts the
# game. This script reads the import table of every .exe and .dll under
# <install dir> (gdk-pixbuf loaders included) and copies each imported DLL
# that the top level lacks from <msys2 dll dir> (e.g. /ucrt64/bin). It
# repeats until nothing new is needed, so dependencies of dependencies come
# along too.
#
# DLLs that Windows itself provides (found in <system dir>, by default
# C:\Windows\System32) are never copied, so a MSYS2 build such as Mesa's
# opengl32.dll can't replace the system one. The exception is vulkan-1.dll,
# which comes with graphics drivers rather than with Windows.
#
# Fails if an import is found in none of those places.

set -euo pipefail

usage="usage: $0 <install dir> <msys2 dll dir> [<system dir>]"
inst=${1:?$usage}
src=${2:?$usage}
sys=${3:-/c/Windows/System32}
for d in "$inst" "$src" "$sys"; do
  [ -d "$d" ] || { echo "$0: not a directory: $d" >&2; exit 1; }
done
command -v objdump >/dev/null ||
  { echo "$0: objdump not found (it comes with binutils)" >&2; exit 1; }

# Bundle these even when the system dir has them.
declare -A always=([vulkan-1.dll]=1)

# Windows matches DLL names case-insensitively, so key everything on the
# lowercased name.
declare -A in_src in_sys have skipped
for f in "$src"/*; do b=${f##*/}; in_src[${b,,}]=$b; done
for f in "$sys"/*; do b=${f##*/}; in_sys[${b,,}]=1; done

added=0
while :; do
  have=()
  for f in "$inst"/*; do b=${f##*/}; have[${b,,}]=1; done
  new=0 files=0 imports=0
  unresolved=()
  while IFS= read -r -d '' pe; do
    files=$((files + 1))
    # Not inside the process substitution, so a failing objdump stops the script.
    names=$(objdump -p "$pe" | awk '/DLL Name:/ { print $3 }')
    for name in $names; do
      imports=$((imports + 1))
      l=${name,,}
      if [[ -n ${have[$l]:-} ]]; then
        continue
      elif [[ -n ${in_sys[$l]:-} && -z ${always[$l]:-} ]] ||
           [[ $l == api-ms-win-* || $l == ext-ms-* ]]; then
        # Log it when MSYS2 has one too: if the tester's Windows lacks it,
        # this is the line to look for.
        if [[ -n ${in_src[$l]:-} && -z ${skipped[$l]:-} ]]; then
          skipped[$l]=1
          echo "skipped ${in_src[$l]}: Windows has it (needed by ${pe#"$inst"/})"
        fi
        continue
      elif [[ -n ${in_src[$l]:-} ]]; then
        cp "$src/${in_src[$l]}" "$inst/"
        have[$l]=1
        new=$((new + 1))
        echo "added ${in_src[$l]} (needed by ${pe#"$inst"/})"
      else
        unresolved+=("$name (needed by ${pe#"$inst"/})")
      fi
    done
  done < <(find "$inst" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)
  added=$((added + new))
  [ "$new" -eq 0 ] && break
done

echo "$0: scanned $files files, $imports imports; added $added DLLs"
if [ "${#unresolved[@]}" -gt 0 ]; then
  echo "$0: these imports are in neither $src nor $sys:" >&2
  printf '  %s\n' "${unresolved[@]}" >&2
  exit 1
fi
[ "$files" -gt 0 ] || { echo "$0: no .exe or .dll found under $inst" >&2; exit 1; }
