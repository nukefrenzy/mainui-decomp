"""Strings and imports used by given functions. Usage: fstr.py <gdir> addr..."""
import csv,sys,collections
csv.field_size_limit(10**8)
g=sys.argv[1]; want=[a.zfill(8) for a in sys.argv[2:]]
rd=lambda n: list(csv.DictReader(open(f"{g}/{n}"),delimiter="\t",quoting=csv.QUOTE_NONE))
fs={f["entry"]:f for f in rd("functions.tsv")}
S=collections.defaultdict(list); I=collections.defaultdict(list)
for s in rd("strings.tsv"):
    if s["func_entry"] in want: S[s["func_entry"]].append(s["value"][:70])
for c in rd("calls.tsv"):
    if c["caller"] in want and fs.get(c["callee"],{}).get("is_thunk")=="true": I[c["caller"]].append(c["callee_name"].split("(")[0][-28:])
for a in want:
    print(a, fs[a]["size"] if a in fs else "?", "| S:", "; ".join(dict.fromkeys(S[a]))[:300], "| I:", ",".join(dict.fromkeys(I[a]))[:200])
