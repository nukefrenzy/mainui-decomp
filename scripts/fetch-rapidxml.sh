#!/bin/sh
# Fetch RapidXml 1.13 (rapidxml.sourceforge.net), the header-only XML parser compiled into MainUI
# (MainUI.cpp reads gamelist.xml with it; rapidxml::parse_error sits in that unit's tail at 0x3acc0).
# Unpacks to build/rapidxml/1.13/ (gitignored) and prints the include directory for -I.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VER=1.13
URL="https://sourceforge.net/projects/rapidxml/files/rapidxml/rapidxml%20$VER/rapidxml-$VER.zip/download"
ZIPSHA=c3f0b886374981bb20fabcf323d755db4be6dba42064599481da64a85f5b3571
SHA=d61c53fd63f11aef0e18d253746ee800903dc82e4ad3cc533d0fdca69f07c4f9   # rapidxml.hpp
DIR="$ROOT/build/rapidxml/$VER"
HDR="$DIR/rapidxml-$VER/rapidxml.hpp"

if ! { [ -f "$HDR" ] && echo "$SHA  $HDR" | sha256sum -c --status; }; then
    rm -rf "$DIR"
    mkdir -p "$DIR"
    curl -sSfL -o "$DIR/rapidxml-$VER.zip" "$URL"
    echo "$ZIPSHA  $DIR/rapidxml-$VER.zip" | sha256sum -c >&2
    python3 -I -c 'import sys, zipfile; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])' \
        "$DIR/rapidxml-$VER.zip" "$DIR"
    echo "$SHA  $HDR" | sha256sum -c >&2
fi
echo "$DIR/rapidxml-$VER"
