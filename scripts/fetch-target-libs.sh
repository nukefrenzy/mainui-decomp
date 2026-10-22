#!/bin/sh
# SDK used by Onion's development toolchain; only for link/ABI audit, not a device image.
# Keep all vendor binaries in ignored build/. Runtime libraries come from pinned Arm GNU.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
W="$ROOT/build/target-libs"
ARCHIVE="$W/my283.tar.xz"
SHA=67c2dcc7aac183c1e3dd893dbb3c2f138ef59ae8fc738b7a266c98e67f32c038
URL=https://raw.githubusercontent.com/shauninman/miyoomini-toolchain-buildroot/30e0e41f859f81b7ec390be34006ab1ba5f12f80/support/my283.tar.xz
mkdir -p "$W/lib"
if ! { [ -f "$ARCHIVE" ] && echo "$SHA  $ARCHIVE" | sha256sum -c --status; }; then
    curl -fLsS --retry 3 "$URL" -o "$ARCHIVE"
fi
echo "$SHA  $ARCHIVE" | sha256sum -c >&2
tar xJf "$ARCHIVE" -C "$W"
python3 - "$W" <<'PY'
from pathlib import Path
import sys
from elftools.elf.elffile import ELFFile
root = Path(sys.argv[1])
files = ['libz.so.1.2.8', 'libSDL-1.2.so.0.11.4', 'libSDL_ttf-2.0.so.0.10.1',
         'libSDL_image-1.2.so.0.8.4', 'libSDL_mixer-1.2.so.0.12.0', 'libshmvar.so',
         'libgamename.so', 'libfreetype.so.6.17.1', 'librsautil.so', 'libmi_common.so',
         'libmi_sys.so', 'libmi_disp.so', 'libmi_panel.so', 'libmi_gfx.so',
         'libmi_divp.so', 'libmi_ao.so', 'libmad.so.0.2.1']
for name in files:
    source = root/'my283/usr/lib'/name
    with source.open('rb') as stream:
        elf = ELFFile(stream)
        if (elf['e_machine'] != 'EM_ARM' or elf['e_type'] != 'ET_DYN'
                or elf.elfclass != 32 or not elf.little_endian
                or elf['e_flags'] != 0x5000400):
            raise ValueError(f'{name}: expected ARM EABI5 hard-float shared library')
        sonames = [t.soname for t in elf.get_section_by_name('.dynamic').iter_tags()
                   if t.entry.d_tag == 'DT_SONAME']
        soname = sonames[0] if sonames else name  # Miyoo/MI libraries omit DT_SONAME.
        if Path(soname).name != soname:
            raise ValueError(f'{name}: unsafe SONAME')
    target = root/'lib'/soname
    target.unlink(missing_ok=True)
    target.symlink_to(Path('../my283/usr/lib')/name)
print(root/'lib')
PY
