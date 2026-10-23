#!/usr/bin/env python3
"""Copy function names from the source into ghidra/names.tsv.

The audit link (tools/linkaudit.py) places every function of the 22 Miyoo units at its original
address, because the linked .text is identical to MainUI's. Its symbol table therefore names
every Miyoo function the way src/ does, including template and inline functions. This tool
writes those names (demangled, without parameter lists) into names.tsv for every row inside a
Miyoo unit, so the Ghidra names follow the source. Library rows (cJSON, SQLite) and comments
are kept; a renamed row becomes `confirmed`, and an empty comment gets the source file.

Run after tools/linkaudit.py (it reads build/link-audit/MainUI.audit.elf), and after
tools/manual_names.py whenever names.tsv is regenerated. --check reports differences only and
exits 1 if there are any.
"""
import argparse
import csv
import os
import re
import subprocess
import sys

from elftools.elf.elffile import ELFFile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELF = os.path.join(ROOT, 'build/link-audit/MainUI.audit.elf')
NAMES = os.path.join(ROOT, 'ghidra/names.tsv')
TUMAP = os.path.join(ROOT, 'docs/tu-map.tsv')


def units():
    """(lo, hi, source file) for every Miyoo unit in the map."""
    out = []
    srcs = {os.path.splitext(f)[0]: f for f in os.listdir(os.path.join(ROOT, 'src'))
            if f.endswith(('.c', '.cpp'))}
    for row in csv.reader(open(TUMAP), delimiter='\t'):
        if len(row) < 4 or not (row[3] == 'proven' or row[2] == 'runtime'):
            continue
        src = 'the C runtime start files' if row[2] == 'runtime' else 'src/' + srcs.get(row[2], row[2])
        out.append((int(row[0], 16), int(row[1], 16), src))
    return out


def unit_of(addr, table):
    for lo, hi, src in table:
        if lo <= addr < hi:
            return src
    return None


def elf_functions():
    """address -> list of (symbol name, is_global) for every function symbol."""
    elf = ELFFile(open(ELF, 'rb'))
    funcs = {}
    for s in elf.get_section_by_name('.symtab').iter_symbols():
        if s['st_info']['type'] == 'STT_FUNC' and s['st_value']:
            funcs.setdefault(s['st_value'] & ~1, []).append(
                (s.name, s['st_info']['bind'] == 'STB_GLOBAL'))
    return funcs


def demangle(names):
    tc = os.environ.get('MIYOO_TC')
    tool = os.path.join(tc, 'arm-linux-gnueabihf-c++filt') if tc else 'c++filt'
    out = subprocess.run([tool], input='\n'.join(names), capture_output=True, text=True,
                         check=True).stdout.split('\n')
    return dict(zip(names, out))


def drop_decltype(name):
    """Remove 'decltype (...)' return types, whose parentheses would confuse strip_params."""
    while True:
        i = name.find('decltype (')
        if i < 0:
            return name
        depth = 0
        for j in range(i + 9, len(name)):
            if name[j] == '(':
                depth += 1
            elif name[j] == ')':
                depth -= 1
                if depth == 0:
                    name = (name[:i] + name[j + 1:]).lstrip()
                    break
        else:
            return name


def strip_params(name):
    """'A<B>::f(int) const' -> 'A<B>::f' (cut at the first top-level '(')."""
    name = drop_decltype(name)
    depth = 0
    for i, c in enumerate(name):
        if c == '<':
            depth += 1
        elif c == '>':
            depth -= 1
        elif c == '(' and depth == 0 and i > 0 and not name[:i].endswith('operator'):
            return name[:i]
    return name


def strip_return_type(name):
    """Function templates demangle with their return type first: keep what follows the last
    top-level space ('int* std::f<int>' -> 'std::f<int>')."""
    depth = 0
    cut = 0
    for i, c in enumerate(name):
        if c in '<(':
            depth += 1
        elif c in '>)':
            depth -= 1
        elif c == ' ' and depth == 0 and not name[:i].endswith('operator'):
            cut = i + 1
    return name[cut:]


def plain_name(symbol, demangled):
    """Ghidra name for one symbol: qualified name without parameters or return type; a deleting
    destructor (D0) is X::deletingDtor, the convention used throughout names.tsv."""
    name = compact(strip_return_type(strip_params(demangled)))
    if re.search(r'D0Ev$', symbol) and '::~' in name:
        name = name[:name.rindex('::~')] + '::deletingDtor'
    return name


def compact(name):
    """No whitespace (Ghidra rejects it): '> >' -> '>>', ', ' -> ',', other spaces -> '_'."""
    name = name.replace(', ', ',').replace('> >', '>>').replace('> >', '>>')
    return re.sub(r'\s+', '_', name.strip())


def pick(symbols):
    """Prefer a global symbol, then a mangled one, then the shortest (stable choice)."""
    return sorted(symbols, key=lambda s: (not s[1], not s[0].startswith('_Z'), len(s[0]), s[0]))[0][0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--check', action='store_true', help='report differences, change nothing')
    args = ap.parse_args()
    if not os.path.exists(ELF):
        sys.exit(f'{ELF} missing: run tools/linkaudit.py first')

    table = units()
    funcs = elf_functions()
    lines = open(NAMES).read().split('\n')
    header = lines[0]
    body = [(l.split('\t') + [''] * 5)[:max(5, len(l.split('\t')))] for l in lines[1:] if l]
    wanted = {}
    for row in body:
        addr = int(row[0], 16)
        if unit_of(addr, table) and addr in funcs:
            wanted[row[0]] = pick(funcs[addr])
    plain = demangle(sorted(set(wanted.values())))

    changed = 0
    for row in body:
        sym = wanted.get(row[0])
        if sym is None or row[3] == 'library-exact':
            continue
        # Library rows keep their names, except runtime_<addr> placeholders inside a Miyoo unit.
        if row[3] == 'library' and not row[1].startswith('runtime_'):
            continue
        name = plain_name(sym, plain.get(sym, sym))
        if name == row[1]:
            continue
        # A hand name that only adds a suffix (e.g. an overload's GameDetailWindow_empty) stays.
        if row[3] != 'guess' and row[1].startswith(name) and len(row[1]) > len(name):
            continue
        changed += 1
        if args.check:
            print(f'{row[0]}\t{row[1]} -> {name}')
            continue
        row[1] = name
        row[3] = 'confirmed'
        if name.startswith(('std::', '__gnu_cxx::', 'tiny_utf8::', 'rapidxml::')) or '<' in name.split('::')[0]:
            row[2] = 'library'
        src = unit_of(int(row[0], 16), table)
        if src.startswith('the C runtime'):
            row[2] = 'library'
        if not row[4]:
            row[4] = f'Byte-exact source: {src}.'
    if args.check:
        print(f'{changed} names differ from the source')
        return 1 if changed else 0
    with open(NAMES, 'w') as fh:
        fh.write(header + '\n' + ''.join('\t'.join(r) + '\n' for r in body))
    print(f'{changed} names synced from the source')
    return 0


if __name__ == '__main__':
    sys.exit(main())
