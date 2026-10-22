"""Check import/version extraction on a real ARM dynamic ELF, including COPY relocation data."""
import copy
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('linkaudit', Path(__file__).resolve().parents[1]/'linkaudit.py')
linkaudit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(linkaudit)


@unittest.skipUnless(os.environ.get('MIYOO_TC'), 'set MIYOO_TC to the pinned ARM toolchain')
class DynamicImports(unittest.TestCase):
    def test_versioned_imports_and_copy_object(self):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root/'library.s').write_text('''
.syntax unified
.arm
.text
.global fixture_api
.type fixture_api, %function
fixture_api: bx lr
.data
.global fixture_data
.type fixture_data, %object
.size fixture_data, 4
fixture_data: .word 42
''')
            (root/'entry.s').write_text('''
.syntax unified
.arch armv7-a
.arm
.text
.global fixture_entry
fixture_entry:
    bl fixture_api
    movw r0, #:lower16:fixture_data
    movt r0, #:upper16:fixture_data
    bx lr
''')
            (root/'versions').write_text('FIXTURE_1 { global: fixture_api; fixture_data; local: *; };\n')
            for name in ('entry', 'library'):
                subprocess.run([pre+'as', str(root/(name+'.s')), '-o', str(root/(name+'.o'))],
                               check=True, capture_output=True)
            subprocess.run([pre+'ld', '-shared', '-soname', 'libfixture.so',
                            '--version-script='+str(root/'versions'), str(root/'library.o'),
                            '-o', str(root/'libfixture.so')], check=True, capture_output=True)
            output = root/'fixture.elf'
            subprocess.run([pre+'ld', '-e', 'fixture_entry', '--dynamic-linker=/lib/ld-linux-armhf.so.3',
                            str(root/'entry.o'), '-L'+str(root), '-lfixture', '-o', str(output)],
                           check=True, capture_output=True)
            actual = linkaudit.dynamic_metadata(output)
            version = dict(library='libfixture.so', version='FIXTURE_1')
            self.assertEqual(actual['imports'], dict(fixture_api=version, fixture_data=version))
            self.assertEqual(actual['needed'], ['libfixture.so'])
            self.assertEqual(actual['interpreter'], '/lib/ld-linux-armhf.so.3')
            same = linkaudit.compare_dynamic(actual, actual)
            self.assertEqual(same['version_mismatches'], [])
            self.assertTrue(same['needed_order_matches'])
            changed = copy.deepcopy(actual)
            changed['imports']['fixture_api']['version'] = 'WRONG'
            changed['imports'].pop('fixture_data')
            changed['imports']['extra'] = None
            changed['needed'].append('libextra.so')
            changed['interpreter'] = '/wrong'
            difference = linkaudit.compare_dynamic(actual, changed)
            self.assertEqual(difference['added_imports'], ['extra'])
            self.assertEqual(difference['missing_imports'], ['fixture_data'])
            self.assertEqual(difference['version_mismatches'][0]['symbol'], 'fixture_api')
            self.assertFalse(difference['needed_order_matches'])
            self.assertFalse(difference['interpreter_matches'])


if __name__ == '__main__':
    unittest.main()
