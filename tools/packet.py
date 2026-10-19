#!/usr/bin/env python3
"""Write a compact brief ("packet") for one function, or for every unmatched function in a range,
so a rewrite can start from one small file instead of many lookups.

Usage:
  packet.py <addr> [<addr> ...] [--asm] [--stdout]
  packet.py --range <lo> <hi> [--asm]          all functions in [lo, hi) not yet in src/matches.tsv
  packet.py --tu <name>                        same, for a unit in docs/tu-map.tsv
Options: --exports DIR (default $MAINUI_EXPORTS or ~/mainui-exports), --out DIR (default build/packets),
         --max-lines N (cap on the decompiled body, default 90)

A packet holds: current name, size, unit, confidence, the vtable slots pointing at the function,
the Ghidra C with every function and global renamed to its current name, the field offsets used
on each parameter, callees (name, size, matched?), callers and strings. --asm adds the disassembly.
Packets contain decompiler output, so they go to build/ (gitignored) and are never committed.
"""
import argparse
import bisect
import csv
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
csv.field_size_limit(10**8)

ap = argparse.ArgumentParser()
ap.add_argument("addrs", nargs="*")
ap.add_argument("--range", nargs=2, metavar=("LO", "HI"))
ap.add_argument("--tu")
ap.add_argument("--asm", action="store_true")
ap.add_argument("--stdout", action="store_true")
ap.add_argument("--exports", default=os.environ.get("MAINUI_EXPORTS", os.path.expanduser("~/mainui-exports")))
ap.add_argument("--out", default=os.path.join(ROOT, "build/packets"))
ap.add_argument("--max-lines", type=int, default=90)
a = ap.parse_args()


def tsv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


def key(addr):
    return f"{int(addr, 16):08x}"


# Current names and confidences (ghidra/names.tsv), globals, Ghidra's function table
names, conf = {}, {}
with open(os.path.join(ROOT, "ghidra/names.tsv")) as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= 4 and re.fullmatch(r"[0-9a-f]{8}", p[0]):
            names[p[0]], conf[p[0]] = p[1], p[3]
glob = {key(g["addr"]): g["name"] for g in tsv(os.path.join(ROOT, "ghidra/globals.tsv"))}
funcs = {f["entry"]: f for f in tsv(os.path.join(a.exports, "functions.tsv"))}
matched = {key(m["addr"]): m["file"] for m in tsv(os.path.join(ROOT, "src/matches.tsv"))}


def name_of(addr):
    if addr in names:
        return names[addr]
    f = funcs.get(addr)
    return f["name"] if f else f"FUN_{addr}"


# Size = distance to the next function start, as in progress.py and in the compiled symbol
# (Ghidra's own size leaves out the literal pool). Ghidra's size is shown too when it differs.
starts = sorted({int(e, 16) for e, f in funcs.items() if f["is_external"] != "true"} | {int(e, 16) for e in names})


def size_of(addr):
    v = int(addr, 16)
    i = bisect.bisect_right(starts, v)
    return starts[i] - v if i < len(starts) else int(funcs.get(addr, {}).get("size", 0))


def ghidra_size(addr):
    return int(funcs[addr]["size"]) if addr in funcs else 0


# Vtable slots: which class/slot points at each function
slots = {}
for cls, info in json.load(open(os.path.join(ROOT, "ghidra/classes.json"))).items():
    for vt in info["vtables"]:
        for i, e in enumerate(vt["entries"]):
            slots.setdefault(key(e), []).append(f"{cls} slot {i} (+{4 * i:#x})")

# Translation units
tus = []
tu_path = os.path.join(ROOT, "docs/tu-map.tsv")
if os.path.exists(tu_path):
    tus = [(int(t["lo"], 16), int(t["hi"], 16), t["name"]) for t in tsv(tu_path)]


def tu_of(addr):
    v = int(addr, 16)
    return next((n for lo, hi, n in tus if lo <= v < hi), "-")


# Pick the functions to write
if a.tu:
    hit = [(lo, hi) for lo, hi, n in tus if n == a.tu]
    if not hit:
        sys.exit(f"unit {a.tu} not in docs/tu-map.tsv")
    lo, hi = hit[0]
elif a.range:
    lo, hi = int(a.range[0], 16), int(a.range[1], 16)
if a.tu or a.range:
    want = sorted(e for e, f in funcs.items() if lo <= int(e, 16) < hi and f["is_thunk"] != "true"
                  and f["is_external"] != "true" and e not in matched)
else:
    want = [key(x) for x in a.addrs]
if not want:
    sys.exit("nothing to do")
wanted = set(want)

# Calls and strings
callees, callers = {}, {}
for c in tsv(os.path.join(a.exports, "calls.tsv")):
    callers.setdefault(c["callee"], []).append(c["caller"])
    if c["caller"] in wanted:
        callees.setdefault(c["caller"], []).append(c["callee"])
strs = {}
for s in tsv(os.path.join(a.exports, "strings.tsv")):
    if s["func_entry"] in wanted:
        strs.setdefault(s["func_entry"], []).append(s["value"])

# Decompiled bodies (one pass over the big file; only the wanted functions are kept)
body, cur = {}, None
with open(os.path.join(a.exports, "MainUI-354-clean_named.c"), errors="replace") as f:
    for line in f:
        if line.startswith("//@@FUNC "):
            cur = line.split()[1]
            cur = cur if cur in wanted else None
            if cur:
                body[cur] = []
        elif cur:
            body[cur].append(line.rstrip("\n"))

# Rename every function/global reference to its current name. Ghidra's names in the export may be
# stale, so map each address-bearing token (FUN_/DAT_/<hint>_<addr>) by its address.
tok = re.compile(r"\b(?:[A-Za-z_][\w]*::)*[A-Za-z_~][\w]*\b")
by_old = {}
for e, f in funcs.items():
    by_old[f["name"]] = e
    by_old[f"{f['namespace']}::{f['name']}"] = e


def rename(text):
    def sub(m):
        t = m.group(0)
        e = by_old.get(t)
        if e is None:
            h = re.search(r"_([0-9a-f]{5,8})$", t)
            if h:
                e = h.group(1).zfill(8)
                if t.startswith("DAT_") and e in glob:
                    return glob[e]
                if e not in names and e not in funcs:
                    return t
        return names.get(e, t) if e else t
    return tok.sub(sub, text)


# Field offsets: "(x + 0x18)" and "x[3]" (index, element size unknown); "*x + 0x14" is a vtable slot
field_re = re.compile(r"(?<![*\w])([A-Za-z_]\w*)\s*\+\s*(0x[0-9a-f]+|\d+)\b")
index_re = re.compile(r"\b(param_\d+|this)\[(\d+)\]")
decl_re = re.compile(r"\b(\w+ \*+)\s*(\w+)\s*[,;)]")
plain_re = re.compile(r"\b(u?int|undefined4|u?long)\s+(\w+)\s*[,;)]")
SIZES = {"char": 1, "byte": 1, "undefined": 1, "bool": 1, "uchar": 1, "short": 2, "ushort": 2,
         "undefined2": 2, "int": 4, "uint": 4, "undefined4": 4, "long": 4, "ulong": 4, "float": 4,
         "code": 0, "undefined8": 8, "longlong": 8, "double": 8}


def elem_size(t):
    if not t:
        return 0
    base, stars = t.replace(" ", "").rstrip("*"), t.count("*")
    return 4 if stars > 1 else SIZES.get(base, 0)


vcall_re = re.compile(r"\*\s*\(?\s*(?:\(\w+ \*\*?\))?\s*([A-Za-z_]\w*)\s*\)?\s*\+\s*(0x[0-9a-f]+|\d+)\)\)\s*\(")


def disasm(addr, size):
    tc = os.environ.get("MIYOO_TC", "")
    objdump = os.path.join(tc, "arm-linux-gnueabihf-objdump") if tc else "arm-linux-gnueabihf-objdump"
    lo = int(addr, 16)
    out = subprocess.run([objdump, "-d", "--no-show-raw-insn", f"--start-address={lo:#x}",
                          f"--stop-address={lo + size:#x}", os.path.join(ROOT, "MainUI-354-clean")],
                         capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        p = line.split("\t")
        if len(p) >= 2 and p[0].strip().endswith(":"):
            ins = re.sub(r"<[^>]*>", "", "\t".join(p[1:])).strip()
            rows.append(f"{p[0].strip()[:-1]:>6}  {ins}")
    return rows


def packet(e):
    out = [f"# {name_of(e)} @ 0x{int(e, 16):x}", ""]
    gs = "" if ghidra_size(e) == size_of(e) else f" (Ghidra body {ghidra_size(e)})"
    out.append(f"- size {size_of(e)} bytes{gs}; unit {tu_of(e)}; confidence {conf.get(e, '-')}"
               + (f"; MATCHED in {matched[e]}" if e in matched else ""))
    if e in slots:
        out.append("- vtable: " + "; ".join(slots[e]))
    cs = callers.get(e, [])
    if cs:
        shown = ", ".join(dict.fromkeys(name_of(c) for c in cs[:6]))
        out.append(f"- callers ({len(cs)}): {shown}" + (" ..." if len(cs) > 6 else ""))
    lines, skip = [], False
    for l in body.get(e, []):           # drop the exporter's "[MainUI map]" comment (may span lines)
        if l.startswith("/* [MainUI map]"):
            skip = True
        if not skip:
            lines.append(l)
        elif "*/" in l:
            skip = False
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    lines = [rename(l) for l in lines]
    # Ghidra scales "p + n" and "p[n]" by the pointee size of p's declared type; "(int)p + n" is bytes
    joined = " ".join(lines)
    types = dict((v, t) for t, v in decl_re.findall(joined))
    for t, v in plain_re.findall(joined):     # integer-typed variables: offsets are already bytes
        types.setdefault(v, "byte *")
    offs, vslots = {}, set()
    for l in lines[1:]:          # skip the signature line
        for m in field_re.finditer(l):
            p, o = m.group(1), int(m.group(2), 0)
            if p.startswith(("DAT_", "PTR_")) or re.fullmatch(r"[0-9]+|0x\w+", p):
                continue
            cast = re.search(r"\((?:u?int|undefined4|u?long)\)\s*$", l[:m.start()])
            scale = 1 if cast else elem_size(types.get(p))
            offs.setdefault(p, set()).add(f"+{o * scale:#x}" if scale else f"+{o}?")
        for p, i in index_re.findall(l):
            sc = elem_size(types.get(p))
            offs.setdefault(p, set()).add(f"+{int(i) * sc:#x}" if sc else f"[{i}]?")
        for _, o in vcall_re.findall(l):
            vslots.add(int(o, 0) // 4)
    if offs:
        out.append("- fields: " + "; ".join(f"{p} " + " ".join(sorted(v, key=lambda x: (not x.startswith("+0x"), int(re.sub(r"[^0-9a-fx]", "", x) or "0", 0))))
                                            for p, v in sorted(offs.items())))
    if vslots:
        out.append("- virtual calls: slots " + ", ".join(f"{v} (+{4 * v:#x})" for v in sorted(vslots)))
    ce = list(dict.fromkeys(callees.get(e, [])))
    if ce:
        out += ["", "## Calls"]
        for c in ce:
            f = funcs.get(c, {})
            kind = "import" if f.get("is_thunk") == "true" or f.get("is_external") == "true" else f"{size_of(c)} B"
            flag = " MATCHED" if c in matched else ""
            out.append(f"- {name_of(c)} ({c.lstrip('0') or '0'}, {kind}){flag}")
    if strs.get(e):
        out += ["", "## Strings"] + [f"- {json.dumps(s)[:100]}" for s in dict.fromkeys(strs[e])]
    out += ["", "## Ghidra C", "```c"]
    if len(lines) > a.max_lines:
        out += lines[:a.max_lines] + [f"// ... {len(lines) - a.max_lines} more lines (--max-lines)"]
    else:
        out += lines
    out.append("```")
    if a.asm:
        out += ["", "## Disassembly", "```"] + disasm(e, size_of(e)) + ["```"]
    return "\n".join(out) + "\n"


if not a.stdout:
    os.makedirs(a.out, exist_ok=True)
index = []
for e in want:
    if e not in funcs:
        print(f"{e}: not a function start in functions.tsv", file=sys.stderr)
        continue
    text = packet(e)
    if a.stdout:
        sys.stdout.write(text + "\n")
    else:
        with open(os.path.join(a.out, f"{e}.md"), "w") as f:
            f.write(text)
        index.append(f"{e}\t{size_of(e)}\t{name_of(e)}")
if index and len(want) > 1:
    tag = a.tu or (f"{a.range[0]}-{a.range[1]}" if a.range else "list")
    with open(os.path.join(a.out, f"index-{tag}.tsv"), "w") as f:
        f.write("addr\tsize\tname\n" + "\n".join(index) + "\n")
if not a.stdout:
    print(f"wrote {len(index)} packet(s) to {os.path.relpath(a.out, ROOT)}")
