#!/bin/sh
# Re-verify every byte-exact rewrite in src/ against the original MainUI.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_matches.sh [path/to/MainUI-354-clean]
# Add a line to MATCHES below for each new rewrite: "<file in src/> <function or mangled name> <hex address>"
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
MATCHES="
TrimusWindow.cpp _ZN12TrimusWindowC2Eii 1d7dc
TrimusWindow.cpp _ZN12TrimusWindowD2Ev 1d830
TrimusWindow.cpp animateIn 39478
TrimusWindow.cpp animateOut 394a0
TrimusWindow.cpp getId 394c8
MenuWindow.cpp _ZN10MenuWindowC2EiiP8ListMenu 2e6d4
MenuWindow.cpp _ZN10MenuWindowD2Ev 2ec3c
MenuWindow.cpp _ZN10MenuWindowD0Ev 2ed78
writeCmdToRun.cpp writeCmdToRun 17fc0
"
fail=0; n=0
echo "$MATCHES" | while read -r f fn addr; do
    [ -z "$f" ] && continue
    python3 "$ROOT/tools/matchfn.py" "$ROOT/src/$f" "$fn" "$addr" --orig "$O" || echo "FAILED: $f $fn $addr" >> "$ROOT/.match_failures"
done
if [ -f "$ROOT/.match_failures" ]; then cat "$ROOT/.match_failures"; rm -f "$ROOT/.match_failures"; exit 1; fi
echo "All rewrites match."
