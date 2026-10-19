"""Print decompiled function(s) by address from decomp_raw.c. Usage: fn.py <decomp.c> addr [addr...]"""
import sys
want={a.lower().replace("0x","").zfill(8) for a in sys.argv[2:]}
out=False
for line in open(sys.argv[1],errors="replace"):
    if line.startswith("//@@FUNC "):
        out=line.split()[1] in want
    if out: sys.stdout.write(line)
