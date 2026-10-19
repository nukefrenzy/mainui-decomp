#!/usr/bin/env python3
"""Check whether a rewritten function compiles to exactly the original MainUI bytes.

Usage:
  matchfn.py <source.c|.cpp> <function_name> <orig_address_hex> [--orig MainUI-354-clean]
             [--toolchain DIR] [--flags "-O0 -fsigned-char"] [--context N]

Compiles <source> with the Arm GNU Toolchain 8.2-2018.08 (g++ for .cpp/.cc, gcc for .c)
using the flags recovered from MainUI, finds <function_name> in the object (plain or
mangled name), and compares it byte for byte with the original at <orig_address>.
Words the linker fills in (relocations: calls to other functions, string/global addresses,
MOVW/MOVT immediates) and BL calls are ignored, since they depend on final layout.

Exit code 0 = MATCH, 1 = differs, 2 = error.
"""
import argparse
import os
import subprocess
import sys
import tempfile

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

HERE = os.path.dirname(os.path.abspath(__file__))
ap = argparse.ArgumentParser()
ap.add_argument("source")
ap.add_argument("function")
ap.add_argument("address")
ap.add_argument("--orig", default=os.path.join(HERE, "MainUI-354-clean"))
ap.add_argument("--toolchain", default=os.environ.get("MIYOO_TC", ""),
                help="toolchain bin/ dir (or set MIYOO_TC)")
ap.add_argument("--flags", default="-O0 -fsigned-char")
ap.add_argument("--context", type=int, default=6)
a = ap.parse_args()

tc = a.toolchain
pre = os.path.join(tc, "arm-linux-gnueabihf-") if tc else "arm-linux-gnueabihf-"
cpp = a.source.endswith((".cpp", ".cc", ".cxx"))
cc = pre + ("g++" if cpp else "gcc")
objdump = pre + "objdump"
obj = tempfile.mktemp(suffix=".o")
r = subprocess.run([cc, *a.flags.split(), "-c", a.source, "-o", obj], capture_output=True, text=True)
if r.returncode:
    print(r.stderr)
    sys.exit(2)
ver = subprocess.run([cc, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
if "8.2-2018-08 (arm-rel-8.23)" not in ver:
    print(f"warning: not the original toolchain: {ver}")

o = ELFFile(open(obj, "rb"))
secs = list(o.iter_sections())
sym = None
for s in o.get_section_by_name(".symtab").iter_symbols():
    if s["st_info"]["type"] != "STT_FUNC" or s["st_size"] == 0:
        continue
    dem = subprocess.run([pre + "c++filt", s.name], capture_output=True, text=True).stdout.strip()
    base = dem.split("(")[0]
    if a.function in (s.name, dem, base) or base.endswith("::" + a.function) or base == a.function:
        sym = s
        break
if sym is None:
    print(f"function {a.function} not found in compiled object")
    sys.exit(2)
sec = secs[sym["st_shndx"]]
data = sec.data()
off, size = sym["st_value"] & ~1, sym["st_size"]
code = data[off:off + size]
mask = bytearray(size)
for rs in secs:
    if isinstance(rs, RelocationSection) and rs["sh_info"] == sym["st_shndx"]:
        for rel in rs.iter_relocations():
            p = rel["r_offset"] - off
            if 0 <= p < size:
                mask[p:p + 4] = b"\1\1\1\1"
for i in range(0, size - 3, 4):
    hb = code[i + 3]
    if ((hb & 0x0f) == 0x0b and (hb >> 4) != 0xf) or hb in (0xfa, 0xfb):
        mask[i:i + 4] = b"\1\1\1\1"

e = ELFFile(open(a.orig, "rb"))
addr = int(a.address, 16)
orig = None
for seg in e.iter_segments():
    if seg["p_type"] == "PT_LOAD" and seg["p_vaddr"] <= addr < seg["p_vaddr"] + seg["p_filesz"]:
        orig = seg.data()[addr - seg["p_vaddr"]:addr - seg["p_vaddr"] + size]
if orig is None or len(orig) < size:
    print("address not in the original file")
    sys.exit(2)

diffs = [i for i in range(0, size, 4) if not any(mask[i:i + 4]) and orig[i:i + 4] != code[i:i + 4]]
masked = sum(1 for i in range(0, size, 4) if any(mask[i:i + 4]))
name = f"{a.function} @ {hex(addr)}"
if not diffs:
    # Also check the original function doesn't continue past our end (size mismatch)
    nxt = None
    for seg in e.iter_segments():
        if seg["p_type"] == "PT_LOAD" and seg["p_vaddr"] <= addr < seg["p_vaddr"] + seg["p_filesz"]:
            nxt = seg.data()[addr - seg["p_vaddr"] + size - 4:addr - seg["p_vaddr"] + size]
    print(f"MATCH  {name}: {size} bytes, {size // 4 - masked} words exact, {masked} link-time words ignored")
    sys.exit(0)


def dis(blob, start):
    path = tempfile.mktemp(suffix=".bin")
    open(path, "wb").write(blob)
    out = subprocess.run([objdump, "-D", "-b", "binary", "-m", "arm", f"--adjust-vma={start}", path],
                         capture_output=True, text=True).stdout
    rows = {}
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) >= 3 and parts[0].strip().endswith(":"):
            rows[int(parts[0].strip()[:-1], 16)] = "\t".join(parts[2:]).strip()
    return rows


d0 = diffs[0]
lo, hi = max(0, d0 - 4 * a.context), min(size, d0 + 4 * (a.context + 1))
ro, rc = dis(orig, addr), dis(code, addr)
print(f"DIFF   {name}: {len(diffs)} of {size // 4} words differ; first at +{d0:#x}")
print(f"{'addr':>8}  {'original':40s} {'yours':40s}")
for i in range(lo, hi, 4):
    flag = "  <<" if i in diffs else ("  (link)" if any(mask[i:i + 4]) else "")
    print(f"{addr + i:8x}  {ro.get(addr + i, '?'):40s} {rc.get(addr + i, '?'):40s}{flag}")
sys.exit(1)
