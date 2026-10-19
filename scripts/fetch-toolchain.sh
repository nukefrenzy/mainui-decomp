#!/bin/sh
# Download and verify the exact compiler Miyoo used: Arm GNU Toolchain 8.2-2018.08
# (arm-linux-gnueabihf, x86_64 Linux host). Unpacks to ./toolchain/.
# Prints the bin/ directory to use as MIYOO_TC.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
NAME=gcc-arm-8.2-2018.08-x86_64-arm-linux-gnueabihf
URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/8.2-2018.08/$NAME.tar.xz
SHA=5b3f20e1327edc3073e545a5bd3d15f33e7f94181ff4e37a76e95924c1b439b9   # Arm's published value
TAR="$ROOT/toolchain/$NAME.tar.xz"

mkdir -p "$ROOT/toolchain"
if [ ! -x "$ROOT/toolchain/$NAME/bin/arm-linux-gnueabihf-gcc" ]; then
    if [ ! -f "$TAR" ] || ! echo "$SHA  $TAR" | sha256sum -c --status; then
        curl -fL --retry 3 -o "$TAR" "$URL"
    fi
    echo "$SHA  $TAR" | sha256sum -c >&2
    tar xJf "$TAR" -C "$ROOT/toolchain"
fi
"$ROOT/toolchain/$NAME/bin/arm-linux-gnueabihf-gcc" --version | head -1 | grep -q "8.2-2018-08 (arm-rel-8.23)"
echo "$ROOT/toolchain/$NAME/bin"
