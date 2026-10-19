#!/bin/sh
# Re-verify every byte-exact rewrite in src/ against the original MainUI.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_matches.sh [path/to/MainUI-354-clean]
# The list lives in src/matches.tsv (file, symbol, addr, size, signature); add one line per new rewrite.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
fails=$(tail -n +2 "$ROOT/src/matches.tsv" | while IFS="$(printf '\t')" read -r f fn addr size sig; do
    [ -z "$f" ] && continue
    python3 "$ROOT/tools/matchfn.py" "$ROOT/src/$f" "$fn" "$addr" --orig "$O" --quiet >&2 || echo "FAILED: $f $fn $addr"
done)
if [ -n "$fails" ]; then echo "$fails"; exit 1; fi
echo "All rewrites match."
