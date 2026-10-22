"""Exercise audit evidence extraction against actual ARM relocations, without executing ARM code."""
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
class ArmRelocationEvidence(unittest.TestCase):
    def test_body_targets_and_rel_addends(self):
        for base, addend in [(0x30000, 12), (0x3fff8, 12), (0x40000, -4)]:
            with self.subTest(base=base, addend=addend):
                self.check_evidence(base, addend)

    def check_evidence(self, base, addend):
        pre = str(Path(os.environ['MIYOO_TC'])/'arm-linux-gnueabihf-')
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            source, obj, original = folder/'probe.s', folder/'probe.o', folder/'original.elf'
            source.write_text(f'''
.syntax unified
.arch armv7-a
.arm
.text
.global probe
.type probe, %function
probe:
    movw r3, #:lower16:missing_data{addend:+d}
    movt r3, #:upper16:missing_data{addend:+d}
    bl missing_call
    bl missing_back
    bx lr
.size probe, .-probe
''')
            assembled = subprocess.run([pre+'as', str(source), '-o', str(obj)], capture_output=True, text=True)
            self.assertEqual(assembled.returncode, 0, assembled.stderr)
            subprocess.run([pre+'ld', '-Ttext=0x20000', '-e', 'probe',
                            f'--defsym=missing_data={base:#x}', '--defsym=missing_call=0x21000', '--defsym=missing_back=0x1f000',
                            str(obj), '-o', str(original)], check=True, capture_output=True)
            functions, calls, addresses = linkaudit.source_evidence(
                [dict(object=str(obj), source=str(source), lo=0x20000)], original)
            self.assertEqual(functions[0x20000], {'probe'})
            self.assertEqual(calls['missing_call'],
                             [dict(file=str(source), at=0x20008, target=0x21000)])
            self.assertEqual(calls['missing_back'],
                             [dict(file=str(source), at=0x2000c, target=0x1f000)])
            # Recover the symbol base, not the referenced element's address.
            self.assertEqual(addresses['missing_data'], {base})
            # A relocation-masked caller can keep its instructions while binding to the
            # wrong defined function. Resolve that rewritten reference against the original.
            source.write_text(source.read_text().replace('bl missing_call', 'bl probe'))
            assembled = subprocess.run([pre+'as', str(source), '-o', str(obj)], capture_output=True, text=True)
            self.assertEqual(assembled.returncode, 0, assembled.stderr)
            functions, calls, _ = linkaudit.source_evidence(
                [dict(object=str(obj), source=str(source), lo=0x20000)], original)
            count, errors = linkaudit.check_resolved_calls(functions, calls)
            self.assertEqual(count, 1)
            self.assertEqual(errors, [dict(symbol='probe', file=str(source), at=0x20008,
                                          target=0x21000, expected=[0x20000])])



if __name__ == '__main__':
    unittest.main()
