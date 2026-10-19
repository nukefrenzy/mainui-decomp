"""Recover Itanium C++ ABI class info (typeinfo, base classes, vtables) from a non-PIE ARM ELF.

Usage: python3 -I rtti.py <elf> <out.json>
"""
import json
import re
import struct
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

elf_path, out_path = sys.argv[1], sys.argv[2]
f = open(elf_path, "rb")
elf = ELFFile(f)

segs = [s for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]


def read(addr, n):
    for s in segs:
        if s["p_vaddr"] <= addr < s["p_vaddr"] + s["p_filesz"]:
            f.seek(s["p_offset"] + addr - s["p_vaddr"])
            return f.read(n)
    return None


def u32(addr):
    b = read(addr, 4)
    return struct.unpack("<I", b)[0] if b and len(b) == 4 else None


def cstr(addr, maxlen=256):
    b = read(addr, maxlen)
    if not b:
        return None
    return b.split(b"\0")[0].decode("latin1")


# Relocations: address -> symbol name (typeinfo vptrs point at libstdc++ vtables)
dynsym = elf.get_section_by_name(".dynsym")
reloc_at = {}
for sec in elf.iter_sections():
    if isinstance(sec, RelocationSection):
        for r in sec.iter_relocations():
            if r["r_info_sym"]:
                reloc_at[r["r_offset"]] = dynsym.get_symbol(r["r_info_sym"]).name

text = elf.get_section_by_name(".text")
text_lo, text_hi = text["sh_addr"], text["sh_addr"] + text["sh_size"]

# Find typeinfo name strings in .rodata: "<len><Name>" mangled forms
ro = elf.get_section_by_name(".rodata")
ro_data, ro_base = ro.data(), ro["sh_addr"]
name_re = re.compile(rb"(?<=\0)((?:N?\d+[A-Za-z_][A-Za-z0-9_]*)+E?)\0")
names = {}
for m in name_re.finditer(ro_data):
    s = m.group(1).decode()
    mm = re.fullmatch(r"(\d+)([A-Za-z_][A-Za-z0-9_]*)", s)
    if mm and int(mm.group(1)) == len(mm.group(2)):
        names[ro_base + m.start(1)] = mm.group(2)

# Scan data sections for words pointing at those names -> typeinfo objects
data_secs = [s for s in elf.iter_sections() if s.name in (".data.rel.ro", ".data", ".rodata")]
word_index = {}  # value -> list of addresses holding it
for s in data_secs:
    d, base = s.data(), s["sh_addr"]
    for off in range(0, len(d) - 3, 4):
        v = struct.unpack_from("<I", d, off)[0]
        word_index.setdefault(v, []).append(base + off)

KIND = {
    "_ZTVN10__cxxabiv117__class_type_infoE": "class",
    "_ZTVN10__cxxabiv120__si_class_type_infoE": "single",
    "_ZTVN10__cxxabiv121__vmi_class_type_infoE": "multi",
}
# Non-PIE: libstdc++ vtables are R_ARM_COPY'd into the executable; typeinfo vptr = copy + 8
VPTR_KIND = {}
for sec in elf.iter_sections():
    if isinstance(sec, RelocationSection):
        for r in sec.iter_relocations():
            if r["r_info_type"] == 20 and r["r_info_sym"]:  # R_ARM_COPY
                nm = dynsym.get_symbol(r["r_info_sym"]).name
                if nm in KIND:
                    VPTR_KIND[r["r_offset"] + 8] = KIND[nm]
PLT = {}
for sec in elf.iter_sections():
    if sec.name == ".plt":
        plt_lo, plt_hi = sec["sh_addr"], sec["sh_addr"] + sec["sh_size"]
classes = {}
ti_addr_to_name = {}
for naddr, cname in names.items():
    for holder in word_index.get(naddr, []):
        ti = holder - 4
        kind = VPTR_KIND.get(u32(ti), None)
        if kind is None:
            continue
        ti_addr_to_name[ti] = cname
        classes[cname] = {"typeinfo": hex(ti), "kind": kind, "bases": [], "_ti": ti}

for cname, c in classes.items():
    ti = c["_ti"]
    if c["kind"] == "single":
        b = u32(ti + 8)
        c["bases"] = [ti_addr_to_name.get(b, hex(b) if b else "?")]
    elif c["kind"] == "multi":
        count = u32(ti + 12) or 0
        for i in range(min(count, 16)):
            b = u32(ti + 16 + i * 8)
            c["bases"].append(ti_addr_to_name.get(b, hex(b) if b else "?"))

# Vtables: [offset_to_top=0][typeinfo ptr][fn ptrs...]
for cname, c in classes.items():
    ti = c["_ti"]
    vts = []
    for holder in word_index.get(ti, []):
        if u32(holder - 4) not in (0, None) and u32(holder - 4) < 0x80000000:
            continue  # offset_to_top must be 0 or negative for secondary vtables
        entries = []
        a = holder + 4
        while True:
            v = u32(a)
            if v is None:
                break
            fn = v & ~1
            if v == 0 and entries:
                entries.append(None)  # pure virtual / null slot
            elif text_lo <= fn < text_hi:
                entries.append(hex(fn))
            elif plt_lo <= v < plt_hi:
                entries.append("pure")
            else:
                break
            a += 4
            if len(entries) > 200:
                break
        while entries and entries[-1] is None:
            entries.pop()
        if entries:
            vts.append({"vtable": hex(holder - 4), "offset_to_top": u32(holder - 4), "entries": entries})
    c["vtables"] = vts
    del c["_ti"]

json.dump(classes, open(out_path, "w"), indent=1)
print(len(classes), "classes;", sum(len(c["vtables"]) for c in classes.values()), "vtables")
for n, c in sorted(classes.items()):
    print(f'{n:28s} {c["kind"]:6s} bases={",".join(c["bases"]) or "-":28s} '
          f'vslots={[len(v["entries"]) for v in c["vtables"]]}')
