#!/bin/sh
# Check that your MainUI binary and toolchain are the exact ones this project targets.
# Usage: scripts/verify.sh [path/to/MainUI-354-clean] [path/to/toolchain/bin]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=${1:-$ROOT/MainUI-354-clean}
TC=${2:-${MIYOO_TC:-}}
ok=0

if echo "98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94  $BIN" | sha256sum -c --status 2>/dev/null; then
    echo "OK    MainUI-354-clean (Onion v4.4.0 beta 20260120)"
else
    echo "FAIL  $BIN is missing or not the expected MainUI-354-clean (run scripts/fetch-mainui.sh)"; ok=1
fi
if [ -n "$TC" ] && "$TC/arm-linux-gnueabihf-gcc" --version 2>/dev/null | head -1 | grep -q "8.2-2018-08 (arm-rel-8.23)"; then
    echo "OK    toolchain: $("$TC/arm-linux-gnueabihf-gcc" --version | head -1)"
else
    echo "FAIL  toolchain not found or wrong version (run scripts/fetch-toolchain.sh, set MIYOO_TC)"; ok=1
fi
python3 -c "import elftools" 2>/dev/null && echo "OK    pyelftools" || { echo "FAIL  pip install pyelftools"; ok=1; }
exit $ok
