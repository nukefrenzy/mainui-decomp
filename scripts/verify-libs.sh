#!/bin/sh
# Rebuild the two embedded libraries from their public sources with Miyoo's flags and check
# that every byte matches MainUI (only link-time relocated words are ignored).
#   SQLite 3.30.1 (C)       -> 0x57920..0x11cdc8
#   cJSON 1.7.10 (as C++)   -> 0x51b60..0x5791c
# Needs: MIYOO_TC, npm (SQLite source ships inside the npm package sqlite3@4.1.1), git.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
O=${1:-$ROOT/MainUI-354-clean}
TC=${MIYOO_TC:?set MIYOO_TC to the toolchain bin/ directory}
W=$(mktemp -d)
cd "$W"

npm pack sqlite3@4.1.1 --silent >/dev/null
tar xzf sqlite3-4.1.1.tgz package/deps/sqlite-autoconf-3300100.tar.gz
tar xzf package/deps/sqlite-autoconf-3300100.tar.gz
grep -q '2019-10-10 20:19:45 18db032d058f1436ce3dea84081f4ee5a0f2259ad97301d43c426bc7f3df1b0b' sqlite-autoconf-3300100/sqlite3.c
"$TC/arm-linux-gnueabihf-gcc" -O0 -fsigned-char -c sqlite-autoconf-3300100/sqlite3.c -o sqlite3.o

git init -q cjson && git -C cjson fetch -q --depth 1 https://github.com/DaveGamble/cJSON tag v1.7.10 && git -C cjson checkout -q FETCH_HEAD
"$TC/arm-linux-gnueabihf-g++" -x c++ -O0 -fsigned-char -c cjson/cJSON.c -o cjson.o

fail=0
python3 "$ROOT/tools/strictcmp.py" "$O" sqlite3.o 57920 || fail=1
python3 "$ROOT/tools/strictcmp.py" "$O" cjson.o 51b60 || fail=1
cd / && rm -rf "$W"
[ $fail = 0 ] && echo "Both libraries are byte-identical." || { echo "Library mismatch."; exit 1; }
