"""Exercise the actual linked-code proof, including ambiguous byte-identical COMDATs."""
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
class LinkedText(unittest.TestCase):
    def test_relocations_and_code_corruption(self):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root/'probe.s'
            assembly = '''
.syntax unified
.arch armv7-a
.arm
.text
.global entry
entry:
    bl target
    movw r1, #:lower16:datum
    movt r1, #:upper16:datum
    mov r0, #42
    bx lr
target: bx lr
.data
datum: .word 99
'''
            def build(name, text=assembly, data_address='0x30000', emit=True):
                source.write_text(text)
                obj, output = root/(name+'.o'), root/(name+'.elf')
                subprocess.run([pre+'as', str(source), '-o', str(obj)], check=True, capture_output=True)
                cmd = [pre+'ld', '-e', 'entry', '-Ttext=0x20000', '-Tdata='+data_address,
                       str(obj), '-o', str(output)]
                subprocess.run(cmd+(['--emit-relocs'] if emit else []), check=True, capture_output=True)
                return output
            original = build('original')
            moved_data = build('moved', data_address='0x40000')
            result = linkaudit.check_linked_text(original, moved_data)
            self.assertEqual(result['status'], 'MATCH')
            self.assertGreater(result['relocation_words'], 0)
            wrong_call = build('wrong-call', assembly.replace('bl target', 'bl entry'))
            result = linkaudit.check_linked_text(original, wrong_call)
            self.assertEqual(result['status'], 'MATCH')  # Existing call mask remains unchanged.
            self.assertEqual(len(result['control_flow_differences']), 1)
            self.assertNotEqual(result['control_flow_differences'][0]['original_target'],
                                result['control_flow_differences'][0]['rebuilt_target'])
            patched_original = build('patched-original', assembly.replace('bl target', '.inst 0xe320f000'))
            result = linkaudit.check_linked_text(patched_original, original)
            self.assertEqual(result['status'], 'MATCH')
            self.assertEqual(len(result['control_flow_differences']), 1)
            self.assertIsNone(result['control_flow_differences'][0]['original_target'])
            wrong_code = build('wrong', assembly.replace('#42', '#43'))
            result = linkaudit.check_linked_text(original, wrong_code)
            self.assertEqual(result['status'], 'MISMATCH')
            self.assertEqual(result['mismatching_words'], 1)
            stripped_evidence = build('no-relocs', emit=False)
            self.assertIn('missing static', linkaudit.check_linked_text(original, stripped_evidence)['error'])

    def test_same_bytes_different_comdat_identity(self):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first = root/'first.s'
            first.write_text('''
.syntax unified
.arm
.text
.global entry
entry: bx lr
.word shared
.section .text.shared,"axG",%progbits,shared,comdat
.weak shared
.type shared, %function
shared: mov r0, #7
bx lr
''')
            subprocess.run([pre+'as', str(first), '-o', str(root/'first.o')], check=True, capture_output=True)
            def build(name, signature):
                source = root/(name+'.s')
                source.write_text(f'''
.syntax unified
.arm
.text
second: bx lr
.section .text.{signature},"axG",%progbits,{signature},comdat
.weak {signature}
.type {signature}, %function
{signature}: mov r0, #7
bx lr
''')
                obj, output = root/(name+'.o'), root/(name+'.elf')
                subprocess.run([pre+'as', str(source), '-o', str(obj)], check=True, capture_output=True)
                subprocess.run([pre+'ld', '-e', 'entry', '-Ttext=0x20000', '--emit-relocs',
                                str(root/'first.o'), str(obj), '-o', str(output)],
                               check=True, capture_output=True)
                return output
            original, wrong = build('distinct', 'distinct'), build('duplicate', 'shared')
            result = linkaudit.check_linked_text(original, wrong)
            self.assertEqual(result['status'], 'MISMATCH')
            self.assertEqual(result['size']-result['rebuilt_size'], 8)


if __name__ == '__main__':
    unittest.main()
