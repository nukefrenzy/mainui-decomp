#!/usr/bin/env python3
"""Compile the proven launcher units, partially link them, and inventory final-link blockers.

This builds an experimental executable, not a verified replacement. --require-ready still fails
until final layout is proven. Known internal-call/SQLite linkage errors and recovered-storage mismatches
always fail. No stubs, forced duplicate definitions or ignored unresolved symbols are used.
Outputs (objects, linker log, JSON/TSV/Markdown reports) stay in the ignored build directory.
"""
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
import os
import re
from pathlib import Path
import shlex
import subprocess
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parent.parent
SHA = '98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94'


def tsv(path):
    with open(path, newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t', quoting=csv.QUOTE_NONE))


def symbols(path):
    with open(path, 'rb') as stream:
        elf = ELFFile(stream)
        return [dict(name=s.name, bind=s['st_info']['bind'], type=s['st_info']['type'],
                     section=s['st_shndx'], value=s['st_value'], size=s['st_size'])
                for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name]


def dynamic_metadata(path):
    """Inventory real imports (including COPY objects) and their per-symbol version requirements."""
    with open(path, 'rb') as stream:
        elf = ELFFile(stream)
        table = elf.get_section_by_name('.dynsym')
        imported = {i for i,s in enumerate(table.iter_symbols())
                    if s.name and s['st_shndx'] == 'SHN_UNDEF'}
        for rels in elf.iter_sections():
            if isinstance(rels, RelocationSection) and rels['sh_link'] == elf.get_section_index('.dynsym'):
                imported.update(r['r_info_sym'] for r in rels.iter_relocations() if r['r_info_type'] == 20)
        versions = {}
        needs = elf.get_section_by_name('.gnu.version_r')
        if needs:
            for need, auxiliary in needs.iter_versions():
                for aux in auxiliary:
                    versions[aux['vna_other']] = dict(library=need.name, version=aux.name)
        indices = elf.get_section_by_name('.gnu.version')
        imports = {}
        for i in sorted(imported):
            index = indices.get_symbol(i)['ndx'] if indices else None
            if isinstance(index, int):
                index &= 0x7fff
            imports[table.get_symbol(i).name] = versions.get(index)
        dynamic = elf.get_section_by_name('.dynamic')
        interp = elf.get_section_by_name('.interp')
        return dict(imports=imports, needed=[t.needed for t in dynamic.iter_tags()
                    if t.entry.d_tag == 'DT_NEEDED'],
                    interpreter=interp.data().rstrip(b'\0').decode() if interp else None,
                    entry=elf['e_entry'], allocated_sections=[dict(name=s.name,
                    address=s['sh_addr'], size=s['sh_size']) for s in elf.iter_sections()
                    if s['sh_flags'] & 2])


def compare_dynamic(original, rebuilt):
    a, b = original['imports'], rebuilt['imports']
    return dict(added_imports=sorted(b.keys()-a.keys()), missing_imports=sorted(a.keys()-b.keys()),
                version_mismatches=[dict(symbol=n, original=a[n], rebuilt=b[n])
                    for n in sorted(a.keys() & b.keys()) if a[n] != b[n]],
                needed_order_matches=original['needed'] == rebuilt['needed'],
                interpreter_matches=original['interpreter'] == rebuilt['interpreter'])


def check_linked_text(original, rebuilt):
    """Check actual ld output, using the existing unit proof's relocation/call-word policy.

    --emit-relocs preserves static relocation sites in the audit ELF. Unlike a per-unit walk,
    this comparison observes actual COMDAT selection across every input object. Final absolute
    relocation values and call destinations are deliberately not claimed by this code proof.
    """
    with open(original, 'rb') as stream:
        expected = ELFFile(stream).get_section_by_name('.text')
        address, data = expected['sh_addr'], expected.data()
    result = dict(status='MISMATCH', address=address, size=len(data), relocation_words=0,
                  call_words=0, compared_words=0, mismatching_words=0, first_mismatches=[],
                  control_flow_differences=[])
    with open(rebuilt, 'rb') as stream:
        elf = ELFFile(stream)
        section = elf.get_section_by_name('.text')
        if section['sh_addr'] != address or section['sh_size'] != len(data):
            result.update(error='linked .text address or size differs',
                          rebuilt_address=section['sh_addr'], rebuilt_size=section['sh_size'])
            return result
        body = section.data()
        mask = bytearray(len(body))
        relocations = [s for s in elf.iter_sections() if isinstance(s, RelocationSection)
                       and s['sh_info'] == elf.get_section_index('.text')]
        if not relocations:
            result['error'] = 'missing static .text relocations; link with --emit-relocs'
            return result
        # ARM mapping symbols distinguish instructions from embedded literal-pool words.
        mappings = sorted((s['st_value'], s.name.split('.')[0])
                          for s in elf.get_section_by_name('.symtab').iter_symbols()
                          if s['st_shndx'] == elf.get_section_index('.text')
                          and s.name.split('.')[0] in ('$a', '$d', '$t'))
        def call(word):
            return ((word & 0x0f000000 == 0x0b000000 and word >> 28 != 15)
                    or word & 0xfe000000 == 0xfa000000)
        def target(at, word):
            if not call(word):
                return None
            displacement = (word & 0xffffff) << 2
            if word & 0xfe000000 == 0xfa000000:
                displacement |= (word >> 23) & 2  # BLX immediate H bit.
            if displacement & 0x2000000:
                displacement -= 0x4000000
            return at+8+displacement
        for rels in relocations:
            for rel in rels.iter_relocations():
                offset = rel['r_offset'] - address
                if offset < 0 or offset+4 > len(body) or offset % 4:
                    result['error'] = 'unaligned or out-of-range text relocation'
                    return result
                mask[offset:offset+4] = b'\1'*4
                result['relocation_words'] += 1
        mapping_index, mapping = 0, None
        for offset in range(0, len(body), 4):
            word = int.from_bytes(body[offset:offset+4], 'little')
            if call(word):
                mask[offset:offset+4] = b'\1'*4
                result['call_words'] += 1
            while mapping_index < len(mappings) and mappings[mapping_index][0] <= address+offset:
                mapping = mappings[mapping_index][1]
                mapping_index += 1
            original_word = int.from_bytes(data[offset:offset+4], 'little')
            if mapping == '$a' and original_word != word and (call(word) or call(original_word)):
                result['control_flow_differences'].append(dict(address=address+offset,
                    original_word=original_word, rebuilt_word=word,
                    original_target=target(address+offset, original_word),
                    rebuilt_target=target(address+offset, word)))
        for offset in range(0, len(body), 4):
            if any(mask[offset:offset+4]):
                continue
            result['compared_words'] += 1
            if body[offset:offset+4] != data[offset:offset+4]:
                result['mismatching_words'] += 1
                if len(result['first_mismatches']) < 16:
                    result['first_mismatches'].append(address+offset)
        result['status'] = 'MISMATCH' if result['mismatching_words'] else 'MATCH'
    return result


def imm16(word):
    return ((word >> 4) & 0xf000) | (word & 0xfff)


def source_evidence(objects, original):
    """Use original .text call targets / MOVW+MOVT pairs, independently of masked matches.

Only each unit's body has an established direct address mapping here. Tail/vtable references
remain in the inventory, but are not guessed. Subtract encoded REL addends from data addresses.
    """
    functions = defaultdict(set)
    with open(original, 'rb') as stream:
        text = ELFFile(stream).get_section_by_name('.text')
        data, base = text.data(), text['sh_addr']
    calls, addresses = defaultdict(list), defaultdict(set)
    for unit in objects:
        with open(ROOT / unit['object'], 'rb') as stream:
            elf = ELFFile(stream)
            st = elf.get_section_by_name('.symtab')
            for sym in st.iter_symbols():
                if isinstance(sym['st_shndx'], int) and sym['st_info']['type'] == 'STT_FUNC':
                    if elf.get_section(sym['st_shndx']).name == '.text':
                        functions[unit['lo'] + sym['st_value']].add(sym.name)
            rels = elf.get_section_by_name('.rel.text')
            if rels is None:
                continue
            body = elf.get_section_by_name('.text').data()
            relocations = {r['r_offset']: r for r in rels.iter_relocations()}
            for off, rel in relocations.items():
                name = st.get_symbol(rel['r_info_sym']).name
                at = unit['lo'] + off
                word = int.from_bytes(data[at-base:at-base+4], 'little')
                kind = rel['r_info_type']
                if kind in (28, 29) and word >> 28 != 15 and word & 0x0e000000 == 0x0a000000:
                    displacement = (word & 0xffffff) << 2
                    if displacement & 0x2000000:
                        displacement -= 0x4000000
                    calls[name].append(dict(file=unit['source'], at=at, target=at+8+displacement))
                if kind == 43 and off+4 in relocations:
                    high = relocations[off+4]
                    if high['r_info_type'] != 44 or high['r_info_sym'] != rel['r_info_sym']:
                        continue
                    original_hi = int.from_bytes(data[at-base+4:at-base+8], 'little')
                    if word & 0x0ff00000 != 0x03000000 or original_hi & 0x0ff00000 != 0x03400000:
                        continue
                    object_lo = int.from_bytes(body[off:off+4], 'little')
                    object_hi = int.from_bytes(body[off+4:off+8], 'little')
                    # Both halves must address the same ARM register.
                    if ((word ^ original_hi) | (object_lo ^ object_hi)) & 0xf000:
                        continue
                    value = imm16(word) | imm16(original_hi) << 16
                    # ARM REL MOVW/MOVT each encode the same signed 16-bit addend,
                    # rather than the low/high halves of a 32-bit addend.
                    if imm16(object_lo) != imm16(object_hi):
                        continue
                    addend = imm16(object_lo)
                    if addend & 0x8000:
                        addend -= 0x10000
                    addresses[name].add((value-addend) & 0xffffffff)
    return functions, calls, addresses


def check_resolved_calls(functions, calls):
    """Catch resolved names whose definitions disagree with original body call destinations."""
    definitions_by_name = defaultdict(set)
    for address, defined_names in functions.items():
        for name in defined_names:
            definitions_by_name[name].add(address)
    resolved_checked, resolved_mismatches = 0, []
    for name, sites in calls.items():
        if name not in definitions_by_name:
            continue
        for site in sites:
            resolved_checked += 1
            if site['target'] not in definitions_by_name[name]:
                resolved_mismatches.append(dict(symbol=name, **site,
                    expected=sorted(definitions_by_name[name])))
    return resolved_checked, resolved_mismatches


def check_storage(objects, original, manifest):
    """Compare named initial storage exactly; pointer-table entries compare pointed-to strings.

No byte/relocation masks are applied. BSS and COMMON must match original zero storage. Final
absolute addresses are not claimed: these are initializer and extent checks before final linking.
    """
    by_file = {Path(unit['source']).name: ROOT/unit['object'] for unit in objects}
    results = []
    with open(original, 'rb') as stream:
        elf = ELFFile(stream)
        sections = [dict(addr=s['sh_addr'], size=s['sh_size'], name=s.name,
                         zero=s['sh_type'] == 'SHT_NOBITS',
                         data=b'' if s['sh_type'] == 'SHT_NOBITS' else s.data())
                    for s in elf.iter_sections() if s['sh_flags'] & 2]
    def original_bytes(address, size):
        for sec in sections:
            if sec['addr'] <= address and address+size <= sec['addr']+sec['size']:
                return bytes(size) if sec['zero'] else sec['data'][address-sec['addr']:address-sec['addr']+size]
        raise ValueError(f'original storage range is unmapped: {address:#x}+{size}')
    def original_string(address):
        for sec in sections:
            if sec['addr'] <= address < sec['addr']+sec['size'] and not sec['zero']:
                rest = sec['data'][address-sec['addr']:]
                end = rest.find(b'\0')
                if end >= 0:
                    return rest[:end+1]
        raise ValueError(f'original string is unmapped or unterminated: {address:#x}')
    rows = tsv(manifest)
    if not rows:
        raise ValueError('storage manifest has no entries')
    keys = [(r['file'], r['symbol']) for r in rows]
    if len(set(keys)) != len(keys):
        raise ValueError('duplicate storage manifest definition')
    for row in rows:
        address, size = int(row['addr'], 16), int(row['size'])
        result = dict(symbol=row['symbol'], file=row['file'], address=address, size=size,
                      kind=row['kind'], status='MATCH', strings_checked=0, evidence=row['evidence'])
        try:
            with open(by_file[row['file']], 'rb') as stream:
                obj = ELFFile(stream)
                table = obj.get_section_by_name('.symtab')
                candidates = [s for s in table.iter_symbols() if s.name == row['symbol']
                              and s['st_shndx'] != 'SHN_UNDEF']
                if len(candidates) != 1:
                    raise ValueError(f'expected one definition, found {len(candidates)}')
                symbol = candidates[0]
                if symbol['st_size'] != size:
                    raise ValueError(f'symbol size {symbol["st_size"]} differs from recorded {size}')
                expected = original_bytes(address, size)
                idx, offset = symbol['st_shndx'], symbol['st_value']
                if idx == 'SHN_COMMON':
                    result['object_section'] = 'COMMON'
                    if row['kind'] != 'bytes' or expected != bytes(size):
                        raise ValueError('COMMON storage does not match original zero bytes')
                else:
                    if not isinstance(idx, int):
                        raise ValueError('definition has no allocated storage section')
                    sec = obj.get_section(idx)
                    if offset+size > sec['sh_size']:
                        raise ValueError('symbol extent exceeds its storage section')
                    result['object_section'] = sec.name
                    content = bytes(size) if sec['sh_type'] == 'SHT_NOBITS' else sec.data()[offset:offset+size]
                    rels = {r['r_offset']-offset: r for rs in obj.iter_sections()
                            if isinstance(rs, RelocationSection) and rs['sh_info'] == idx
                            for r in rs.iter_relocations() if offset <= r['r_offset'] < offset+size}
                    if row['kind'] == 'bytes':
                        if rels:
                            raise ValueError('unexpected relocation in byte-checked storage')
                        if content != expected:
                            first = next((n for n,(a,b) in enumerate(zip(content, expected)) if a != b), 0)
                            raise ValueError(f'initial bytes differ at +{first:#x}')
                    elif row['kind'] == 'cstring_table':
                        if size % 4 or any(n % 4 for n in rels):
                            raise ValueError('unaligned pointer-table entry')
                        for n in range(0, size, 4):
                            pointer = int.from_bytes(expected[n:n+4], 'little')
                            value = int.from_bytes(content[n:n+4], 'little')
                            rel = rels.get(n)
                            if pointer == 0 and rel is None and value == 0:
                                continue
                            if rel is None or rel['r_info_type'] != 2:  # R_ARM_ABS32
                                raise ValueError(f'entry {n//4} requires an ABS32 string relocation')
                            target = table.get_symbol(rel['r_info_sym'])
                            if not isinstance(target['st_shndx'], int):
                                raise ValueError(f'entry {n//4} has no local string definition')
                            target_sec = obj.get_section(target['st_shndx'])
                            start = target['st_value'] + value  # ELF REL addend
                            data = target_sec.data()
                            end = data.find(b'\0', start)
                            if start < 0 or end < 0 or data[start:end+1] != original_string(pointer):
                                raise ValueError(f'string entry {n//4} differs')
                            result['strings_checked'] += 1
                    else:
                        raise ValueError(f'unsupported storage check kind: {row["kind"]}')
        except (KeyError, OSError, ValueError) as error:
            result.update(status='MISMATCH', error=str(error))
        results.append(result)
    return results


def audit(args):
    os.chdir(ROOT)
    original = Path(args.orig).resolve()
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA:
        raise ValueError('original binary hash differs from the documented target')
    if not (ROOT/'src/data.tsv').is_file():
        raise ValueError('required storage manifest src/data.tsv is missing')
    out = Path(args.out).resolve()
    # Keep the report's reproducible command paths and generated artifacts inside the workspace.
    out.relative_to(ROOT)
    if not out.is_relative_to(ROOT / 'build'):
        raise ValueError('--out must be inside build/')
    out.mkdir(parents=True, exist_ok=True)
    # A failed rebuild must not leave reports or linked artifacts from a previous successful audit.
    for name in ('launcher.o', 'MainUI.audit.elf', 'audit.json', 'report.md', 'unresolved.tsv',
                 'commands.txt', 'partial-link.log', 'launcher.map', 'final-link.log', 'final-link.map'):
        (out/name).unlink(missing_ok=True)
    tc = os.environ.get('MIYOO_TC')
    if not tc:
        tc = subprocess.check_output(['scripts/fetch-toolchain.sh'], text=True).strip()
    pre = str(Path(tc).resolve() / 'arm-linux-gnueabihf-')
    version = subprocess.check_output([pre+'gcc', '--version'], text=True).splitlines()[0]
    if '8.2-2018-08 (arm-rel-8.23)' not in version:
        raise ValueError('MIYOO_TC must select the pinned Arm GNU 8.2-2018.08 toolchain')
    environment = dict(os.environ, MIYOO_TC=str(Path(tc).resolve()))
    library_build = subprocess.run(['scripts/build-libs.sh', str(original)], env=environment,
                                   capture_output=True, text=True)
    (out/'embedded-libs.log').write_text(library_build.stdout+library_build.stderr)
    if library_build.returncode:
        raise RuntimeError(f'embedded library verification failed (see {out}/embedded-libs.log)')
    target_dir = Path(subprocess.check_output(['scripts/fetch-target-libs.sh'], text=True).strip())
    original_dynamic = dynamic_metadata(original)
    library_inputs = []
    runtime = {'libm.so.6', 'libstdc++.so.6', 'libdl.so.2', 'libpthread.so.0',
               'libgcc_s.so.1', 'libc.so.6'}
    for name in original_dynamic['needed']:
        path = (Path(subprocess.check_output([pre+'g++', '-print-file-name='+name], text=True).strip())
                if name in runtime else target_dir/name)
        if not path.is_file():
            raise ValueError(f'missing target library: {name}: {path}')
        library_inputs.append(dict(name=name, path=str(path.resolve()),
            sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            source='pinned_toolchain' if name in runtime else 'my283_sdk'))
    manifest = tsv('src/matches.tsv')
    units = {r['name']: r for r in tsv('docs/tu-map.tsv') if r['confidence'] == 'proven'}
    sources = sorted(list(Path('src').glob('tu_*.cpp')) + list(Path('src').glob('tu_*.c')))
    if {s.stem for s in sources} != set(units):
        raise ValueError('source files and proven TU map differ; complete the unit proofs first')
    fetch = {}
    compiled, refs, providers = [], defaultdict(set), defaultdict(list)
    commands, warnings = [['scripts/build-libs.sh', str(original)], ['scripts/fetch-target-libs.sh']], []
    for source in sources:
        rows = [r for r in manifest if r['file'] == source.name]
        flags = {r.get('cflags', '-') for r in rows}
        if len(flags) != 1:
            raise ValueError(f'{source}: missing or inconsistent per-file cflags')
        deps = sorted({d for r in rows for d in r['deps'].split(',') if d != '-'})
        cmd = [pre + ('g++' if source.suffix == '.cpp' else 'gcc'), '-O0', '-fsigned-char']
        for dep in deps:
            if dep not in ('tinyutf8', 'rapidxml'):
                raise ValueError(f'unknown dependency: {dep}')
            if dep not in fetch:
                fetch[dep] = subprocess.check_output([f'scripts/fetch-{dep}.sh'], text=True).strip()
            cmd.append('-I' + fetch[dep])
        extra = next(iter(flags))
        if extra != '-':
            cmd.extend(shlex.split(extra))
        obj = (out / (source.stem + '.o')).relative_to(ROOT)
        cmd.extend(['-c', str(source), '-o', str(obj)])
        commands.append(cmd)
        result = subprocess.run(cmd, capture_output=True, text=True)
        (out / (source.stem + '.compile.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f'{source}: compilation failed (see {out})')
        if result.stderr:
            warnings.append(str(source))
        compiled.append(dict(source=str(source), object=str(obj), lo=int(units[source.stem]['lo'], 16)))
        for sym in symbols(obj):
            if sym['section'] == 'SHN_UNDEF':
                refs[sym['name']].add(str(source))
            elif sym['bind'] != 'STB_LOCAL':
                providers[sym['name']].append(dict(file=str(source), **sym))
        print(f'compiled {source}', flush=True)
    libraries = [dict(source='build/embedded-libs/cJSON.c', object='build/embedded-libs/cjson.o', lo=0x51b60),
                 dict(source='build/embedded-libs/sqlite-autoconf-3300100/sqlite3.c',
                      object='build/embedded-libs/sqlite3.o', lo=0x57920)]
    for unit in libraries:
        with open(unit['object'], 'rb') as stream:
            unit['text_bytes'] = ELFFile(stream).get_section_by_name('.text')['sh_size']
        unit['sha256'] = hashlib.sha256(Path(unit['object']).read_bytes()).hexdigest()
        for sym in symbols(unit['object']):
            if sym['section'] == 'SHN_UNDEF':
                refs[sym['name']].add(unit['source'])
            elif sym['bind'] != 'STB_LOCAL':
                providers[sym['name']].append(dict(file=unit['source'], **sym))
    # Preserve established original body order, including the two embedded libraries.
    objects = sorted(compiled + libraries, key=lambda u: u['lo'])
    combined = out / 'launcher.o'
    if combined.exists():
        combined.unlink()  # Never leave a stale successful partial link after a failed rerun.
    cmd = [pre+'ld', '-r', '-Map='+str(out/'launcher.map'), '-o', str(combined),
           *(u['object'] for u in objects)]
    commands.append(cmd)
    result = subprocess.run(cmd, capture_output=True, text=True)
    (out/'partial-link.log').write_text(result.stdout+result.stderr)
    if result.returncode:
        combined.unlink(missing_ok=True)
    final_output = out/'MainUI.audit.elf'
    if final_output.exists():
        final_output.unlink()
    # Original arithmetic helpers are static libgcc code, but exception unwinding uses libgcc_s.
    # Select arithmetic objects before dynamic libgcc_s can satisfy their references.
    final_cmd = [pre+'g++', *(u['object'] for u in objects), '-lgcc', '-Wl,--emit-relocs',
                 '-L'+str(target_dir), '-Wl,-rpath-link,'+str(target_dir), '-Wl,--no-as-needed',
                 *('-l:'+name for name in original_dynamic['needed']),
                 '-Wl,-Map='+str(out/'final-link.map'), '-o', str(final_output)]
    commands.append(final_cmd)
    final_result = subprocess.run(final_cmd, capture_output=True, text=True)
    (out/'final-link.log').write_text(final_result.stdout+final_result.stderr)
    if final_result.returncode:
        final_output.unlink(missing_ok=True)
    rebuilt_dynamic = dynamic_metadata(final_output) if final_result.returncode == 0 else None
    dynamic_comparison = compare_dynamic(original_dynamic, rebuilt_dynamic) if rebuilt_dynamic else None
    linked_text = check_linked_text(original, final_output) if rebuilt_dynamic else None
    placements = []
    if rebuilt_dynamic:
        linked_symbols = {s['name']: s['value'] for s in symbols(final_output)
                          if s['bind'] != 'STB_LOCAL' and s['section'] != 'SHN_UNDEF'}
        for unit in compiled:
            with open(unit['object'], 'rb') as stream:
                elf = ELFFile(stream)
                body_index = elf.get_section_index('.text')
                anchors = [s for s in symbols(unit['object']) if s['section'] == body_index
                           and s['type'] == 'STT_FUNC' and s['bind'] != 'STB_LOCAL'
                           and s['name'] in linked_symbols]
                if anchors:
                    anchor = min(anchors, key=lambda s: s['value'])
                    expected = unit['lo']+anchor['value']
                    actual = linked_symbols[anchor['name']]
                    placements.append(dict(source=unit['source'], symbol=anchor['name'],
                        original=expected, rebuilt=actual, delta=actual-expected))
    unresolved = ({s['name'] for s in symbols(combined) if s['section'] == 'SHN_UNDEF'}
                  if result.returncode == 0 else set(refs)-set(providers))
    duplicates = {name: rows for name, rows in providers.items()
                  if len([r for r in rows if r['bind'] == 'STB_GLOBAL' and r['section'] != 'SHN_COMMON']) > 1}
    with open(original, 'rb') as stream:
        elf = ELFFile(stream)
        dynamic_symbols = {s.name: s for s in elf.get_section_by_name('.dynsym').iter_symbols()}
        imports = {n for n,s in dynamic_symbols.items() if s['st_shndx'] == 'SHN_UNDEF'}
        # Imported objects may be defined in the executable by R_ARM_COPY relocations.
        rel = elf.get_section_by_name('.rel.dyn')
        if rel:
            imports.update(elf.get_section(rel['sh_link']).get_symbol(r['r_info_sym']).name
                           for r in rel.iter_relocations() if r['r_info_type'] == 20)
        needed = [t.needed for t in elf.get_section_by_name('.dynamic').iter_tags()
                  if t.entry.d_tag == 'DT_NEEDED']
        sections = [dict(name=s.name, address=s['sh_addr'], size=s['sh_size'])
                    for s in elf.iter_sections() if s['sh_flags'] & 2]
        entry = elf['e_entry']
        interp = elf.get_section_by_name('.interp').data().rstrip(b'\0').decode()
    names = sorted(set(unresolved) | set(providers))
    demangled = subprocess.check_output([pre+'c++filt'], input='\n'.join(names), text=True).splitlines()
    dem = dict(zip(names, demangled))
    by_function = defaultdict(list)
    for name in providers:
        if '(' in dem[name]:
            by_function[dem[name].split('(', 1)[0]].append(name)
    library_names = {r['name'] for r in tsv('ghidra/libnames.tsv')}
    globals_by_addr = {int(r['addr'], 16): r['name'] for r in tsv('ghidra/globals.tsv')}
    functions, calls, addresses = source_evidence(objects, original)
    declared_data = defaultdict(list)
    declaration = re.compile(r'^\s*extern\s+[^();]+?\b(\w+)\s*(?:\[[^;]*\])?\s*;\s*//\s*(0x[0-9a-fA-F]+)')
    for unit in compiled:
        for line_number, line in enumerate(Path(unit['source']).read_text().splitlines(), 1):
            match = declaration.match(line)
            if match:
                declared_data[match[1]].append(dict(file=unit['source'], line=line_number, address=int(match[2], 16)))
    resolved_checked, resolved_mismatches = check_resolved_calls(functions, calls)
    records = []
    for name in sorted(unresolved):
        label = dem[name]
        base = label.split('(', 1)[0]
        targets = sorted({c['target'] for c in calls.get(name, [])})
        exact_providers = sorted({p for target in targets for p in functions.get(target, []) if p != name})
        candidates = sorted(set(by_function.get(base, [])) - {name})
        if name in imports:
            category = 'original_dynamic_import'
        elif base in library_names:
            category = 'embedded_library_declaration' if name.startswith('_Z') and base.startswith('sqlite3_') else 'embedded_library'
        elif exact_providers:
            category = 'internal_call_mismatch'
        elif candidates:
            category = 'signature_candidate'
        elif name in ('_GLOBAL_OFFSET_TABLE_', '__dso_handle'):
            category = 'linker_or_crt'
        elif name.startswith('g_') or name in declared_data or (name in addresses and '(' not in label):
            category = 'data_definition'
        else:
            category = 'unclassified'
        addr = sorted(addresses.get(name, []))
        records.append(dict(symbol=name, signature=label, category=category,
                            referenced_by=sorted(refs[name]), original_call_targets=targets,
                            original_data_addresses=addr,
                            source_data_declarations=declared_data.get(name, []),
                            original_global_names=[globals_by_addr[a] for a in addr if a in globals_by_addr],
                            target_definitions=exact_providers, signature_candidates=candidates))
    aliases = defaultdict(list)
    for name, values in addresses.items():
        if not name.startswith('.LC') and not name.startswith('$'):
            for addr in values:
                if name in unresolved or name in providers:
                    aliases[addr].append(name)
    aliases = [dict(address=addr, symbols=sorted(set(names))) for addr,names in sorted(aliases.items())
               if len(set(names)) > 1 and any(n in unresolved for n in names)]
    storage = check_storage(compiled, original, ROOT/'src/data.tsv')
    storage_errors = [r for r in storage if r['status'] != 'MATCH']
    counts = dict(sorted(Counter(r['category'] for r in records).items()))
    report = dict(original_sha256=SHA, compiler=version, units=len(compiled), embedded_libraries=libraries,
                  library_inputs=library_inputs, rebuilt_dynamic=rebuilt_dynamic,
                  dynamic_comparison=dynamic_comparison, body_placements=placements,
                  linked_text=linked_text,
                  partial_link_returncode=result.returncode,
                  final_link_returncode=final_result.returncode,
                  executable_ready=False, unresolved_count=len(records), categories=counts,
                  duplicate_definitions=duplicates, compile_warnings=warnings, unresolved=records,
                  storage=storage, storage_mismatches=storage_errors,
                  resolved_body_calls_checked=resolved_checked,
                  resolved_body_call_mismatches=resolved_mismatches,
                  original_address_aliases=aliases, original=dict(entry=entry, interpreter=interp,
                  needed=needed, allocated_sections=sections), commands=commands)
    (out/'audit.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'commands.txt').write_text('\n'.join(shlex.join(c) for c in commands)+'\n')
    fields = ['category','symbol','signature','referenced_by','original_call_targets',
              'original_data_addresses','target_definitions','signature_candidates']
    with open(out/'unresolved.tsv', 'w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t');writer.writerow(fields)
        for row in records:
            writer.writerow([json.dumps(row[k]) if isinstance(row[k], list) else row[k] for k in fields])
    lines = ['# Link-readiness audit', '', f'Compiled units: {len(compiled)}.',
             f'Partial link (`ld -r`): {"PASS" if result.returncode == 0 else "FAIL"}.',
             f'Embedded libraries: {len(libraries)}; target/runtime libraries: {len(library_inputs)}.',
             f'Executable link with target SDK: {"PASS" if final_result.returncode == 0 else "FAIL"}.',
             f'Unresolved symbols after partial link: {len(records)}. This is not a verified executable.', '',
             '| Category | Count |', '| --- | --- |']
    lines += [f'| {k} | {v} |' for k,v in counts.items()]
    lines += ['', f'Storage objects checked: {len(storage)}; mismatches: {len(storage_errors)}; strings: {sum(r["strings_checked"] for r in storage)}.',
              f'Resolved body calls checked: {resolved_checked}; destination mismatches: {len(resolved_mismatches)}.',
              'Full symbol evidence: `unresolved.tsv` / `audit.json`.',
              'Exact commands: `commands.txt`. Link output: `partial-link.log` / `launcher.map`.',
              'Executable-link diagnostics: `final-link.log` / `final-link.map`.',
              'Library proof: `embedded-libs.log`; SDK/runtime paths and SHA-256: `audit.json`.', '',
              'Call evidence covers .text bodies only; tail/vtable targets are not inferred.',
              'Address aliases and same-name signature candidates need declaration/type review.',
              'ld -r success does not check class definitions, calling ABI compatibility, final imports,',
              'data initialization, runtime behavior or byte-exact linked layout.', '']
    if dynamic_comparison:
        lines += [f'Final imports: {len(rebuilt_dynamic["imports"])}; added: {len(dynamic_comparison["added_imports"])}; '
                  f'missing: {len(dynamic_comparison["missing_imports"])}; '
                  f'version mismatches: {len(dynamic_comparison["version_mismatches"])}.',
                  f'DT_NEEDED order matches: {dynamic_comparison["needed_order_matches"]}; '
                  f'interpreter matches: {dynamic_comparison["interpreter_matches"]}.',
                  'Link success and matching imports do not establish original layout or device safety.', '']
        lines += [f'Actual linked .text (relocation/call words masked): {linked_text["status"]}; '
                  f'{linked_text["size"]:,} bytes; {linked_text["mismatching_words"]} differing unmasked words.',
                  f'Raw ARM call/patch differences requiring review: {len(linked_text["control_flow_differences"])}.',
                  'This checks actual COMDAT retention; absolute relocation/call values remain unproven.', '']
        first_shift = next((p for p in placements if p['delta']), None)
        if first_shift:
            lines += [f'First displaced TU body anchor: {first_shift["source"]}: '
                      f'{first_shift["original"]:#x} -> {first_shift["rebuilt"]:#x} '
                      f'({first_shift["delta"]:+d} bytes).', '']
    (out/'report.md').write_text('\n'.join(lines))
    print('\n'.join(lines[:len(counts)+10]), flush=True)
    print(f'Reports: {out.relative_to(ROOT)}/report.md, audit.json, unresolved.tsv')
    if dynamic_comparison:
        print('\n'.join(lines[-5:]), flush=True)
    failed = (result.returncode or resolved_mismatches or storage_errors
              or counts.get('internal_call_mismatch') or counts.get('embedded_library_declaration')
              or (dynamic_comparison and (dynamic_comparison['added_imports']
                  or dynamic_comparison['missing_imports'] or dynamic_comparison['version_mismatches']
                  or not dynamic_comparison['needed_order_matches']
                  or not dynamic_comparison['interpreter_matches']))
              or (args.require_link and final_result.returncode)
              or (linked_text and linked_text['status'] != 'MATCH')
              or (args.require_ready and not report['executable_ready']))
    return 1 if failed else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--orig', default=str(ROOT/'MainUI-354-clean'))
    parser.add_argument('--out', default=str(ROOT/'build/link-audit'))
    parser.add_argument('--require-link', action='store_true', help='also fail if executable linking fails')
    parser.add_argument('--require-ready', action='store_true', help='also require proven final layout (not yet implemented)')
    args = parser.parse_args()
    try:
        return audit(args)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'linkaudit: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
