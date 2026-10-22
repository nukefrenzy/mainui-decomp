#!/bin/sh
# Check every whole unit in src/ (src/tu_<lo>.cpp, or .c for C files) against the original MainUI as the linker lays it out:
# body, then the template/inline tail in object order. See tools/unitcheck.py.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_units.sh [path/to/MainUI-354-clean]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
# Units still being written: only their body prefix is reported (unitcheck --body-only); a short prefix is
# not a failure, a compile error is.
# Remove a unit from this list once it is complete.
PARTIAL=""
fails=
for f in "$ROOT"/src/tu_*.cpp "$ROOT"/src/tu_*.c; do
    [ -e "$f" ] || continue
    u=$(basename "$f"); u=${u%.*}
    case " $PARTIAL " in
        *" $u "*) echo "== $(basename "$f") (in progress, body prefix only)"
                  python3 "$ROOT/tools/unitcheck.py" "$f" --orig "$O" --body-only
                  [ $? -le 1 ] || fails="$fails $(basename "$f")"
                  continue ;;
    esac
    echo "== $(basename "$f")"
    python3 "$ROOT/tools/unitcheck.py" "$f" --orig "$O" || fails="$fails $(basename "$f")"
done
if [ -n "$fails" ]; then echo "FAILED:$fails"; exit 1; fi
echo "All units match."
