#!/usr/bin/env python3
"""Find MainUI functions whose source can be derived mechanically, and prove each one byte-exact.

Usage:
  autoderive.py --shapes                 group non-template functions by code shape -> build/shapes.tsv
  autoderive.py --headers                draft class headers from RTTI + field accesses -> build/layouts/
  autoderive.py --tu <unit> [--jobs N]   try the pattern library on one unit of docs/tu-map.tsv
                                         -> build/auto/<unit>/results.tsv and matched.cpp
Options: --exports DIR (default $MAINUI_EXPORTS or ~/mainui-exports), --max-size N (pattern cap, 0x80)

Patterns are checked with a synthetic declaration per candidate: at -O0 a method and a free function
taking the same pointer compile to the same code, so each candidate is an extern "C" function over a
struct that holds only the field it touches. Destructors use a minimal class hierarchy instead.
Calls and addresses are link-time words, so callee and global types never matter. Matching uses the
same masking as matchfn.py and also requires the original function to end where ours does.
Everything goes to build/ (gitignored); promotion into src/ is a separate step.
"""
import argparse
import collections
import csv
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
csv.field_size_limit(10**8)

ap = argparse.ArgumentParser()
ap.add_argument("--shapes", action="store_true")
ap.add_argument("--headers", action="store_true")
ap.add_argument("--tu")
ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
ap.add_argument("--max-size", type=lambda x: int(x, 0), default=0x80)
ap.add_argument("--exports", default=os.environ.get("MAINUI_EXPORTS", os.path.expanduser("~/mainui-exports")))
a = ap.parse_args()
if not (a.shapes or a.headers or a.tu):
    ap.error("nothing to do: pass --shapes, --headers or --tu")

TC = os.environ.get("MIYOO_TC", "")
PRE = os.path.join(TC, "arm-linux-gnueabihf-") if TC else "arm-linux-gnueabihf-"
MIYOO = [(0x15b20, 0x51b60), (0x11cdc8, 0x150000)]


def tsv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


def in_miyoo(v):
    return any(lo <= v < hi for lo, hi in MIYOO)


# ---------------------------------------------------------------- functions, names, marks
names = {}
with open(os.path.join(ROOT, "ghidra/names.tsv")) as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= 4 and re.fullmatch(r"[0-9a-f]{8}", p[0]):
            names[int(p[0], 16)] = p[1]
funcs = {int(f["entry"], 16) for f in tsv(os.path.join(a.exports, "functions.tsv"))
         if f["is_external"] != "true" and f["is_thunk"] != "true"}
starts = sorted(s for s in set(names) | funcs if in_miyoo(s))
ends = {}
for i, s in enumerate(starts):
    hi = next(h for lo, h in MIYOO if lo <= s < h)
    ends[s] = min(starts[i + 1], hi) if i + 1 < len(starts) else hi
matched = {int(m["addr"], 16) for m in tsv(os.path.join(ROOT, "src/matches.tsv"))}

# Template marks come from tusplit.py's probe (one run, report kept in build/)
os.makedirs(BUILD, exist_ok=True)
rep = subprocess.run([sys.executable, os.path.join(ROOT, "tools/tusplit.py"), "--exports", a.exports,
                      "--out", os.path.join(BUILD, "tu-map.autoderive.tsv"), "--report"],
                     capture_output=True, text=True, check=True).stdout
tmpl = {int(l[:6], 16) for l in rep.splitlines() if re.match(r"^[0-9a-f]{6} ", l) and " T " in l}
units = {t["name"]: (int(t["lo"], 16), int(t["hi"], 16)) for t in tsv(os.path.join(ROOT, "docs/tu-map.tsv"))}

elf = ELFFile(open(os.path.join(ROOT, "MainUI-354-clean"), "rb"))
text = elf.get_section_by_name(".text")
TDATA, TBASE = text.data(), text["sh_addr"]


def words(addr, n):
    return [struct.unpack_from("<I", TDATA, addr - TBASE + i)[0] for i in range(0, n, 4)]


# ---------------------------------------------------------------- disassembly (one objdump per range)
def load_disasm():
    ins = {}
    for lo, hi in MIYOO:
        out = subprocess.run([PRE + "objdump", "-d", "--no-show-raw-insn", f"--start-address={lo:#x}",
                              f"--stop-address={hi:#x}", os.path.join(ROOT, "MainUI-354-clean")],
                             capture_output=True, text=True).stdout
        for line in out.splitlines():
            p = line.split("\t")
            if len(p) >= 2 and p[0].strip().endswith(":"):
                op = p[1].strip()
                arg = re.sub(r"\s*[;@<].*$", "", p[2]).strip() if len(p) > 2 else ""
                ins[int(p[0].strip()[:-1], 16)] = (op, arg)
    return ins


def body(ins, s):
    return [ins[x] for x in range(s, ends[s], 4) if x in ins]


# ---------------------------------------------------------------- shapes
def shape(s):
    w = words(s, ends[s] - s)
    rets = [i for i, x in enumerate(w) if (x & 0x0fff8000) == 0x08bd8000 or x == 0xe12fff1e
            or (x & 0x0fffffff) == 0x049df004]
    last = rets[-1] if rets else len(w) - 1
    out = []
    for x in w[:last + 1]:
        if (x >> 25) & 7 == 5 or (x >> 24) in (0xfa, 0xfb):
            x &= 0xff000000                   # B/BL/BLX target
        elif (x & 0x0ff00000) in (0x03000000, 0x03400000):
            x &= 0xfff0f000                   # MOVW/MOVT immediate
        out.append(x)
    return (tuple(out), len(w) - last - 1)


def do_shapes():
    groups = collections.defaultdict(list)
    for s in starts:
        if s not in tmpl and ends[s] - s >= 4:
            groups[shape(s)].append(s)
    rows = sorted(groups.values(), key=lambda v: (-len(v), v[0]))
    path = os.path.join(BUILD, "shapes.tsv")
    with open(path, "w") as f:
        f.write("shape\tsize\tcount\tmatched\tmembers\n")
        for i, v in enumerate(rows):
            f.write(f"s{i:04d}\t{ends[v[0]] - v[0]}\t{len(v)}\t{sum(1 for x in v if x in matched)}\t"
                    + " ".join(f"{x:x}:{names.get(x, '?')}" for x in v) + "\n")
    multi = [v for v in rows if len(v) > 1]
    print(f"{sum(map(len, rows))} non-template functions in {len(rows)} shapes; {len(multi)} shapes are shared "
          f"by {sum(map(len, multi))} functions ({sum((ends[v[0]] - v[0]) * len(v) for v in multi):,} bytes)")
    print(f"wrote {os.path.relpath(path, ROOT)}")


# ---------------------------------------------------------------- field accesses (this-tracking)
MEM = re.compile(r"^(ldr|str)(b|sb|h|sh)?$")
TYPE = {("ldr", None): "int", ("str", None): "int", ("ldr", "b"): "unsigned char", ("ldr", "sb"): "char",
        ("str", "b"): "char", ("ldr", "h"): "unsigned short", ("ldr", "sh"): "short", ("str", "h"): "short"}
SIZE = {"int": 4, "unsigned char": 1, "char": 1, "unsigned short": 2, "short": 2}


def accesses(ins, s):
    """this-relative field accesses: {off: type} and member objects whose address is taken."""
    b = body(ins, s)
    slot = next((m.group(1) for op, arg in b[:8] if op == "str"
                 for m in [re.fullmatch(r"r0, \[fp, #-(\d+)\]", arg)] if m), None)
    if slot is None:
        return {}, set()
    fields, objs, this = {}, set(), set()
    for op, arg in b:
        regs = re.findall(r"\b(r\d+|ip|lr)\b", arg)
        m = MEM.match(op)
        if op == "ldr" and arg.endswith(f"[fp, #-{slot}]"):
            this.add(regs[0])
            continue
        if m:
            mm = re.search(r"\[(r\d+)(?:, #(-?\d+))?\]$", arg)
            if mm and mm.group(1) in this:
                t = TYPE.get((m.group(1), m.group(2)), "int")
                off = int(mm.group(2) or 0)
                if off > 0 and (off not in fields or fields[off] == "int"):
                    fields[off] = t            # +0 is the vtable pointer; a narrow access beats "int"
            if m.group(1) == "ldr" and regs:
                this.discard(regs[0])
            continue
        mm = re.fullmatch(r"(r\d+), (r\d+), #(\d+)", arg)
        if op == "add" and mm and mm.group(2) in this:
            objs.add(int(mm.group(3)))
        if regs and op not in ("str", "cmp", "push", "pop", "bl", "blx", "b") and not op.startswith("b"):
            this.discard(regs[0])
    return fields, objs


# ---------------------------------------------------------------- headers
def do_headers(ins):
    classes = json.load(open(os.path.join(ROOT, "ghidra/classes.json")))
    owner = collections.defaultdict(set)      # function -> classes whose vtable or name claims it
    for c, info in classes.items():
        for vt in info["vtables"]:
            for e in vt["entries"]:
                owner[int(e, 16)].add(c)
    for s, n in names.items():
        if "::" in n and n.split("::")[0] in classes:
            owner[s].add(n.split("::")[0])
    acc = collections.defaultdict(dict)
    objs = collections.defaultdict(set)
    for s, cs in owner.items():
        if not in_miyoo(s) or s not in ends or len(cs) != 1:
            continue                           # shared base slots say nothing about one class
        f, o = accesses(ins, s)
        c = next(iter(cs))
        for off, t in f.items():
            acc[c].setdefault(off, t)
        objs[c] |= o

    def chain(c):
        b = classes[c]["bases"]
        return chain(b[0]) + [c] if b and b[0] in classes else [c]

    # an offset belongs to the most-derived ancestor that already uses it, else to the class itself
    own = collections.defaultdict(dict)
    for c in sorted(classes, key=lambda c: len(chain(c))):
        for off, t in sorted(acc[c].items()):
            if not any(off in own[x] for x in chain(c)[:-1]):
                own[c][off] = t
        for off in objs[c]:
            if not any(off in own[x] for x in chain(c)) and off not in own[c]:
                own[c][off] = "obj"
    out = os.path.join(BUILD, "layouts")
    os.makedirs(out, exist_ok=True)
    for c, info in classes.items():
        base = info["bases"][0] if info["bases"] else None
        vt = info["vtables"][0]["entries"] if info["vtables"] else []
        bvt = classes[base]["vtables"][0]["entries"] if base and classes[base]["vtables"] else []
        lines = [f"// {c}: draft generated by tools/autoderive.py --headers. Review before use.", "#pragma once"]
        if base:
            lines.append(f'#include "{base}.h"')
        lines.append(f"struct {c} : {base} {{" if base else f"struct {c} {{")
        for i, e in enumerate(vt):
            if i < len(bvt) and bvt[i] == e:
                continue                       # inherited unchanged
            n = names.get(int(e, 16), "")
            meth = n.split("::")[-1] if "::" in n else ""
            if i <= 1:
                if i == 0:
                    lines.append(f"    virtual ~{c}();                      // slots 0-1: {vt[0]}, {vt[1]}")
                continue
            if not meth or re.match(r"(vfunc\d+|ctor_or_dtor_\w+|dtor|deletingDtor)$", meth):
                meth = f"vfunc{i}"
            lines.append(f"    virtual int {meth}();" + " " * max(1, 28 - len(meth)) + f"// slot {i} (+{4 * i:#x}) {e}")
        start = 4 if not base and vt else 0
        if base:
            start = max([o + SIZE.get(t, 4) for x in chain(base) for o, t in own[x].items()] + [4])
        pos = start
        for off, t in sorted(own[c].items()):
            if off < pos:
                lines.append(f"    // +{off:#x} {t}: overlaps the previous field; check")
                continue
            if off > pos:
                lines.append(f"    char pad_{pos:x}[{off - pos:#x}];")
            if t == "obj":
                lines.append(f"    char obj_{off:x}[4];                 // +{off:#x} member object, address taken; size unknown")
                pos = off + 4
            else:
                lines.append(f"    {t} field_{off:x};" + " " * max(1, 24 - len(t) - len(f'{off:x}')) + f"// +{off:#x}")
                pos = off + SIZE[t]
        lines.append("};")
        with open(os.path.join(out, f"{c}.h"), "w") as f:
            f.write("\n".join(lines) + "\n")
    print(f"wrote {len(classes)} draft headers to {os.path.relpath(out, ROOT)}")


# ---------------------------------------------------------------- compiling and comparing candidates
def compile_obj(src):
    d = tempfile.mkdtemp(prefix="ad-", dir=BUILD)
    cpp, obj = os.path.join(d, "c.cpp"), os.path.join(d, "c.o")
    open(cpp, "w").write(src)
    r = subprocess.run([PRE + "g++", "-O0", "-fsigned-char", "-w", "-c", cpp, "-o", obj],
                       capture_output=True, text=True)
    if r.returncode:
        return None, d
    return obj, d


def symbols(obj):
    o = ELFFile(open(obj, "rb"))
    secs = list(o.iter_sections())
    out = {}
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
        out[s.name] = (code, bytes(mask))
    return out


def same(addr, code, mask):
    if ends[addr] - addr != len(code):        # the original must end where ours does
        return False
    b = TDATA[addr - TBASE:addr - TBASE + len(code)]
    return all(mask[i] or b[i] == code[i] for i in range(len(code)))


# ---------------------------------------------------------------- pattern library
ARGT = {"str": ["int"], "strb": ["char", "unsigned char", "bool"], "strh": ["short", "unsigned short"]}
RETT = ["int", "char", "unsigned char", "bool", "short", "unsigned short"]


def features(ins, s):
    b = body(ins, s)
    args, i = [], 0
    while i < len(b) and b[i][0] in ("push", "add", "sub") and ("fp" in b[i][1] or "sp" in b[i][1]):
        i += 1
    pending = {}
    while i < len(b):
        op, arg = b[i]
        m = re.fullmatch(r"(r\d), (r[0-3])", arg)
        if op == "mov" and m:
            pending[m.group(1)] = m.group(2)
            i += 1
            continue
        m = re.fullmatch(r"(r\d), \[fp, #-\d+\]", arg)
        if op in ARGT and m:
            src = pending.get(m.group(1), m.group(1))
            if src in ("r0", "r1", "r2", "r3") and int(src[1]) == len(args):
                args.append(op)
                i += 1
                continue
        break
    rest = b[i:]
    consts, lo16 = set(), {}
    for op, arg in rest:
        m = re.fullmatch(r"(r\d), #(-?\d+)", arg)
        if m and op in ("mov", "movw"):
            consts.add(int(m.group(2)))
            lo16[m.group(1)] = int(m.group(2))
        if m and op == "movt" and m.group(1) in lo16:
            v = (int(m.group(2)) << 16) | lo16[m.group(1)]
            consts.add(v - (1 << 32) if v >= 1 << 31 else v)
        if m and op == "mvn":
            consts.add(~int(m.group(2)))
    fields = {}
    for op, arg in rest:
        mm = MEM.match(op)
        m = re.search(r"\[(r\d+), #(\d+)\]$", arg)
        if mm and m and "fp" not in arg:
            fields.setdefault(int(m.group(2)), set()).add((mm.group(1), mm.group(2)))
    adds = {int(m.group(1)) for op, arg in rest if op == "add"
            for m in [re.fullmatch(r"r\d, r\d, #(\d+)", arg)] if m}
    calls = sum(1 for op, _ in rest if op in ("bl", "blx"))
    returns = any(op == "mov" and arg.startswith("r0, ") for op, arg in rest)
    globs = any(op == "movw" for op, _ in rest)   # an address (or a constant) built with movw/movt
    return args, consts, fields, adds, calls, returns, globs


def arg_types(args, first_ptr=None):
    """Every combination of C types for the stored argument widths (byte/half args are ambiguous)."""
    combos = [[]]
    for i, w in enumerate(args):
        opts = [first_ptr] if (i == 0 and first_ptr) else ARGT[w]
        combos = [c + [t] for c in combos for t in opts]
    return combos[:24]


def candidates(ins, s):
    """(pattern, declarations, function body) triples for one function; name placeholder is FN."""
    args, consts, fields, adds, calls, returns, globs = features(ins, s)
    out = []
    G = 'extern "C" int G;'

    def sig(ret, types):
        ps = ", ".join(f"{t} a{i}" for i, t in enumerate(types)) or "void"
        return f'extern "C" {ret} FN({ps})'

    for types in arg_types(args):
        if calls == 0:
            out.append(("empty", "", sig("void", types) + " {}"))
            for k in sorted(consts) or [0]:
                for r in RETT:
                    out.append(("return-const", "", sig(r, types) + f" {{ return {k}; }}"))
            for j, t in enumerate(types):
                out.append(("return-arg", "", sig(t, types) + f" {{ return a{j}; }}"))
            if globs:
                for r in ("int", "char", "unsigned char", "bool", "short"):
                    out.append(("return-global", G.replace("int", r), sig(r, types) + " { return G; }"))
                out.append(("return-global-addr", G, sig("int *", types) + " { return &G; }"))
        if calls == 1:
            for k in sorted(consts):
                for r in ("void", "int"):
                    ret = "" if r == "void" else "return "
                    out.append(("call-const", f'extern "C" {r} g(...);', sig(r, types) + f" {{ {ret}g({k}); }}"))
            if globs:
                for r in ("void", "int"):
                    ret = "" if r == "void" else "return "
                    for x in ("G", "&G"):
                        out.append(("call-global", f'{G}\nextern "C" {r} g(...);',
                                    sig(r, types) + f" {{ {ret}g({x}); }}"))
            ps = ", ".join(f"a{i}" for i in range(len(types)))
            out.append(("forward", "extern \"C\" void g(...);", sig("void", types) + f" {{ g({ps}); }}"))
            for r in ("int", "char", "unsigned char", "bool"):
                out.append(("forward", f'extern "C" {r} g(...);', sig(r, types) + f" {{ return g({ps}); }}"))
    if args:
        n = len(args)
        for off, kinds in sorted(fields.items()):
            for k in kinds:
                for ft in ([TYPE.get(k, "int")] + (["bool"] if k[1] == "b" else [])
                           + (["void *", "float"] if k[1] is None else [])):
                    pad = f"char pad[{off}]; " if off else ""
                    decl = f"struct S {{ {pad}{ft} f; }};"
                    rest = [t for t in arg_types(args[1:])] if n > 1 else [[]]
                    for more in rest[:6]:
                        types = ["S *"] + more
                        if k[0] == "ldr" and calls == 0:
                            for r in [ft] + (["int"] if ft != "int" else []):
                                out.append(("getter", decl, sig(r, types) + " { return a0->f; }"))
                        if k[0] == "str" and calls == 0:
                            for j in range(1, n):
                                out.append(("setter", decl, sig("void", types) + f" {{ a0->f = a{j}; }}"))
                            for c in sorted(consts) or [0]:
                                out.append(("setter-const", decl, sig("void", types) + f" {{ a0->f = {c}; }}"))
                        if k[0] == "ldr" and calls == 1:
                            ps = ", ".join(["a0->f"] + [f"a{i}" for i in range(1, n)])
                            out.append(("forward-field", 'extern "C" void g(...);',
                                        decl + "\n" + sig("void", types) + f" {{ g({ps}); }}"))
                            out.append(("forward-field", 'extern "C" int g(...);',
                                        decl + "\n" + sig("int", types) + f" {{ return g({ps}); }}"))
        if calls == 0:
            for off in sorted(adds):
                decl = f"struct S {{ char pad[{off}]; int f; }};"
                out.append(("return-member-addr", decl, sig("int *", ["S *"]) + " { return &a0->f; }"))
        if calls == 1:
            for off in sorted(adds):
                decl = f"struct S {{ char pad[{off}]; int f; }};"
                for more in (arg_types(args[1:]) if n > 1 else [[]])[:6]:
                    types = ["S *"] + more
                    ps = ", ".join(["&a0->f"] + [f"a{i}" for i in range(1, n)])
                    out.append(("forward-member", 'extern "C" void g(...);',
                                decl + "\n" + sig("void", types) + f" {{ g({ps}); }}"))
                    out.append(("forward-member", 'extern "C" int g(...);',
                                decl + "\n" + sig("int", types) + f" {{ return g({ps}); }}"))
    return out


def dtor_candidates(ins, d2, base, has_vt, d0):
    """Empty destructors: members with destructors are found as 'add rX, rY, #off' before a call.
    The deleting destructor passes sizeof(class) to sized operator delete, which fixes the class size."""
    size = None
    if d0:
        size = next((int(m.group(1)) for op, arg in body(ins, d0) if op == "mov"
                     for m in [re.fullmatch(r"r1, #(\d+)", arg)] if m), None)
    b = body(ins, d2)
    offs = sorted({int(m.group(1)) for op, arg in b if op == "add"
                   for m in [re.fullmatch(r"r\d, r\d, #(\d+)", arg)] if m})
    out = []
    for use in ([offs, []] if offs else [[]]):
        decl = ["struct M { ~M(); };"]
        if base:
            decl.append("struct B { virtual ~B(); int b_; };")
        cls = ["struct C" + (" : B" if base else "") + " {"]
        pos = 8 if base else (4 if has_vt else 0)
        for off in use:
            if off > pos:
                cls.append(f"  char pad{off}[{off - pos}];")
            cls.append(f"  M m{off};")
            pos = off + 1
        if size and size > pos:
            cls.append(f"  char tail[{size - pos}];")
        cls.append("  virtual ~C();" if (base or has_vt) else "  ~C();")
        cls.append("};")
        out.append(("dtor", "\n".join(decl + cls), "C::~C() {}"))
    return out


def try_function(job):
    s, cands, want = job
    if not cands:
        return s, None, 0
    src, keys = [], []
    for k, (pat, decl, fn) in enumerate(cands):
        if pat == "dtor":
            src.append(f"namespace n{k} {{\n{decl}\n{fn}\n}}")
        else:
            src.append(f"namespace n{k} {{\n{decl}\n{fn.replace('FN', f'f_{k}')}\n}}")
        keys.append(k)
    obj, d = compile_obj("\n".join(src))
    try:
        if obj is None:                         # one bad candidate breaks the batch: retry one by one
            syms = {}
            for k in keys:
                pat, decl, fn = cands[k]
                body_ = fn if pat == "dtor" else fn.replace("FN", f"f_{k}")
                o, d2 = compile_obj(f"namespace n{k} {{\n{decl}\n{body_}\n}}")
                if o:
                    syms.update(symbols(o))
                subprocess.run(["rm", "-rf", d2])
        else:
            syms = symbols(obj)
        for k in keys:
            pat, decl, fn = cands[k]
            if pat == "dtor":
                d2 = syms.get(f"_ZN2n{k}1CD2Ev")
                if d2 and same(s, *d2):
                    ok = all(x in syms and same(addr, *syms[x])
                             for x, addr in [(f"_ZN2n{k}1CD0Ev", want)] if addr)
                    if ok:
                        return s, cands[k], len(cands)
                continue
            sym = syms.get(f"f_{k}")
            if sym and same(s, *sym):
                return s, cands[k], len(cands)
        return s, None, len(cands)
    finally:
        subprocess.run(["rm", "-rf", d])


def do_tu(ins, unit):
    if unit not in units:
        sys.exit(f"unit {unit} not in docs/tu-map.tsv")
    lo, hi = units[unit]
    fs = [s for s in starts if lo <= s < hi]
    todo = [s for s in fs if s not in tmpl and s not in matched]
    classes = json.load(open(os.path.join(ROOT, "ghidra/classes.json")))
    jobs, kind = [], {}
    dtor_of = {}
    for c, info in classes.items():
        d2 = next((s for s, n in names.items() if n == f"{c}::dtor"), None)
        d0 = next((s for s, n in names.items() if n == f"{c}::deletingDtor"), None)
        if d2 in todo:
            dtor_of[d2] = (c, info["bases"][0] if info["bases"] else None, bool(info["vtables"]), d0)
    for s in todo:
        if s in dtor_of:
            c, base, vt, d0 = dtor_of[s]
            jobs.append((s, dtor_candidates(ins, s, base, vt, d0), d0))
            kind[s] = "dtor"
        elif ends[s] - s <= a.max_size:
            jobs.append((s, candidates(ins, s), None))
            kind[s] = "small"
        else:
            kind[s] = "large"
    with ThreadPoolExecutor(a.jobs) as ex:
        res = {s: (c, n) for s, c, n in ex.map(try_function, jobs)}
    d0_hit = {dtor_of[s][3] for s, (c, _) in res.items() if c and s in dtor_of and dtor_of[s][3]}
    out = os.path.join(BUILD, "auto", unit)
    os.makedirs(out, exist_ok=True)
    stats = collections.Counter()
    sbytes = collections.Counter()
    with open(os.path.join(out, "results.tsv"), "w") as f, open(os.path.join(out, "matched.cpp"), "w") as m:
        f.write("addr\tsize\tname\tstatus\tpattern\ttried\n")
        m.write(f"// {unit}: functions matched by tools/autoderive.py, with the synthetic declarations that\n"
                "// proved them. Drafts for promotion into src/; names and types still need review.\n")
        for s in fs:
            size = ends[s] - s
            nm = names.get(s, "?")
            if s in tmpl:
                st, pat, n = "TEMPLATE", "std probe", 0
            elif s in matched:
                st, pat, n = "MATCHED", "src/", 0
            elif s in d0_hit:
                st, pat, n = "MATCH", "deleting dtor (from D2 source)", 0
            elif s in res and res[s][0]:
                st, pat, n = "MATCH", res[s][0][0], res[s][1]
                pat_, decl, fn = res[s][0]
                fn = fn.replace("FN", f"fn_{s:x}")
                decl, fn = (re.sub(r"\b([gG])\b", rf"\1_{s:x}", x) for x in (decl, fn))   # unique callees
                m.write(f"\n// {s:#x} {nm} ({size} B, {pat_})\nnamespace a_{s:x} {{\n{decl}\n{fn}\n}}\n")
                if pat_ == "dtor" and dtor_of[s][3]:
                    m.write(f"// {dtor_of[s][3]:#x} {names.get(dtor_of[s][3], '?')}: deleting dtor, same source\n")
            elif s in res:
                st, pat, n = "NOMATCH", kind[s], res[s][1]
            else:
                st, pat, n = "SKIP", kind[s], 0
            stats[st] += 1
            sbytes[st] += size
            f.write(f"{s:06x}\t{size}\t{nm}\t{st}\t{pat}\t{n}\n")
    pats = collections.Counter(l.split("\t")[4] for l in open(os.path.join(out, "results.tsv"))
                               if "\tMATCH\t" in l)
    tot = sum(sbytes.values())
    print(f"{unit}: {len(fs)} functions, {tot:,} bytes")
    for st in ("MATCHED", "TEMPLATE", "MATCH", "NOMATCH", "SKIP"):
        print(f"  {st:9s} {stats[st]:5d} fns {sbytes[st]:8,} B ({100 * sbytes[st] / tot:5.1f}%)")
    print("  new matches by pattern: " + ", ".join(f"{p} {n}" for p, n in pats.most_common()))
    print(f"wrote {os.path.relpath(out, ROOT)}/results.tsv and matched.cpp")


if a.shapes:
    do_shapes()
if a.headers or a.tu:
    INS = load_disasm()
    if a.headers:
        do_headers(INS)
    if a.tu:
        do_tu(INS, a.tu)
