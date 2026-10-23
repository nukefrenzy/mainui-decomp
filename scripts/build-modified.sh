#!/bin/sh
# Build MainUI from src/ after you have changed it (a modified build).
# Usage: scripts/build-modified.sh
#
# Same compilers, flags, libraries and link as scripts/build-mainui.sh, but without requiring the
# result to equal the original: the link audit's byte comparisons are expected to fail once the
# code changes, so they are reported, not enforced. Compile or link errors still fail the build.
# The result is build/MainUI-modified. It is still Miyoo's program with your changes: never commit
# it to this repository. For an unmodified tree it is byte-identical to the original, and the
# script says so.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
export MIYOO_TC=${MIYOO_TC:-$(scripts/fetch-toolchain.sh)}
scripts/fetch-mainui.sh >&2
scripts/fetch-tinyutf8.sh >/dev/null
scripts/fetch-rapidxml.sh >/dev/null
scripts/fetch-gcc73.sh >/dev/null
rm -f build/link-audit/MainUI
status=0
python3 tools/linkaudit.py >&2 || status=$?
if [ "$status" -ge 2 ] || [ ! -f build/link-audit/MainUI ]; then
    echo "build-modified: compiling or linking failed (see build/link-audit/)" >&2
    exit 1
fi
cp build/link-audit/MainUI build/MainUI-modified
EXPECTED=98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94
if echo "$EXPECTED  build/MainUI-modified" | sha256sum -c --status; then
    echo "build-modified: identical to MainUI-354-clean (no code changes)" >&2
else
    echo "build-modified: differs from MainUI-354-clean, as expected for a modified build" >&2
fi
echo "$ROOT/build/MainUI-modified"
