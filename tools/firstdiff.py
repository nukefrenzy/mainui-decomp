"""For functions in an object that don't fully match, locate them in the original by their
prefix and report the first differing 4-byte word (both instructions disassembled).

Usage: python3 -I firstdiff.py <original_elf> <object.o> <lo> <hi> <objdump> [N]
"""
import subprocess
import sys
import collections

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

orig_path, obj_path, lo, hi, objdump = sys.argv[1:6]
N = int(sys.argv[6]) if len(sys.argv) > 6 else 15
orig = ELFFile(open(orig_path, "rb"))
ot = orig.get_section_by_name(".text")
od, ob = ot.data(), ot["sh_addr"]
lo, hi = int(lo, 16) - ob, int(hi, 16) - ob

obj = ELFFile(open(obj_path, "rb"))
ti = [i for i, s in enumerate(obj.iter_sections()) if s.name == ".text"][0]
td = obj.get_section(ti).data()
mask = bytearray(len(td))
for sec in obj.iter_sections():
    if isinstance(sec, RelocationSection) and sec["sh_info"] == ti:
        for r in sec.iter_relocations():
            mask[r["r_offset"]:r["r_offset"] + 4] = b"\1\1\1\1"
funcs = [(s.name, s["st_value"], s["st_size"]) for s in obj.get_section_by_name(".symtab").iter_symbols()
         if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] == ti and s["st_size"] >= 64]


def dis(word):
    open("/tmp/_w.bin", "wb").write(word)
    out = subprocess.run([objdump, "-D", "-b", "binary", "-m", "arm", "/tmp/_w.bin"], capture_output=True, text=True).stdout
    return out.strip().splitlines()[-1].split("\t", 2)[-1].strip() if out else "?"


kinds = collections.Counter()
shown = 0
for name, off, size in sorted(funcs, key=lambda f: -f[2]):
    code, m = td[off:off + size], mask[off:off + size]
    # match prefix of 24 bytes (masked) anywhere 4-aligned in region
    pre = 24
    cands = []
    first = next((i for i in range(0, pre, 4) if not any(m[i:i + 4])), None)
    if first is None:
        continue
    p = od.find(code[first:first + 4], lo)
    while p != -1 and p < hi and len(cands) < 50:
        s = p - first
        if s % 4 == 0 and all(m[i] or od[s + i] == code[i] for i in range(pre)):
            cands.append(s)
        p = od.find(code[first:first + 4], p + 1)
    best = None
    for s in cands:
        d = next((i for i in range(0, size, 4) if not any(m[i:i + 4]) and od[s + i:s + i + 4] != code[i:i + 4]), None)
        if d is None:
            best = None
            break  # full match
        if best is None or d > best[1]:
            best = (s, d)
    if best is None:
        continue
    s, d = best
    a, b = dis(od[s + d:s + d + 4]), dis(code[d:d + 4])
    kinds[(a.split()[0] if a != "?" else "?", b.split()[0] if b != "?" else "?")] += 1
    if shown < N:
        print(f"{name:32s} size {size:5d} at {hex(ob + s)} diff@+{d:#x}: orig [{a}]  ours [{b}]")
        shown += 1
print("mnemonic pairs (orig, ours):", kinds.most_common(12))
