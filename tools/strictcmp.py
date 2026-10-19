"""Strict check: object's whole .text vs original at a base address; only relocated words masked.
Usage: strictcmp.py <orig> <obj.o> <base_hex>"""
import sys
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
orig, objp, base = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
o = ELFFile(open(objp, "rb")); ti = [i for i, s in enumerate(o.iter_sections()) if s.name == ".text"][0]
td = o.get_section(ti).data()
e = ELFFile(open(orig, "rb")); t = e.get_section_by_name(".text"); ed = t.data(); s0 = base - t["sh_addr"]
m = bytearray(len(td)); nrel = 0
for s in o.iter_sections():
    if isinstance(s, RelocationSection) and s["sh_info"] == ti:
        for r in s.iter_relocations(): m[r["r_offset"]:r["r_offset"] + 4] = b"\1\1\1\1"; nrel += 1
diff = [i for i in range(len(td)) if not m[i] and td[i] != ed[s0 + i]]
print(f"{len(td)} bytes compared at {hex(base)}..{hex(base+len(td))}; {nrel} relocated words masked; {len(diff)} differing bytes")
if diff: print("first diffs at offsets", [hex(i) for i in diff[:8]])
sys.exit(1 if diff else 0)
