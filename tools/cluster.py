"""Assign subsystems and provisional names to MainUI functions.

Inputs (from ExportStructure/ExportDecomp + rtti.py), outputs names.json + summary on stdout.
Usage: python3 -I cluster.py <ghidra_out_dir> <classes.json> <out_dir>
"""
import collections
import csv
import json
import re
import sys

gdir, classes_path, out_dir = sys.argv[1:4]
csv.field_size_limit(10**8)


def tsv(name):
    with open(f"{gdir}/{name}", newline="") as fh:
        return list(csv.DictReader(fh, delimiter="\t", quoting=csv.QUOTE_NONE))


funcs = {r["entry"]: r for r in tsv("functions.tsv")}
calls = tsv("calls.tsv")
strings = tsv("strings.tsv")
classes = json.load(open(classes_path))


def norm(a):
    return "%08x" % int(a, 16)


internal = {a: f for a, f in funcs.items() if f["is_thunk"] == "false" and f["is_external"] == "false"}

# Third-party code compiled into the binary (boundaries found from call graph + strings)
SQLITE = (0x57920, 0x11CDC8)   # SQLite 3.30.1 amalgamation (verified byte-exact)
RUNTIME_FROM = 0x150000        # libgcc / crt helpers at the end of .text
lib_of = {}
for a in internal:
    v = int(a, 16)
    if 0x51B60 <= v < 0x57920:
        lib_of[a] = "lib_cjson"
    elif SQLITE[0] <= v < SQLITE[1]:
        lib_of[a] = "lib_sqlite"
    elif v >= RUNTIME_FROM:
        lib_of[a] = "lib_runtime"

# Subsystem rules: (subsystem, regex over import names, regex over strings)
RULES = [
    ("database",  r"^sqlite3_",                       r"(?i)\b(select|insert|update|delete|create table)\b|\.db\b"),
    ("audio",     r"^(Mix_|MI_AO_)|^_?snd_|mad_",     r"(?i)\.(wav|mp3|ogg)\b|volume|sound"),
    ("display",   r"^MI_DISP_|^MI_PANEL_|^MI_GFX_|^MI_DIVP_", r"(?i)lumination|brightness|hue|saturation|contrast|/dev/fb|pwm"),
    ("render",    r"^(SDL_(Blit|Fill|Create.*Surface|Flip|Set|Map|Upper|Free|Display|Lock|Unlock|Convert|GetRGB|Rotate)|TTF_|IMG_|zoom|rotozoom)", r"(?i)\.ttf\b|\.png\b|skin/|font"),
    ("input",     r"^(SDL_(PollEvent|WaitEvent|PumpEvents|GetKey|EnableKey))", r"/dev/input|(?i:keymap)|\bKEY_"),
    ("wifi",      r"^$",                              r"(?i)wpa_|wlan|wifi|hostapd|ssid|psk|udhcpc|hotspot"),
    ("power",     r"^(reboot|sync)$",                 r"(?i)poweroff|reboot|battery|batt|charg|axp|/sys/class/power|offOrder|suspend|sleep"),
    ("config",    r"^(json|cJSON)",                   r"(?i)system\.json|config\.json|\.json\b|appconfigs"),
    ("theme",     r"^$",                              r"(?i)themes?/|skin\b|theme"),
    ("language",  r"^$",                              r"(?i)\blang\b|lang/|language"),
    ("launch",    r"^(execv|execl|execvp|system|popen|fork|vfork)$", r"(?i)cmd_to_run|launch\.sh|\.sh\b|retroarch|emu/"),
    ("files",     r"^(opendir|readdir|closedir|stat|scandir|mkdir|rename|remove|unlink|access)$", r"(?i)/mnt/SDCARD|roms/|imgs/|\.zip\b"),
    ("device",    r"^(MI_SYS_|ioctl|InitKeyShm|GetKeyShm|SetKeyShm|UninitKeyShm|encrypt|encrypter)", r"(?i)uuid|serial|/proc/|/sys/"),
    ("threads",   r"^pthread_",                       r"^$"),
]
RULES = [(n, re.compile(i), re.compile(s)) for n, i, s in RULES]

imports_of = collections.defaultdict(list)
callees_of = collections.defaultdict(set)
callers_of = collections.defaultdict(set)
for c in calls:
    callee = funcs.get(c["callee"])
    if callee is None:
        continue
    if callee["is_thunk"] == "true" or callee["is_external"] == "true":
        imports_of[c["caller"]].append(c["callee_name"])
    else:
        callees_of[c["caller"]].add(c["callee"])
        callers_of[c["callee"]].add(c["caller"])

strings_of = collections.defaultdict(list)
for s in strings:
    if s["func_entry"]:
        strings_of[s["func_entry"]].append(s["value"])

# Class methods from vtables (classes.json uses bare hex; Ghidra uses 8-digit hex)
method_of = {}  # addr -> (class, slot)
for cname, c in classes.items():
    for vt in c["vtables"]:
        for i, e in enumerate(vt["entries"]):
            if e and e != "pure":
                a = norm(e)
                # keep the most-base class that owns this exact implementation
                if a not in method_of:
                    method_of[a] = (cname, i)


def depth(cname):
    d = 0
    while classes.get(cname, {}).get("bases"):
        cname = classes[cname]["bases"][0]
        d += 1
    return d


for cname in sorted(classes, key=depth):
    for vt in classes[cname]["vtables"]:
        for i, e in enumerate(vt["entries"]):
            if e and e != "pure":
                a = norm(e)
                if depth(method_of[a][0]) > depth(cname):
                    method_of[a] = (cname, i)

# Constructors/destructors: functions whose decompiled body stores a class vtable pointer
vt_addr = {}
for cname, c in classes.items():
    for vt in c["vtables"]:
        vt_addr["%x" % (int(vt["vtable"], 16) + 8)] = cname
ctor_of = collections.defaultdict(set)
cur = None
with open(f"{gdir}/decomp_raw.c", errors="replace") as fh:
    for line in fh:
        if line.startswith("//@@FUNC "):
            cur = line.split()[1]
            continue
        for m in re.finditer(r"(?:PTR_\w*_|&DAT_|0x)00?([0-9a-f]{6})\b", line):
            if m.group(1).lstrip("0") in vt_addr:
                ctor_of[cur].add(vt_addr[m.group(1).lstrip("0")])

# stb_image zlib: functions citing stb_zlib.cpp plus their nearby callees
stb = {a for a, ss in strings_of.items() if any("stb_zlib" in s for s in ss)}
todo = list(stb)
while todo:
    x = todo.pop()
    for y in callees_of[x]:
        if y not in stb and abs(int(y, 16) - int(x, 16)) < 0x4000 and y not in lib_of:
            stb.add(y)
            todo.append(y)
for a in stb:
    lib_of[a] = "lib_stb"

subsys = dict(lib_of)
evidence = {a: "library" for a in lib_of}
for a in internal:
    if a in lib_of:
        continue
    score = collections.Counter()
    for imp in imports_of[a]:
        for n, ri, _ in RULES:
            if ri.pattern != "^$" and ri.search(imp):
                score[n] += 2
    for s in strings_of[a]:
        for n, _, rs in RULES:
            if rs.pattern != "^$" and rs.search(s):
                score[n] += 3
    if score:
        subsys[a] = score.most_common(1)[0][0]
        evidence[a] = "direct"

# Class methods inherit a subsystem from their class family
FAMILY = [
    (r"Wifi|Hotspot", "wifi"), (r"Theme|Color", "theme"), (r"Language", "language"),
    (r"Favorite|Recent|Rom|FileBrowser|FileAction|Search|DBCache|DBCached", "games"),
    (r"KeyTest|LCDTest|SoundTest|SDCardTest|HardwareTest|ADCInfo", "hardware_test"),
    (r"KeySetting|Setting|DeviceInfo", "settings"),
    (r"Window|Dialog|Popup|PopUp|Notify|CountDown|Progress|Ime|IME|Menu|Drawer|Item|Label|Drawable", "ui_framework"),
    (r"Action", "actions"), (r"Worker", "threads"),
]
for a, (cname, slot) in method_of.items():
    for pat, fam in FAMILY:
        if re.search(pat, cname):
            subsys.setdefault(a, fam)
            if fam != "ui_framework":
                subsys[a] = fam if evidence.get(a) != "direct" else subsys[a]
            evidence.setdefault(a, "class")
            break
for a, cs in ctor_of.items():
    if a in internal and a not in method_of:
        cname = sorted(cs, key=depth)[-1]
        for pat, fam in FAMILY:
            if re.search(pat, cname):
                subsys.setdefault(a, fam)
                evidence.setdefault(a, "class")
                break

# Propagate to unlabelled functions by neighbour majority (2 rounds)
for _ in range(2):
    new = {}
    for a in internal:
        if a in subsys:
            continue
        votes = collections.Counter(subsys[n] for n in callees_of[a] | callers_of[a]
                                    if n in subsys and not subsys[n].startswith("lib_"))
        if votes:
            new[a] = votes.most_common(1)[0][0]
    for a, s in new.items():
        subsys[a] = s
        evidence[a] = "neighbour"

# Provisional names
names = {}
for a, f in internal.items():
    orig = f["name"]
    if not orig.startswith("FUN_"):
        continue  # keep Ghidra/symbol names (entry, _init, etc.)
    tag = subsys.get(a, "misc")
    if a in lib_of:
        names[a] = {"name": f"{lib_of[a][4:]}_{a[-5:]}", "subsystem": tag, "confidence": "library"}
        continue
    if a in method_of:
        cname, slot = method_of[a]
        nm = f"{cname}::vfunc{slot}"
        conf = "confirmed-class"  # class + slot are certain; slot meaning is not
    elif a in ctor_of:
        cname = sorted(ctor_of[a], key=depth)[-1]
        nm = f"{cname}::ctor_or_dtor_{a[-5:]}"
        conf = "likely"
    else:
        hint = ""
        for s in strings_of[a]:
            w = re.findall(r"[A-Za-z][A-Za-z0-9]{3,}", s)
            if w:
                hint = "_" + w[0][:20]
                break
        if not hint and imports_of[a]:
            hint = "_" + re.sub(r"\W", "", imports_of[a][0].split("::")[-1].split("(")[0])[:20]
        nm = f"{tag}{hint}_{a[-5:]}"
        conf = "guess" if evidence.get(a) == "neighbour" or a not in evidence else "inferred"
    names[a] = {"name": nm, "subsystem": tag, "confidence": conf}

json.dump(names, open(f"{out_dir}/names.json", "w"), indent=0)

# Summary
total = len(internal)
cnt = collections.Counter(subsys.get(a, "misc") for a in internal)
size = collections.Counter()
for a in internal:
    size[subsys.get(a, "misc")] += int(funcs[a]["size"])
print(f"{total} internal functions, {len(method_of)} virtual methods, {len(ctor_of)} ctor/dtor-like")
print("evidence:", collections.Counter(evidence.get(a, "none") for a in internal))
print("confidence:", collections.Counter(v["confidence"] for v in names.values()))
for k, v in cnt.most_common():
    print(f"  {k:14s} {v:5d} funcs {size[k]:7d} bytes")
