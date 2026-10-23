#!/bin/sh
# Check every source file listed in docs/tu-map.tsv against the original MainUI, as the linker
# lays it out: body, then the template/inline tail in object order. See tools/unitcheck.py.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_units.sh [path/to/MainUI-354-clean]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
fails=
# Units of Miyoo's own code: rows marked "proven"; the name column is the file name without extension.
for u in $(awk -F'\t' 'NR > 1 && $4 == "proven" { print $3 }' "$ROOT/docs/tu-map.tsv"); do
    f="$ROOT/src/$u.cpp"
    [ -e "$f" ] || f="$ROOT/src/$u.c"
    if [ ! -e "$f" ]; then
        echo "== $u: no src/$u.cpp or src/$u.c"
        fails="$fails $u"
        continue
    fi
    echo "== $(basename "$f")"
    python3 "$ROOT/tools/unitcheck.py" "$f" --orig "$O" || fails="$fails $(basename "$f")"
done
if [ -n "$fails" ]; then echo "FAILED:$fails"; exit 1; fi
echo "All units match."
