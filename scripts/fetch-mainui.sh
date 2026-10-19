#!/bin/sh
# Fetch MainUI-354-clean from Onion's public git history (commit 07505ea, the v4.4.0 beta
# 20260120 build) and verify it. The binary is Miyoo's and is not stored in this repository.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/MainUI-354-clean"
COMMIT=07505ea58c7bba698d6b9220ff43946a43cac76b
FILE=static/build/.tmp_update/bin/MainUI-354-clean
SHA=98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94

if [ -f "$OUT" ] && echo "$SHA  $OUT" | sha256sum -c --status; then
    echo "MainUI-354-clean already present and verified"; exit 0
fi
TMP=$(mktemp -d)
git -C "$TMP" init -q
git -C "$TMP" remote add origin https://github.com/OnionUI/Onion
git -C "$TMP" fetch -q --depth 1 --filter=blob:none origin "$COMMIT"
git -C "$TMP" show "FETCH_HEAD:$FILE" > "$OUT"
rm -rf "$TMP"
echo "$SHA  $OUT" | sha256sum -c
