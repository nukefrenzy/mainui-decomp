"""Require raw read-only-data equality, including pointer relocations, on real ARM ELFs."""
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
class RawReadOnlyData(unittest.TestCase):
    def test_strings_pointer_relocations_and_bounds(self):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            def build(name, message='original', target='first', address='0x30000', extra='', section='.rodata'):
                source, obj, output = [root/(name+suffix) for suffix in ('.s', '.o', '.elf')]
                source.write_text(f'''
.syntax unified
.arm
.text
.global first
first: bx lr
second: bx lr
.section {section}
message: .asciz "{message}"
.p2align 2
vtable: .word {target}
{extra}
''')
                subprocess.run([pre+'as', str(source), '-o', str(obj)], check=True, capture_output=True)
                subprocess.run([pre+'ld', '-e', 'first', '-Ttext=0x20000',
                                '--section-start='+section+'='+address, '--emit-relocs',
                                str(obj), '-o', str(output)], check=True, capture_output=True)
                return output
            original = build('original')
            identical = build('identical')
            self.assertEqual(linkaudit.check_rodata(original, identical)['status'], 'MATCH')
            for changed in [build('string', message='ORIGINAL'), build('pointer', target='second')]:
                with self.subTest(changed=changed.stem):
                    result = linkaudit.check_rodata(original, changed)
                    self.assertEqual(result['status'], 'MISMATCH')
                    self.assertGreater(result['mismatching_bytes'], 0)
                    self.assertNotEqual(result['sha256'], result['rebuilt_sha256'])
            # The bad pointer really has a relocation; it must still be compared, not masked.
            with (root/'pointer.elf').open('rb') as stream:
                self.assertEqual(ELFFile(stream).get_section_by_name('.rel.rodata').num_relocations(), 1)
            for changed in [build('address', address='0x40000'), build('size', extra='.byte 0')]:
                result = linkaudit.check_rodata(original, changed)
                self.assertEqual(result['status'], 'MISMATCH')
                self.assertIn('address or size', result['error'])
            data_original = build('data-original', section='.data')
            data_identical = build('data-identical', section='.data')
            self.assertEqual(linkaudit.check_data(data_original, data_identical)['status'], 'MATCH')
            for changed in [build('data-string', message='ORIGINAL', section='.data'),
                            build('data-pointer', target='second', section='.data'),
                            build('data-address', address='0x40000', section='.data'),
                            build('data-size', extra='.byte 0', section='.data')]:
                self.assertEqual(linkaudit.check_data(data_original, changed)['status'], 'MISMATCH')


if __name__ == '__main__':
    unittest.main()
