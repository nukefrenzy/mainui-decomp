#!/bin/sh
# Fetch tiny-utf8 (github.com/DuffsDevice/tiny-utf8), the header-only UTF-8 string library compiled
# into MainUI. Its code matches v4.4.1-v4.4.3 (identical under MainUI's flags); v4.4.3 is pinned.
# Unpacks to build/tinyutf8/v4.4.3/ (gitignored) and prints the include directory for -I.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TAG=v4.4.3
COMMIT=84d9878051cd89eb930ebfc2b686d2edfdb9db10
SHA=e8d0133547f1e8ac3b6aafabe12361de48ae8261bffcd50fe11b4e981d0fe153   # include/tinyutf8/tinyutf8.h
DIR="$ROOT/build/tinyutf8/$TAG"
HDR="$DIR/include/tinyutf8/tinyutf8.h"

if ! { [ -f "$HDR" ] && echo "$SHA  $HDR" | sha256sum -c --status; }; then
    rm -rf "$DIR"
    mkdir -p "$DIR"
    git -C "$DIR" init -q
    git -C "$DIR" fetch -q --depth 1 https://github.com/DuffsDevice/tiny-utf8 "$COMMIT"
    git -C "$DIR" checkout -q FETCH_HEAD
    echo "$SHA  $HDR" | sha256sum -c >&2
fi
echo "$DIR/include"
