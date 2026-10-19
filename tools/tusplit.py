#!/usr/bin/env python3
"""Propose translation-unit (source file) boundaries for Miyoo's own code; writes docs/tu-map.tsv.

Usage: tusplit.py [--exports DIR] [--out docs/tu-map.tsv] [--report]

What GCC 8 -O0 does (checked with tools/probe/std_probe.cpp):
- .text holds a file's functions in source order; the static-init pair
  (__static_initialization_and_destruction_0, _GLOBAL__sub_I_*) comes last.
- Template and in-class inline functions are COMDAT sections placed right after the file's .text,
  so a file is [its functions][its first-use template/inline copies].
- String literals go to plain .rodata, de-duplicated within a file and never merged across files.

- Each file's .rodata is one block and blocks follow file order, so string addresses only grow
  from one file to the next.

Signals, strongest first:
1. Same file (hard span, never cut inside): two functions use the same string address, or a later
   function uses a string that lies before a string an earlier function used (rodata inversion).
2. Fixed boundaries: C runtime, cJSON, SQLite, libgcc.
3. A static-init pair ends a file's own functions, but its template/inline tail still follows, so it
   is evidence for the unit, not a cut.
4. Any run of >= MIN_RUN probe-matched template functions ends a file, unless a hard span crosses it;
   a resulting unit that looks like more tail (< BODY_MIN bytes of non-template code, or no
   non-template function of BIG_FN bytes or more) is merged back into the file before it.
Everything between is left to review; the evidence column says why each cut exists.
"""
import argparse
import bisect
import collections
import csv
import os
import re
import subprocess
import tempfile

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
csv.field_size_limit(10**8)
MIN_RUN = 3
BODY_MIN = 0x200
BIG_FN = 0x100

# Reviewed corrections (checked by hand with packet.py). CUT_ADD: a file starts here.
# CUT_DROP: an automatic cut that is wrong. Keep the reason; it goes into the evidence column.
CUT_ADD = {
    0x049c3c: "reviewed: first file's tail ends with std::__copy_move helpers at 0x49b14/0x49b74; "
              "onSIGTERM starts the main-program file",
    0x138c50: "reviewed: KeySettingWindow's inline virtuals (0x138ba4-0x138c30) end its tail; "
              "Wi-Fi code and its string span start here",
}
CUT_DROP = {
    0x049b14: "reviewed: std::__copy_move<..>::__copy_m for a 1-byte type, still the first file's tail",
}
# Units proven as one file (tools/unitcheck.py): both ends are cuts, nothing inside is.
UNIT_FIXED = {
    (0x143720, 0x1484d4): "unitcheck: src/tu_143720.cpp body + 170 template/inline functions fill exactly "
                          "0x143720-0x1484d4; 0x1484d4 starts the next file",
    (0x1484d4, 0x14acfc): "unitcheck: src/tu_1484d4.cpp body + 60 template/inline functions fill exactly "
                          "0x1484d4-0x14acfc; 0x14acfc starts the next file",
}
# libstdc++'s own messages: a string span made only of these says nothing about the user's files
LIB_STR = re.compile(r"^(vector::|basic_string::|deque::|map::|cannot create std::)")

ap = argparse.ArgumentParser()
ap.add_argument("--exports", default=os.environ.get("MAINUI_EXPORTS", os.path.expanduser("~/mainui-exports")))
ap.add_argument("--out", default=os.path.join(ROOT, "docs/tu-map.tsv"))
ap.add_argument("--orig", default=os.path.join(ROOT, "MainUI-354-clean"))
ap.add_argument("--report", action="store_true", help="also print one line per function with its marks")
a = ap.parse_args()

# Fixed regions (docs/program-map.md). Miyoo code starts after the C runtime (_start, *_tm_clones,
# __do_global_dtors_aux, frame_dummy), which ends at frame_dummy (Ghidra's _INIT_0) at 0x15b1c.
FIXED = [(0x14d20, 0x15b20, "runtime", "PLT, _start, crtbegin"),
         (0x51b60, 0x57920, "lib_cjson", "cJSON 1.7.10, byte-exact (verify-libs.sh)"),
         (0x57920, 0x11cdc8, "lib_sqlite", "SQLite 3.30.1, byte-exact (verify-libs.sh)")]
MIYOO = [(0x15b20, 0x51b60), (0x11cdc8, 0x150000)]


def tsv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


def in_miyoo(v):
    return any(lo <= v < hi for lo, hi in MIYOO)


# Function starts: names.tsv plus Ghidra's table, as in progress.py; size = distance to the next start
names = {}
with open(os.path.join(ROOT, "ghidra/names.tsv")) as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= 4 and re.fullmatch(r"[0-9a-f]{8}", p[0]):
            names[int(p[0], 16)] = (p[1], p[3])
funcs = {int(f["entry"], 16): f for f in tsv(os.path.join(a.exports, "functions.tsv"))
         if f["is_external"] != "true" and f["is_thunk"] != "true"}
starts = sorted(s for s in set(names) | set(funcs) if in_miyoo(s))
ends = {}
for i, s in enumerate(starts):
    hi = next(h for lo, h in MIYOO if lo <= s < h)
    ends[s] = min(starts[i + 1], hi) if i + 1 < len(starts) else hi

# 1. Hard spans from shared string addresses
users, fstr, own = collections.defaultdict(set), collections.defaultdict(list), collections.defaultdict(set)
for r in tsv(os.path.join(a.exports, "strings.tsv")):
    if r["func_entry"] and re.fullmatch(r"[0-9a-f]{8}", r["str_addr"]):
        v = int(r["func_entry"], 16)
        if in_miyoo(v):
            users[r["str_addr"]].add(v)
            fstr[v].append(int(r["str_addr"], 16))
            if not LIB_STR.match(r["value"]):
                own[v].add(r["value"])
pairs = [(min(u), max(u)) for u in users.values() if len(u) > 1]
ordered = sorted(fstr)
for j, g in enumerate(ordered):              # rodata inversion: g uses a string below one f used
    lo_g = min(fstr[g])
    for f in ordered[:j]:
        if max(fstr[f]) > lo_g:
            pairs.append((f, g))
            break
spans = []
for lo, hi in sorted(pairs):
    if spans and lo <= spans[-1][1]:
        spans[-1][1] = max(spans[-1][1], hi)
    else:
        spans.append([lo, hi])


def inside_span(cut):
    return any(lo < cut <= hi for lo, hi in spans)


# Compile the probe and collect each function's bytes with link-time words masked
tc = os.environ.get("MIYOO_TC", "")
pre = os.path.join(tc, "arm-linux-gnueabihf-") if tc else "arm-linux-gnueabihf-"
obj = tempfile.mktemp(suffix=".o")
subprocess.run([pre + "g++", "-O0", "-fsigned-char", "-c", os.path.join(ROOT, "tools/probe/std_probe.cpp"),
                "-o", obj], check=True)
o = ELFFile(open(obj, "rb"))
secs = list(o.iter_sections())
probe = collections.defaultdict(list)       # size -> [(name, code, mask)]
sinit_shape = None
for s in o.get_section_by_name(".symtab").iter_symbols():
    if s["st_info"]["type"] != "STT_FUNC" or s["st_size"] == 0:
        continue
    idx = s["st_shndx"]
    off, size = s["st_value"] & ~1, s["st_size"]
    code = secs[idx].data()[off:off + size]
    mask = bytearray(size)
    for rs in secs:
        if isinstance(rs, RelocationSection) and rs["sh_info"] == idx:
            for rel in rs.iter_relocations():
                p = rel["r_offset"] - off
                if 0 <= p < size:
                    mask[p:p + 4] = b"\1\1\1\1"
    for i in range(0, size - 3, 4):
        hb = code[i + 3]
        if ((hb & 0x0f) == 0x0b and (hb >> 4) != 0xf) or hb in (0xfa, 0xfb):
            mask[i:i + 4] = b"\1\1\1\1"
    if s.name.startswith("_GLOBAL__sub_I_"):
        sinit_shape = (code, bytes(mask))
    elif secs[idx].name.startswith(".text._Z"):
        probe[size].append((s.name, code, bytes(mask)))
os.remove(obj)

e = ELFFile(open(a.orig, "rb"))
text = e.get_section_by_name(".text")
tdata, tbase = text.data(), text["sh_addr"]


def same(addr, code, mask):
    b = tdata[addr - tbase:addr - tbase + len(code)]
    return len(b) == len(code) and all(mask[i] or b[i] == code[i] for i in range(len(code)))


tmpl, sinit = {}, set()
for s in starts:
    size = ends[s] - s
    for nm, code, mask in probe.get(size, []):
        if same(s, code, mask):
            tmpl[s] = nm
            break
    if sinit_shape and size == len(sinit_shape[0]) and same(s, *sinit_shape):
        sinit.add(s)                         # _GLOBAL__sub_I_*: the static-init pair ends here

# Cuts
cuts = {}                                    # address -> evidence
for lo, hi, _, why in FIXED:
    cuts.setdefault(lo, why)
cuts[0x15b20] = "end of C runtime"
cuts.setdefault(0x51b60, "cJSON 1.7.10")
cuts.setdefault(0x11cdc8, "end of SQLite")
cuts.setdefault(0x150000, "libgcc / C runtime helpers")
run = []
for s in starts + [None]:
    if s is not None and s in tmpl:
        run.append(s)
        continue
    if len(run) >= MIN_RUN and s is not None and s not in cuts and not inside_span(s):
        cuts[s] = f"after template run {run[0]:#x}-{ends[run[-1]]:#x} ({len(run)} fns)"
    run = []

for c in CUT_DROP:
    cuts.pop(c, None)
for c, why in CUT_ADD.items():
    cuts[c] = why
for (ulo, uhi), why in UNIT_FIXED.items():
    for c in [c for c in cuts if ulo < c < uhi]:
        del cuts[c]
    cuts[ulo] = why
    cuts.setdefault(uhi, "end of " + why.split(":")[0] + f" unit {ulo:#x}")

# Merge template-only fragments back into the file before them
fixed_cuts = {x for f in FIXED for x in f[:2]} | {0x11cdc8, 0x150000} | set(CUT_ADD) \
    | {x for u in UNIT_FIXED for x in u}
changed = True
while changed:
    changed = False
    b = sorted(cuts)
    for lo, hi in zip(b, b[1:]):
        if lo in fixed_cuts:
            continue
        fs = [s for s in starts if lo <= s < hi]
        own_code = [ends[s] - s for s in fs if s not in tmpl]
        tail_like = sum(own_code) < BODY_MIN or max(own_code, default=0) < BIG_FN
        if tail_like and not any(lo <= x < hi for x, _ in spans):
            del cuts[lo]
            changed = True
            break

# Units
bounds = sorted(cuts)
rows = []
for lo, hi in zip(bounds, bounds[1:]):
    fixed = next((f for f in FIXED if f[0] == lo), None)
    fs = [s for s in starts if lo <= s < hi]
    if fixed:
        rows.append((lo, hi, fixed[2], "fixed", fixed[3], ""))
        continue
    if not fs:
        continue
    classes = collections.Counter(n.split("::")[0] for s in fs for n, c in [names.get(s, ("", ""))]
                                  if "::" in n and c != "guess")
    named = [n for s in fs for n, c in [names.get(s, ("", ""))] if c in ("confirmed", "inferred")]
    label = ", ".join(c for c, _ in classes.most_common(3)) or ", ".join(named[:3])
    ev = [cuts[lo]]
    sp = [f"{x:#x}-{y:#x}" for x, y in spans if lo <= x and y < hi]
    if sp:
        ev.append("string spans " + " ".join(sp))
    nt, ns = sum(1 for s in fs if s in tmpl), sum(1 for s in fs if s in sinit)
    si = [s for s in fs if s in sinit]
    ev.append(f"{len(fs)} fns, {nt} template" + (f", static init at {' '.join(hex(x) for x in si)}" if si else ""))
    for x in si:                             # the user's own strings after a static init suggest a second file
        after = [(y, z) for y, z in spans if x < y < hi
                 and any(own.get(t) for t in fs if y <= t <= z)]
        if after:
            ev.append(f"CHECK: own string span {after[0][0]:#x}-{after[0][1]:#x} follows static init {x:#x}; "
                      f"possible split near {after[0][0]:#x}")
    conf = "proven" if (lo, hi) in UNIT_FIXED else "reviewed" if lo in CUT_ADD else "review"
    rows.append((lo, hi, f"tu_{lo:06x}", conf, "; ".join(ev), label))

os.makedirs(os.path.dirname(a.out), exist_ok=True)
with open(a.out, "w") as f:
    f.write("lo\thi\tname\tconfidence\tevidence\tlabel\n")
    for lo, hi, n, c, ev, lab in rows:
        f.write(f"{lo:06x}\t{hi:06x}\t{n}\t{c}\t{ev}\t{lab}\n")
print(f"{len(starts)} Miyoo functions, {len(spans)} string spans, {len(tmpl)} template matches "
      f"({len(set(tmpl.values()))} distinct), {len(sinit)} static-init pairs -> {len(rows)} units")
print(f"wrote {os.path.relpath(a.out, ROOT)}")
if a.report:
    for s in starts:
        mark = "T " + tmpl[s] if s in tmpl else ("S" if s in sinit else "")
        print(f"{s:06x} {ends[s] - s:5d} {names.get(s, ('?', ''))[0][:40]:40s} {mark}")
