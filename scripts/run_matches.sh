#!/bin/sh
# Re-verify every byte-exact rewrite in src/ against the original MainUI.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_matches.sh [path/to/MainUI-354-clean]
# The list lives in src/matches.tsv (file, symbol, addr, size, deps, signature); add one line per new
# rewrite. deps is "-" or a comma list of third-party headers the file needs (tinyutf8).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
TINYUTF8=
fails=$(tail -n +2 "$ROOT/src/matches.tsv" | while IFS="$(printf '\t')" read -r f fn addr size deps sig; do
    [ -z "$f" ] && continue
    flags="-O0 -fsigned-char"
    case ",$deps," in
        *,tinyutf8,*) [ -n "$TINYUTF8" ] || TINYUTF8=$("$ROOT/scripts/fetch-tinyutf8.sh") || exit 1
                      flags="$flags -I$TINYUTF8" ;;
    esac
    python3 "$ROOT/tools/matchfn.py" "$ROOT/src/$f" "$fn" "$addr" --orig "$O" --flags "$flags" --quiet >&2 \
        || echo "FAILED: $f $fn $addr"
done)
if [ -n "$fails" ]; then echo "$fails"; exit 1; fi
echo "All rewrites match."
