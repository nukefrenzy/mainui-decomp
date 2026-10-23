#!/bin/sh
# Build MainUI from src/ and prove it byte-identical to the original.
# Usage: scripts/build-mainui.sh            (fetches and verifies every pinned input it needs)
#
# Runs the full link audit with --require-ready: every unit is compiled with its recorded
# compiler and flags (src/matches.tsv; DeviceAuth.c uses GCC 7.3.0 from scripts/fetch-gcc73.sh),
# SQLite/cJSON are rebuilt and verified, and the executable is linked against the pinned
# SDK/runtime libraries, then stripped. It fails unless the result's SHA-256 equals the
# original's and every audit check passes. The binary is Miyoo's program: it is written to
# build/link-audit/MainUI (gitignored) and must never be committed or published.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
export MIYOO_TC=${MIYOO_TC:-$(scripts/fetch-toolchain.sh)}
scripts/fetch-mainui.sh >&2
scripts/fetch-tinyutf8.sh >/dev/null
scripts/fetch-rapidxml.sh >/dev/null
scripts/fetch-gcc73.sh >/dev/null
python3 tools/linkaudit.py --require-ready >&2
OUT=build/link-audit/MainUI
EXPECTED=98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94
echo "$EXPECTED  $OUT" | sha256sum -c >&2
echo "$ROOT/$OUT"
