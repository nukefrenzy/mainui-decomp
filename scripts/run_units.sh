#!/bin/sh
# Check every whole unit in src/ (src/tu_<lo>.cpp, or .c for C files) against the original MainUI as the linker lays it out:
# body, then the template/inline tail in object order. See tools/unitcheck.py.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_units.sh [path/to/MainUI-354-clean]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
fails=
for f in "$ROOT"/src/tu_*.cpp "$ROOT"/src/tu_*.c; do
    [ -e "$f" ] || continue
    echo "== $(basename "$f")"
    python3 "$ROOT/tools/unitcheck.py" "$f" --orig "$O" || fails="$fails $(basename "$f")"
done
if [ -n "$fails" ]; then echo "FAILED:$fails"; exit 1; fi
echo "All units match."
