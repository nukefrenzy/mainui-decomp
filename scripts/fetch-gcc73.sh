#!/bin/sh
# Build the GCC 7.3.0 ARM cross compiler that stamped MainUI's second .comment string,
# "GCC: (GNU) 7.3.0" (an upstream source build: no vendor package tag). Only the C compiler
# driver and cc1 are built; assembling and linking reuse the pinned Arm 8.2 toolchain
# (scripts/fetch-toolchain.sh), as do its sysroot headers and its target defaults
# (armv7-a, NEON, hard float, ARM mode). Sources come from ftp.gnu.org and GCC's
# infrastructure mirror (the prerequisite versions GCC 7.3.0's download_prerequisites uses),
# pinned by size and SHA-256. Everything stays under build/gcc73/ (gitignored).
# Prints the bin/ directory of the installed compiler.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TC=$("$ROOT/scripts/fetch-toolchain.sh")
BASE="$ROOT/build/gcc73"
DL="$BASE/dl" SRC="$BASE/src" OBJ="$BASE/obj" PREFIX="$BASE/install"
CC_OUT="$PREFIX/bin/arm-linux-gnueabihf-gcc"

if [ ! -x "$CC_OUT" ]; then
    mkdir -p "$DL"
    while read -r url size sha; do
        f="$DL/$(basename "$url")"
        if ! { [ -f "$f" ] && echo "$sha  $f" | sha256sum -c --status; }; then
            curl -fsSL --retry 3 -o "$f" "$url"
        fi
        [ "$(stat -c %s "$f")" = "$size" ] || { echo "size mismatch: $f" >&2; exit 1; }
        echo "$sha  $f" | sha256sum -c >&2
    done <<'EOF'
https://ftp.gnu.org/gnu/gcc/gcc-7.3.0/gcc-7.3.0.tar.xz 62462388 832ca6ae04636adbb430e865a1451adf6979ab44ca1c8374f61fba65645ce15c
https://gcc.gnu.org/pub/gcc/infrastructure/gmp-6.1.0.tar.bz2 2383840 498449a994efeba527885c10405993427995d3f86b8768d8cdf8d9dd7c6b73e8
https://gcc.gnu.org/pub/gcc/infrastructure/mpfr-3.1.4.tar.bz2 1279284 d3103a80cdad2407ed581f3618c4bed04e0c92d1cf771a65ead662cc397f7775
https://gcc.gnu.org/pub/gcc/infrastructure/mpc-1.0.3.tar.gz 669925 617decc6ea09889fb08ede330917a00b16809b8db88c29c31bfbb49cbf88ecc3
EOF
    rm -rf "$SRC" "$OBJ" "$PREFIX"
    mkdir -p "$SRC" "$OBJ"
    tar xJf "$DL/gcc-7.3.0.tar.xz" -C "$SRC"
    tar xjf "$DL/gmp-6.1.0.tar.bz2" -C "$SRC"
    tar xjf "$DL/mpfr-3.1.4.tar.bz2" -C "$SRC"
    tar xzf "$DL/mpc-1.0.3.tar.gz" -C "$SRC"
    ln -s ../gmp-6.1.0 "$SRC/gcc-7.3.0/gmp"
    ln -s ../mpfr-3.1.4 "$SRC/gcc-7.3.0/mpfr"
    ln -s ../mpc-1.0.3 "$SRC/gcc-7.3.0/mpc"
    (cd "$OBJ" && "$SRC/gcc-7.3.0/configure" \
        --target=arm-linux-gnueabihf --prefix="$PREFIX" \
        --with-sysroot="$TC/../arm-linux-gnueabihf/libc" \
        --with-as="$TC/arm-linux-gnueabihf-as" --with-ld="$TC/arm-linux-gnueabihf-ld" \
        --with-arch=armv7-a --with-fpu=neon --with-float=hard \
        --enable-languages=c --enable-checking=release \
        --disable-multilib --disable-nls --disable-bootstrap --disable-shared \
        --disable-libssp --disable-libsanitizer --disable-libquadmath --disable-libgomp \
        --disable-libatomic --disable-libitm --disable-libcilkrts --disable-libvtv \
        >"$BASE/configure.log" 2>&1)
    (cd "$OBJ" && make -j"$(nproc)" all-gcc >"$BASE/build.log" 2>&1 && make install-gcc >"$BASE/install.log" 2>&1)
fi
# Complete the bin/ directory with the 8.2 toolchain's binutils (as, ld, objdump, c++filt, ...):
# the compiler already uses them through --with-as/--with-ld, and the match tools expect every
# arm-linux-gnueabihf-* tool next to the compiler they are given.
for name in addr2line ar as c++filt elfedit gprof ld ld.bfd nm objcopy objdump ranlib readelf size strings strip; do
    tool="$TC/arm-linux-gnueabihf-$name"
    [ -e "$tool" ] && [ ! -e "$PREFIX/bin/arm-linux-gnueabihf-$name" ] && ln -s "$tool" "$PREFIX/bin/arm-linux-gnueabihf-$name"
done
# Only binutils are borrowed: never a compiler driver from the 8.2 toolchain (this build is C only).
for name in g++ c++ gcc-8.2.1 gfortran; do
    [ -L "$PREFIX/bin/arm-linux-gnueabihf-$name" ] && rm "$PREFIX/bin/arm-linux-gnueabihf-$name"
done
"$CC_OUT" --version | head -1 | grep -q '(GCC) 7.3.0$'
echo "$PREFIX/bin"
