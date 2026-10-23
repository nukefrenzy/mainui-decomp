#!/usr/bin/env python3
"""Compare MainUI-354-clean (Onion) with a stock Miyoo MainUI, function by function.

    python3 tools/stockdiff.py <stock MainUI> [--tsv out.tsv]

The two binaries are separate builds of nearly the same source, so their functions sit in the
same order at slightly different addresses. Function starts come from ghidra/names.tsv and
ghidra/libnames.tsv. Each Onion function is normalized (call and branch targets, movw/movt
immediates and address-like literal words are masked, because they change whenever anything
moves) and searched for in the stock .text near its expected position. The report lists:

- functions found nowhere in stock (changed by Onion), with the calls Onion replaced by NOPs;
- .rodata strings that occur a different number of times in the two builds.

Changes to strings and data are invisible to the function comparison (addresses are masked),
which is why the string list is printed too. docs/onion-changes.md explains the results.
"""
import argparse, collections, re, struct, sys
from elftools.elf.elffile import ELFFile

ONION = 'MainUI-354-clean'
NOPS = (0xe320f000, 0xe1a00000)   # nop, mov r0, r0


class Binary:
    def __init__(self, path):
        self.elf = ELFFile(open(path, 'rb'))
        text = self.elf.get_section_by_name('.text')
        self.text_addr, self.text = text['sh_addr'], text.data()
        self.rodata = self.elf.get_section_by_name('.rodata').data()
        self.plt = {}   # PLT entry address -> imported function name
        rel = self.elf.get_section_by_name('.rel.plt')
        dynsym = self.elf.get_section_by_name('.dynsym')
        base = self.elf.get_section_by_name('.plt')['sh_addr'] + 20
        for i, r in enumerate(rel.iter_relocations()):
            self.plt[base + 12 * i] = dynsym.get_symbol(r['r_info_sym']).name

    def word(self, addr):
        return struct.unpack_from('<I', self.text, addr - self.text_addr)[0]

    def strings(self):
        found = re.finditer(rb'[\x09\x0a\x20-\x7e]{3,}', self.rodata)
        return collections.Counter(m.group().decode('latin1') for m in found)


def normalize(word):
    if ((word >> 20) & 0xfb) == 0x30:       # movw / movt: mask the immediate
        return word & 0xfff0f000
    if ((word >> 25) & 7) == 5:             # b / bl / blx: mask the offset
        return word & 0xff000000
    if 0x10000 <= word < 0x190000:          # literal pool word holding an address
        return 0xdead0000
    return word


def function_starts(onion):
    starts = {}
    for path in ('ghidra/names.tsv', 'ghidra/libnames.tsv'):
        for line in open(path):
            cols = line.rstrip('\n').split('\t')
            try:
                addr = int(cols[0], 16)
            except ValueError:
                continue
            if onion.text_addr <= addr < onion.text_addr + len(onion.text) and addr % 4 == 0:
                starts.setdefault(addr, cols[1])
    return starts


def compare(onion, stock):
    """Yields (onion addr, size, name, stock addr or None) for every Onion function."""
    stock_words = [normalize(w) for (w,) in struct.iter_unpack('<I', stock.text)]
    key_len = 6
    index = collections.defaultdict(list)
    for i in range(len(stock_words) - key_len):
        index[tuple(stock_words[i:i + key_len])].append(i)
    starts = function_starts(onion)
    addrs = sorted(starts)
    end_of_text = onion.text_addr + len(onion.text)
    delta = stock.text_addr - onion.text_addr      # running offset, stock minus Onion
    for n, addr in enumerate(addrs):
        end = addrs[n + 1] if n + 1 < len(addrs) else end_of_text
        words = [normalize(onion.word(a)) for a in range(addr, end - 3, 4)]
        if not words:
            continue
        expected = (addr + delta - stock.text_addr) // 4
        if len(words) >= key_len:
            candidates = sorted(index.get(tuple(words[:key_len]), []), key=lambda i: abs(i - expected))[:50]
        else:
            candidates = range(max(0, expected - 64), expected + 64)
        found = next((i for i in candidates if stock_words[i:i + len(words)] == words), None)
        if found is None:
            yield addr, end - addr, starts[addr], None
        else:
            delta = stock.text_addr + 4 * found - addr
            yield addr, end - addr, starts[addr], stock.text_addr + 4 * found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('stock')
    ap.add_argument('--tsv', help='write every function (onion addr, size, same/changed, stock addr, name)')
    args = ap.parse_args()
    onion, stock = Binary(ONION), Binary(args.stock)
    rows = list(compare(onion, stock))
    stock_names = {s: name for _, _, name, s in rows if s is not None}
    if args.tsv:
        with open(args.tsv, 'w') as f:
            for addr, size, name, s in rows:
                f.write(f'{addr:06x}\t{size}\t{"same" if s is not None else "changed"}\t{s or 0:06x}\t{name}\n')
    changed = [r for r in rows if r[3] is None]
    print(f'{len(rows) - len(changed)} functions identical (addresses masked), {len(changed)} differ:')
    # Line each changed function up with the stock code that follows the previous one.
    stock_at = {}
    pos = stock.text_addr + (rows[0][0] - onion.text_addr)
    for addr, size, name, s in rows:
        if s is not None:
            pos = s
        else:
            stock_at[addr] = pos
        pos += size
    for addr, size, name, _ in changed:
        s = stock_at[addr]
        nops, got, other = [], 0, 0
        for i in range(0, size, 4):
            w_onion, w_stock = onion.word(addr + i), stock.word(s + i)
            if normalize(w_onion) == normalize(w_stock):
                continue
            if w_onion in NOPS and (w_stock >> 24) == 0xeb:
                off = w_stock & 0xffffff
                off -= 0x1000000 if off & 0x800000 else 0
                nops.append((addr + i, s + i + 8 + 4 * off))
            elif w_onion < 0x1000 and w_stock < 0x1000:
                got += 1                    # small offsets, e.g. into the GOT
            else:
                other += 1
        if other:
            kind = 'rewritten'
        elif nops:
            kind = 'patched: calls replaced by NOPs'
        else:
            kind = 'same code; only GOT offsets differ'
        print(f'  {addr:#08x} {name} ({size} bytes): {kind}')
        if not other:
            for at, target in nops:
                callee = stock_names.get(target) or stock.plt.get(target) or f'{target:#x}'
                print(f'      {at:#08x}: NOP where stock calls {callee}')
    print('Strings that differ (count in Onion / stock):')
    a, b = onion.strings(), stock.strings()
    for text in sorted(set(a) | set(b)):
        if a[text] != b[text]:
            print(f'  {a[text]} / {b[text]}  {text!r}')


if __name__ == '__main__':
    sys.exit(main())
