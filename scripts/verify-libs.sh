#!/bin/sh
# Compatibility entry point: rebuild, compare and retain the two embedded library objects.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
exec "$ROOT/scripts/build-libs.sh" "$@"
