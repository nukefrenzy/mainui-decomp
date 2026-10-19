"""Find which functions of a compiled ARM object appear byte-for-byte in an original executable.

Bytes covered by relocations in the object (call targets, literal-pool addresses, MOVW/MOVT
immediates) are masked out, since those are only filled in at link time.

Usage: python3 -I matchobj.py <original_elf> <object.o> [lo hi] [--list]
  lo/hi: hex address range of the original to search (default: whole .text)
Prints: matched / total functions and bytes; --list prints each function's result.
"""
import struct
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

args = [a for a in sys.argv[1:] if not a.startswith("--")]
orig_path, obj_path = args[0], args[1]
listing = "--list" in sys.argv

orig = ELFFile(open(orig_path, "rb"))
otext = orig.get_section_by_name(".text")
odata, obase = otext.data(), otext["sh_addr"]
lo = max(0, int(args[2], 16) - obase) if len(args) > 2 else 0
hi = int(args[3], 16) - obase if len(args) > 3 else len(odata)
region = odata[lo:hi]

obj = ELFFile(open(obj_path, "rb"))
text_idx = [i for i, s in enumerate(obj.iter_sections()) if s.name == ".text"][0]
text = obj.get_section(text_idx)
tdata = text.data()

mask = bytearray(len(tdata))  # 1 = ignore this byte
for sec in obj.iter_sections():
    if isinstance(sec, RelocationSection) and sec["sh_info"] == text_idx:
        for r in sec.iter_relocations():
            o = r["r_offset"]
            mask[o:o + 4] = b"\x01\x01\x01\x01"

# Calls between functions in the same object have no relocation; their offsets depend on
# layout. Mask every BL/BLX(imm) word (cond 0b1011 / 0xfa-0xfb) so layout doesn't matter.
for i in range(0, len(tdata) - 3, 4):
    hb = tdata[i + 3]
    if (hb & 0x0f) == 0x0b and (hb >> 4) != 0xf or hb in (0xfa, 0xfb):
        mask[i:i + 4] = b"\x01\x01\x01\x01"

funcs = []
for s in obj.get_section_by_name(".symtab").iter_symbols():
    if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] == text_idx and s["st_size"] > 0:
        funcs.append((s.name, s["st_value"] & ~1, s["st_size"]))

# Literal pools sit after a function's last instruction but inside st_size; ARM code is 4-aligned.
results = []
used = set()
for name, off, size in sorted(funcs, key=lambda f: f[1]):
    code, m = tdata[off:off + size], mask[off:off + size]
    # anchor: longest run of unmasked bytes (aligned), at least 8 bytes
    best, cur_start = (0, 0), None
    for i in range(0, size + 1, 4):
        unm = i < size and not any(m[i:i + 4])
        if unm and cur_start is None:
            cur_start = i
        if not unm and cur_start is not None:
            if i - cur_start > best[1] - best[0]:
                best = (cur_start, i)
            cur_start = None
    a0, a1 = best
    found = None
    if a1 - a0 >= 8:
        anchor = code[a0:a1]
        p = region.find(anchor)
        while p != -1 and found is None:
            start = p - a0
            if start >= 0 and (start & 3) == 0 and start + size <= len(region):
                cand = region[start:start + size]
                if all(m[i] or cand[i] == code[i] for i in range(size)):
                    found = obase + lo + start
            p = region.find(anchor, p + 1)
    elif size <= 8:
        found = None  # too small to place unambiguously
    results.append((name, size, found))

n_ok = sum(1 for r in results if r[2] is not None)
b_ok = sum(r[1] for r in results if r[2] is not None)
b_all = sum(r[1] for r in results)
print(f"{obj_path.split('/')[-1]}: {n_ok}/{len(results)} functions ({100*n_ok/max(1,len(results)):.1f}%), "
      f"{b_ok}/{b_all} bytes ({100*b_ok/max(1,b_all):.1f}%)")
if listing:
    for name, size, found in results:
        print(f"  {'OK ' if found else '-- '} {size:6d} {hex(found) if found else '':10s} {name}")
