#!/bin/sh
# Re-verify every byte-exact rewrite in src/ against the original MainUI.
# Usage: MIYOO_TC=/path/to/toolchain/bin scripts/run_matches.sh [path/to/MainUI-354-clean]
# The list lives in src/matches.tsv (file, symbol, addr, size, deps, cflags, signature); add one line per new
# rewrite. deps is "-" or a comma list of third-party headers the file needs (tinyutf8, rapidxml), or gcc73
# (compile the file with the GCC 7.3.0 cross compiler from scripts/fetch-gcc73.sh); a file
# gets the union of its rows' deps, since every row compiles the whole file.
# cflags is "-" or extra whitespace-separated compiler flags, identical for every row of a file.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
TINYUTF8=
RAPIDXML=
GCC73=
fails=$(tail -n +2 "$ROOT/src/matches.tsv" | while IFS="$(printf '\t')" read -r f fn addr size deps cflags sig; do
    [ -z "$f" ] && continue
    flags="-O0 -fsigned-char"
    [ "$cflags" = "-" ] || flags="$flags $cflags"
    deps=$(awk -F'\t' -v f="$f" '$1 == f && $5 != "-" { printf ",%s", $5 }' "$ROOT/src/matches.tsv")
    case "$deps," in
        *,tinyutf8,*) [ -n "$TINYUTF8" ] || TINYUTF8=$("$ROOT/scripts/fetch-tinyutf8.sh") || exit 1
                      flags="$flags -I$TINYUTF8" ;;
    esac
    case "$deps," in
        *,rapidxml,*) [ -n "$RAPIDXML" ] || RAPIDXML=$("$ROOT/scripts/fetch-rapidxml.sh") || exit 1
                      flags="$flags -I$RAPIDXML" ;;
    esac
    toolchain=${MIYOO_TC:-}
    case "$deps," in
        *,gcc73,*) [ -n "$GCC73" ] || GCC73=$("$ROOT/scripts/fetch-gcc73.sh") || exit 1
                   toolchain=$GCC73 ;;
    esac
    python3 "$ROOT/tools/matchfn.py" "$ROOT/src/$f" "$fn" "$addr" --orig "$O" --flags "$flags" \
        --toolchain "$toolchain" --quiet >&2 \
        || echo "FAILED: $f $fn $addr"
done)
if [ -n "$fails" ]; then echo "$fails"; exit 1; fi
echo "All rewrites match."
