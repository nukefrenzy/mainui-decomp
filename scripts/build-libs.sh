#!/bin/sh
# Retain pinned public sources and verified embedded-library objects under ignored build/.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
TC=${MIYOO_TC:?set MIYOO_TC to the pinned toolchain bin/ directory}
W="$ROOT/build/embedded-libs"
mkdir -p "$W"
rm -f "$W/cjson.o" "$W/sqlite3.o" "$W/verified"
echo '98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94  '"$O" | sha256sum -c
"$TC/arm-linux-gnueabihf-gcc" --version | head -1 | grep -q '8.2-2018-08 (arm-rel-8.23)'
fetch() {
    if ! { [ -f "$W/$1" ] && echo "$2  $W/$1" | sha256sum -c --status; }; then
        curl -fLsS --retry 3 "$3" -o "$W/$1"
    fi
    echo "$2  $W/$1" | sha256sum -c
}
fetch sqlite3-4.1.1.tgz 09ddd35a26274dd54895c5772c273ea275e3943fdcc241e2ec8718742841fb07 \
    https://registry.npmjs.org/sqlite3/-/sqlite3-4.1.1.tgz
fetch cJSON.c a2470214ebeeb3ba196b0d221b77357348603f769343e1ae5be06a6630fa7169 \
    https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.10/cJSON.c
fetch cJSON.h f1a5e0b21e357eb3abc1de9b2c494410156fe4de323aadb7221fb45b413bcbdc \
    https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.10/cJSON.h
tar xzf "$W/sqlite3-4.1.1.tgz" -C "$W" package/deps/sqlite-autoconf-3300100.tar.gz
tar xzf "$W/package/deps/sqlite-autoconf-3300100.tar.gz" -C "$W"
echo "e0a86381558c57a8e1a4a80fe94e842ad4ef1804a135452e253a4962c67229a6  $W/sqlite-autoconf-3300100/sqlite3.c" | sha256sum -c
"$TC/arm-linux-gnueabihf-gcc" -O0 -fsigned-char -c "$W/sqlite-autoconf-3300100/sqlite3.c" -o "$W/sqlite3.o"
"$TC/arm-linux-gnueabihf-g++" -x c++ -O0 -fsigned-char -c "$W/cJSON.c" -o "$W/cjson.o"
python3 "$ROOT/tools/strictcmp.py" "$O" "$W/cjson.o" 51b60
python3 "$ROOT/tools/strictcmp.py" "$O" "$W/sqlite3.o" 57920
echo 'Both libraries are byte-identical; objects retained in build/embedded-libs/.'
touch "$W/verified"
