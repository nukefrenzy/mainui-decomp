#!/usr/bin/env python3
"""Check a whole translation unit against MainUI, the way the linker lays it out.

Usage: unitcheck.py <src/tu_xxxxxx.cpp> [--unit NAME | --at ADDR] [--end ADDR] [--verbose] [--body-only]

GNU ld places each object's .text first, then its template/inline COMDAT sections (.text._Z*)
in the order they appear in the object; a COMDAT that an earlier file already contributed is
discarded. So:
1. The object's .text must match word for word starting at the unit's start (link-time words
   masked as in matchfn.py: relocations and BL/BLX targets).
2. The .text._Z* sections, walked in object order, must each either match right where the
   previous one ended or exist identically before the unit (discarded: kept from an earlier
   file). Many tiny templates have identical code (std::forward, std::move, trivial ctors), so
   the walk backtracks over that choice. The end of the walk is the unit's real end; it is
   compared with docs/tu-map.tsv. Proven units must finish at their recorded end; --end can
   require a known neighbouring boundary while proving a draft unit. This forces backtracking
   past short layouts that discard byte-identical pointer templates found in earlier files.
3. If no such layout exists, the walk reports where it breaks, and each section is then matched
   to its first identical code in the tail and listed in binary order with its object order, so an
   ordering difference shows up as "out of order" rather than as a mismatch. Identical small
   functions can be paired up wrongly there; only the walk's verdict is exact.
--body-only checks a unit that is still being written: it compares only the object's .text from the
unit start, reports how far it matches (the first mismatch is normally the first unwritten
function) and ignores the tail. Exit code 0 = the whole .text matches, 1 = it stops earlier.
Exit code 0 = whole unit matches, 1 = mismatch, 2 = error.
"""
import argparse
import bisect
import csv
import os
import re
import subprocess
import sys
import tempfile

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ap = argparse.ArgumentParser()
ap.add_argument("source")
ap.add_argument("--unit", help="unit name in docs/tu-map.tsv (default: from the file name tu_<lo>)")
ap.add_argument("--at", help="hex start address instead of a unit")
ap.add_argument("--end", help="required hex end address (default: enforce the end of proven units)")
ap.add_argument("--orig", default=os.path.join(ROOT, "MainUI-354-clean"))
ap.add_argument("--flags", default="-O0 -fsigned-char")
ap.add_argument("--verbose", action="store_true")
ap.add_argument("--body-only", action="store_true", help="compare .text only and report the matching prefix")
ap.add_argument("--window", type=lambda x: int(x, 0), default=0x10000, help="how far past .text to look for the tail")
a = ap.parse_args()

units = {}
proven = set()
tu_path = os.path.join(ROOT, "docs/tu-map.tsv")
if os.path.exists(tu_path):
    with open(tu_path, newline="") as f:
        for t in csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE):
            units[t["name"]] = (int(t["lo"], 16), int(t["hi"], 16))
            if t.get("confidence") == "proven":
                proven.add(t["name"])
if a.at:
    lo, hi, uname = int(a.at, 16), None, f"@{a.at}"
else:
    uname = a.unit or os.path.splitext(os.path.basename(a.source))[0]
    if uname not in units:
        sys.exit(f"unit {uname} not in docs/tu-map.tsv (use --unit or --at)")
    lo, hi = units[uname]
required_end = int(a.end, 16) if a.end else (hi if uname in proven else None)

names = {}
with open(os.path.join(ROOT, "ghidra/names.tsv")) as f:
    for line in f:
        p = line.split("\t")
        if re.fullmatch(r"[0-9a-f]{8}", p[0]):
            names[int(p[0], 16)] = p[1]

# Per-file headers and extra compiler flags from src/matches.tsv. The CLI --flags supplies
# the base flags; cflags is appended, just as in run_matches.sh. Old manifests default to "-".
deps = set()
file_flags = set()
with open(os.path.join(ROOT, "src/matches.tsv"), newline="") as f:
    for m in csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE):
        if m["file"] == os.path.basename(a.source):
            file_flags.add(m.get("cflags", "-") or "-")
            if m.get("deps", "-") != "-":
                deps |= set(m["deps"].split(","))
if len(file_flags) > 1:
    sys.exit(f"inconsistent cflags for {os.path.basename(a.source)} in src/matches.tsv")
extra = []
flags = next(iter(file_flags), "-")
if flags != "-":
    extra.extend(flags.split())
if "tinyutf8" in deps:
    extra.append("-I" + subprocess.run([os.path.join(ROOT, "scripts/fetch-tinyutf8.sh")], capture_output=True,
                                       text=True, check=True).stdout.strip())
if "rapidxml" in deps:
    extra.append("-I" + subprocess.run([os.path.join(ROOT, "scripts/fetch-rapidxml.sh")], capture_output=True,
                                       text=True, check=True).stdout.strip())

tc = os.environ.get("MIYOO_TC", "")
pre = os.path.join(tc, "arm-linux-gnueabihf-") if tc else "arm-linux-gnueabihf-"
cc = pre + ("g++" if a.source.endswith((".cpp", ".cc", ".cxx")) else "gcc")
obj = tempfile.mktemp(suffix=".o")
r = subprocess.run([cc, *a.flags.split(), *extra, "-c", a.source, "-o", obj], capture_output=True, text=True)
if r.returncode:
    print(r.stderr)
    sys.exit(2)

o = ELFFile(open(obj, "rb"))
secs = list(o.iter_sections())
syms = {}                                   # section index -> [(offset, size, demangled name)]
raw = [(s["st_shndx"], s["st_value"] & ~1, s["st_size"], s.name)
       for s in o.get_section_by_name(".symtab").iter_symbols()
       if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]]
dem = subprocess.run([pre + "c++filt"], input="\n".join(x[3] for x in raw), capture_output=True,
                     text=True).stdout.splitlines()
for (idx, off, size, _), d in zip(raw, dem):
    syms.setdefault(idx, []).append((off, size, d))


def masked(idx):
    data = secs[idx].data()
    mask = bytearray(len(data))
    for rs in secs:
        if isinstance(rs, RelocationSection) and rs["sh_info"] == idx:
            for rel in rs.iter_relocations():
                mask[rel["r_offset"]:rel["r_offset"] + 4] = b"\1\1\1\1"
    for i in range(0, len(data) - 3, 4):
        hb = data[i + 3]
        if ((hb & 0x0f) == 0x0b and (hb >> 4) != 0xf) or hb in (0xfa, 0xfb):
            mask[i:i + 4] = b"\1\1\1\1"
    return data, bytes(mask)


order = [i for i, s in enumerate(secs) if s["sh_type"] == "SHT_PROGBITS" and s["sh_flags"] & 0x4
         and (s.name == ".text" or s.name.startswith(".text."))]
odd = [secs[i].name for i in order if re.match(r"\.text\.(startup|unlikely|hot|exit)", secs[i].name)]
if odd:
    print("warning: sections the linker script places elsewhere: " + ", ".join(odd))

e = ELFFile(open(a.orig, "rb"))
text = e.get_section_by_name(".text")
TD, TB = text.data(), text["sh_addr"]


def diff_at(addr, data, mask):
    """Offsets (multiples of 4) where data differs from the original at addr, link-time words masked."""
    b = TD[addr - TB:addr - TB + len(data)]
    if len(b) < len(data):
        return [0]
    return [i for i in range(0, len(data), 4) if not any(mask[i:i + 4]) and b[i:i + 4] != data[i:i + 4]]


def where(idx, off):
    for s_off, size, d in syms.get(idx, []):
        if s_off <= off < s_off + size:
            return f"{d} +{off - s_off:#x}"
    return f"+{off:#x}"


earlier = sorted(s for s in names if s < lo)
starts_all = sorted(names)
fail = 0
cur = lo
idx = next((i for i in order if secs[i].name == ".text"), None)
if idx is not None:
    data, mask = masked(idx)
    d = diff_at(cur, data, mask)
    if d:
        fail = 1
        off = d[0]
        w = lambda b, i: b[i:i + 4][::-1].hex()
        print(f"BODY   MISMATCH at {cur + off:#x} ({where(idx, off)}): {len(d)} of {len(data) // 4} words differ")
        print(f"       original {w(TD, cur - TB + off)}  ours {w(data, off)}")
    else:
        print(f"BODY   OK  {cur:#x}-{cur + len(data):#x} ({len(data)} bytes, {len(syms.get(idx, []))} functions)")
    if a.body_only:
        # count whole functions only: a mismatching function can still share its prologue
        bad = d[0] if d else len(data)
        good = max((off for off, _, _ in syms.get(idx, []) if off <= bad), default=0) if d else len(data)
        nfun = sum(1 for off, _, _ in syms.get(idx, []) if off < good)
        print(f"PREFIX {cur:#x}-{cur + good:#x} matches: {nfun} whole functions, {good} of {len(data)} bytes")
        if d:
            orig = max((x for x in names if x <= cur + good), default=cur + good)
            print(f"       next: ours {where(idx, good)}, original {names.get(orig, '?')} at {orig:#x}"
                  f"{'' if orig == cur + good else f' +{cur + good - orig:#x}'}")
        os.remove(obj)
        sys.exit(1 if d else 0)
    cur += len(data)
tail_lo = cur
# every word in the window is a candidate start: names.tsv does not list every function
region_end = min(tail_lo + a.window, TB + len(TD))
if required_end is not None:
    region_end = min(region_end, required_end)


def earlier_hits(data, mask):
    """Identical code before the unit, at any word (names.tsv does not list every function)."""
    first = None if any(mask[0:4]) else data[0:4]
    for x in range(TB, lo - len(data) + 1, 4):
        if (first is None or TD[x - TB:x - TB + 4] == first) and not diff_at(x, data, mask):
            yield x


def tail_hits(data, mask):
    first = None if any(mask[0:4]) else data[0:4]
    for x in range(tail_lo, region_end - len(data) + 1, 4):
        if first is not None and TD[x - TB:x - TB + 4] != first:
            continue
        if x not in used and not diff_at(x, data, mask):
            yield x
tail_secs = []                               # (k, data, mask, name) in object order
for k, idx in enumerate(i for i in order if secs[i].name != ".text"):
    data, mask = masked(idx)
    if data:
        tail_secs.append((k, data, mask, ", ".join(x[2] for x in syms.get(idx, [])) or secs[idx].name))


def linker_walk():
    """Lay the tail out as ld does: each section in object order either goes at the current end
    or was discarded because an earlier file has it. Many tiny template functions have identical
    code (std::forward/std::move, trivial ctors and dtors returning `this`), so a section can both
    match here and exist earlier; try placing first and backtrack. Returns (plan, deepest) where plan
    is [(k, addr or None)] or None, and deepest = (i, cur) of the furthest failure."""
    sys.setrecursionlimit(max(1000, 4 * len(tail_secs)))
    dead, deepest = set(), [(-1, tail_lo)]
    early = {}

    def go(i, cur):
        if i == len(tail_secs):
            return [] if required_end is None or cur == required_end else None
        if (i, cur) in dead:
            return None
        k, data, mask, _ = tail_secs[i]
        if cur + len(data) <= region_end and not diff_at(cur, data, mask):
            rest = go(i + 1, cur + len(data))
            if rest is not None:
                return [(k, cur)] + rest
        if i not in early:
            early[i] = next(earlier_hits(data, mask), None)
        if early[i] is not None:
            rest = go(i + 1, cur)
            if rest is not None:
                return [(k, None, early[i])] + rest
        dead.add((i, cur))
        if i > deepest[0][0]:
            deepest[0] = (i, cur)
        return None

    return go(0, tail_lo), deepest[0]


plan, (fail_i, fail_cur) = linker_walk()
if plan is not None:
    cur, kept = tail_lo, 0
    for k, at, *old in plan:
        nm = next(t[3] for t in tail_secs if t[0] == k)
        if at is None:
            kept += 1
            if a.verbose:
                print(f"KEPT   earlier at {old[0]:#x} ({names.get(old[0], '?')}): {nm[:80]}")
        else:
            size = len(next(t[1] for t in tail_secs if t[0] == k))
            if a.verbose:
                print(f"TAIL   {at:#x} {size:4d} B  obj#{k:<3d} {nm[:80]}")
            cur = at + size
    print(f"TAIL   {len(plan) - kept} in object order, {kept} kept from earlier files")
    note = "" if hi is None else (" (matches docs/tu-map.tsv)" if cur == hi else f" (docs/tu-map.tsv says {hi:#x})")
    print(f"UNIT   {uname}: {lo:#x}-{cur:#x}{note}")
    print("RESULT", "MATCH" if not fail else "MISMATCH")
    os.remove(obj)
    sys.exit(fail)
fail = 1  # Diagnostic matches below cannot turn a failed linker walk into a proof.
if not tail_secs:
    print(f"UNIT   {uname}: {lo:#x}-{tail_lo:#x}; required end {required_end:#x}")
    print("RESULT MISMATCH")
    os.remove(obj)
    sys.exit(1)
if required_end is not None:
    print(f"END    linker walk must finish at {required_end:#x}")
k, _, _, nm = tail_secs[fail_i]
print(f"TAIL   linker order breaks at {fail_cur:#x}: obj#{k} {nm[:80]} is neither there nor earlier")
print("       (diagnostics below match each section to its first identical code in the tail)")

used, placed, discarded, bad = {}, [], 0, 0
starts_all = starts_all
for k, idx in enumerate(i for i in order if secs[i].name != ".text"):
    data, mask = masked(idx)
    if not data:
        continue
    nm = ", ".join(x[2] for x in syms.get(idx, [])) or secs[idx].name
    hit = next(tail_hits(data, mask), None)
    if hit is not None:
        used[hit] = (k, len(data), nm)
        placed.append(hit)
        continue
    old = next(earlier_hits(data, mask), None)
    if old is not None:
        discarded += 1
        if a.verbose:
            print(f"KEPT   earlier at {old:#x} ({names.get(old, '?')}): {nm[:80]}")
    else:
        bad += 1
        fail = 1
        print(f"TAIL   MISMATCH: {nm[:100]} ({len(data)} B) is not in the tail or earlier")
# walk the tail in binary order: contiguous run of placed sections from the end of .text
cur, last_k, seq = tail_lo, -1, []
while cur in used:
    k, size, nm = used[cur]
    seq.append(k)
    if a.verbose:
        print(f"TAIL   {cur:#x} {size:4d} B  obj#{k:<3d}{'' if k > last_k else ' (out of order)'}  {nm[:80]}")
    last_k = max(last_k, k)
    cur += size
stray = sorted(x for x in used if x >= cur)
for x in list(stray):
    data, mask = masked(next(i for i in order if ", ".join(y[2] for y in syms.get(i, [])) == used[x][2]))
    old = next(earlier_hits(data, mask), None)
    if old is not None:
        discarded += 1
        stray.remove(x)
        if a.verbose:
            print(f"KEPT   earlier at {old:#x} ({names.get(old, '?')}): {used[x][2][:80]} (also matched {x:#x})")
        del used[x]
for x in stray:
    print(f"TAIL   STRAY  {x:#x} {used[x][1]:4d} B  obj#{used[x][0]:<3d} {used[x][2][:90]}")
if stray:
    fail = 1
    print(f"TAIL   GAP at {cur:#x} ({names.get(cur, '?')}): not produced by this file; "
          f"{len(stray)} more of our sections lie beyond it ({stray[0]:#x}-{stray[-1]:#x})")
tails = []                                   # longest increasing subsequence = sections already in order
for k in seq:
    i = bisect.bisect_left(tails, k)
    tails[i:i + 1] = [k]
displaced = len(seq) - len(tails)
if displaced:
    fail = 1
print(f"TAIL   {len(seq)} contiguous, {displaced} displaced from object order, "
      f"{discarded} kept from earlier files, {bad} mismatched")
note = "" if hi is None else (" (matches docs/tu-map.tsv)" if cur == hi else f" (docs/tu-map.tsv says {hi:#x})")
print(f"UNIT   {uname}: {lo:#x}-{cur:#x}{note}")
print("RESULT", "MATCH" if not fail else "MISMATCH")
os.remove(obj)
sys.exit(fail)
