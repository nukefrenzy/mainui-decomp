# BSS step 3: C helper buffers, monitor state and COMMON objects

2026-10-22, after `b24cb99` and [step 2](bss-step2.md). Scope: the BSS of the C units
`tu_14acfc`, `tu_14b860`, `tu_14da50`, `tu_14dc18`, `tu_14e0cc` and `tu_14f730`. The changes
below are uncommitted at the time of writing. No objects were renamed and no placeholder
storage was added; BSS remains **160 bytes short** (35,464 of 35,624).

## How the original allocates this region

Original addresses come from pairing references site by site: linked `.text` is identical to
the original, so the same `MOVW`/`MOVT` site refers to the same object in both. The PIC unit
`tu_14f730` reaches its globals through the GOT, whose slot order differs between the two
links, so its objects are paired through the GOT offsets in its code literals instead (pairing
GOT slots by address mislabels them). Two `.rodata` constants that happen to equal BSS
addresses (0x184668, 0x186f00, 0x182690) are not references.

The region has two parts:

1. **Per-file `.bss`** (0x1824b8–0x182680), in link order: `tu_14acfc`, `tu_14b860`,
   `tu_14dc18`, `tu_14e0cc`, then `tu_14f730`'s `deviceInitialized` at 0x18267c.
2. **One COMMON block** (0x182680–0x188a58, the end of BSS): `g_langPath` (`tu_14da50`), 48
   unreferenced bytes, `g_aps`, `g_scanBuf`, `g_wifiStatus` (`tu_14e0cc`), then `deviceKeyA`,
   `deviceTx`, `deviceKeyB`, `deviceCounter`, `deviceRx` (`tu_14f730`).

Three toolchain rules, each confirmed with probes on the project toolchain (GCC 8.2,
binutils 2.30):

- **C file-local statics** (uninitialized or `= 0`) go to the file's `.bss` in **declaration
  order**, including ones declared after functions.
- **Public uninitialized C globals** are COMMON (`-fcommon`, GCC 8's default). An explicit
  `= 0` makes a public object an ordinary `.bss` definition instead.
- **ld places COMMON per input file, in link order, after all `.bss`;** within one file the
  order is the linker's symbol hash-table order (`bfd_hash` of the name modulo the table size;
  32,749 buckets for MainUI). This model reproduces the rebuilt link's COMMON order exactly
  and a four-symbol probe's. An earlier hypothesis that the hash order spans all files was
  wrong: it fit only because six symbols came from one file.

## Changes made

| File | Change | Why |
| --- | --- | --- |
| `tu_14acfc.c` | statics reordered: `g_pwrKeyBuf`, `g_axpBuf`, `g_tinymixCmd`, `g_hotplugMutex`, `g_hotplugCond`, `g_fds`, `g_hotplugRunning` | original `.bss` order |
| `tu_14e0cc.c` | statics gathered in original order: `g_apCount`, `g_wifiBusy`, `g_apMutex`, `g_wifiScanEnable`, `g_wifiConnecting`, `g_monitorThread`, `g_monitorRunning`, `g_statusCallback` (forward-declared `struct WifiStatus`) | original `.bss` order |
| `tu_14e0cc.c` | `g_aps`, `g_scanBuf`, `g_wifiStatus` no longer `static` | they are in the original COMMON block |
| `tu_14da50.c` | `g_langPath` no longer `static` | in the original COMMON block |
| `tu_14f730.c` | `deviceInitialized` defined `= 0` (still public) | original ordinary `.bss` at 0x18267c; staying public keeps the `-fPIC` GOT access unchanged |

`tu_14f730`'s `.text` and relocations were confirmed byte-identical before and after its
change. Pre-change copies of the five files are in ignored `build/bss-step3/`.

## Result

The full checks pass after all changes: every registered match, **22/22 whole units**, 6/6
tooling tests, and the full link audit (`.text` 1,294,000 bytes masked MATCH, 0 raw ARM call
differences; `.rodata`, `.data`, `.ARM.extab`, `.ARM.exidx` exact without masks; 286 imports,
DT_NEEDED order and interpreter; 59 storage extents; 3,095 body calls).

Every object before the COMMON block now sits at a constant offset from its original, rising
only where the original has a slot that is not reproduced:

| Range (original) | Offset (original − rebuilt) | Cause of the step |
| --- | ---: | --- |
| C++ units through `g_pinyinTable` | 104 | 4 (device ID, step 1) + 96 (step 2) + 4 alignment of SQLite's `.bss`, which follows from that 100-byte shift |
| `g_pwrKeyBuf` … `g_monitorThread` | 108 | 4 bytes at the start of `tu_14acfc`'s `.bss` (0x1824b4–0x1824b8): 8-byte section alignment or an unused word |
| `g_monitorRunning` … `g_langPath` | 112 | 4 unreferenced bytes in `tu_14e0cc`'s statics at 0x182670 |

Inside the COMMON block two differences remain:

- **48 unreferenced bytes** after `g_langPath` (0x182700–0x182730), not reproduced.
- **Within-file order depends on names:** the original has `g_aps`, `g_scanBuf`, `g_wifiStatus`
  and `deviceKeyA`, `deviceTx`, `deviceKeyB`, `deviceCounter`, `deviceRx`; the current names
  hash to `g_scanBuf`, `g_aps`, `g_wifiStatus` and `deviceCounter`, `deviceKeyA`, `deviceRx`,
  `deviceKeyB`, `deviceTx`. The original names are stripped.

The 160-byte deficit is fully accounted for: 4 + 96 + 4 + 4 + 4 + 48. None of these bytes is
referenced by any code in the original.

## Open decisions

- **Names:** reproducing the within-file COMMON order requires names (for `tu_14e0cc`'s three
  and `tu_14f730`'s five objects) whose hash order matches the original. The current names
  are invented; choosing replacements to satisfy the hash is a deliberate constraint that
  needs approval.
- **Unreferenced storage:** the 4-byte device-ID slack, step 2's 96 bytes, the 4 bytes at
  `tu_14acfc`'s start (or an 8-aligned object there), the 4 bytes at 0x182670 and the 48 bytes
  after `g_langPath` would need placeholder definitions. Their names, types and partition
  cannot be recovered from either binary.
- After BSS, the GOT and remaining code relocation values still need comparison.

## Reproduce

Analysis aids in ignored `build/bss-step3/`: `pair.py` (site pairing of BSS references between
the original and the audit ELF), `bfdhash.py` (binutils' `bfd_hash`), `probe/` (GCC and ld
probes), and the pairing outputs. They are optional; the link audit and unit checks are the
proof.
