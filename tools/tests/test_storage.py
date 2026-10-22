"""Check real ARM data, BSS, COMMON, and string-pointer initializers without executing code."""
import csv
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from elftools.elf.elffile import ELFFile

SPEC = importlib.util.spec_from_file_location('linkaudit', Path(__file__).resolve().parents[1]/'linkaudit.py')
linkaudit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(linkaudit)


@unittest.skipUnless(os.environ.get('MIYOO_TC'), 'set MIYOO_TC to the pinned ARM toolchain')
class StorageInitializers(unittest.TestCase):
    def test_initializers_and_corruptions(self):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            source, obj, original = folder/'storage.s', folder/'storage.o', folder/'original.elf'
            manifest = folder/'data.tsv'
            assembly = '''
.syntax unified
.arm
.text
.global fixture_entry
fixture_entry: bx lr
.section .rodata
fixture_text: .asciz "hello"
.data
.global fixture_value
.type fixture_value, %object
fixture_value: .word 0x12345678
.size fixture_value, 4
.global fixture_strings
.type fixture_strings, %object
fixture_strings: .word fixture_text
.size fixture_strings, 4
.bss
.global fixture_zero
.type fixture_zero, %object
fixture_zero: .space 8
.size fixture_zero, 8
.comm fixture_common,4,4
'''
            source.write_text(assembly)
            subprocess.run([pre+'as', str(source), '-o', str(obj)], check=True, capture_output=True)
            subprocess.run([pre+'ld', '-e', 'fixture_entry', '-Ttext=0x20000', '-Tdata=0x30000',
                            str(obj), '-o', str(original)], check=True, capture_output=True)
            names = ['fixture_value', 'fixture_strings', 'fixture_zero', 'fixture_common']
            with original.open('rb') as stream:
                symbols = {s.name: (s['st_value'], s['st_size'])
                           for s in ELFFile(stream).get_section_by_name('.symtab').iter_symbols()
                           if s.name in names}
            with manifest.open('w', newline='') as stream:
                writer = csv.writer(stream, delimiter='\t')
                writer.writerow(['file','symbol','addr','size','kind','evidence'])
                for name in names:
                    address, size = symbols[name]
                    writer.writerow([source.name, name, f'{address:x}', size,
                                     'cstring_table' if name == 'fixture_strings' else 'bytes', 'fixture'])
            units = [dict(source=str(source), object=str(obj))]
            results = linkaudit.check_storage(units, original, manifest)
            self.assertEqual([r['status'] for r in results], ['MATCH']*4)
            self.assertEqual(results[1]['strings_checked'], 1)
            for replacement, bad_symbol in [('0x87654321', 'fixture_value'), ('changed', 'fixture_strings')]:
                with self.subTest(corruption=bad_symbol):
                    bad = assembly.replace('0x12345678', replacement) if bad_symbol == 'fixture_value' else assembly.replace('hello', replacement)
                    source.write_text(bad)
                    subprocess.run([pre+'as', str(source), '-o', str(obj)], check=True, capture_output=True)
                    results = linkaudit.check_storage(units, original, manifest)
                    self.assertEqual([r['symbol'] for r in results if r['status']=='MISMATCH'], [bad_symbol])


if __name__ == '__main__':
    unittest.main()
