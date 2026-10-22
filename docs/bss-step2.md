# BSS step 2: the 96-byte interval at 0x181b18

2026-10-22, after `b24cb99`. This pass is limited to step 2; C helper/COMMON ordering
(step 3) awaits the user's confirmation. No source, manifest or tool changed.

## Result

Original **0x181b18–0x181b78** is **96 bytes of zero-initialized storage that nothing in
the program ever references**, defined in the main-program unit `tu_049c3c` between
`g_lastVolumeTime` and the file-local `g_loggedWifi`. The same unused storage exists, at
the same position, in a second Miyoo build. Its size and position are proven; its name,
type and partition (one object or several) are not recorded anywhere in either binary.
It accounts for **96 of the 160 missing BSS bytes**. The source keeps no placeholder for
it (decision recorded below).

## Neighbours and owner

| Original | Object | Referenced by |
| --- | --- | --- |
| 0x181b00 | `g_romListPositions` (12 bytes) | menus, `main`, the unit's static init |
| 0x181b0c | `g_running` | `onSIGTERM`, `main` |
| 0x181b10 | `g_lastBattery` | `pollStatusA_pushEvent` (word at offset 0 only) |
| 0x181b14 | `g_lastVolumeTime` | `pollVolume_pushEvent` (word at offset 0 only) |
| **0x181b18–0x181b77** | **96 bytes, no reference** | — |
| 0x181b78 | `g_loggedWifi` (file-local) | `wifiStatusCallback` (word at offset 0 only) |
| 0x181b7c | `g_testResults` | hardware-test windows (next unit, tu_04cb68) |

Both neighbours belong to `tu_049c3c`, so the interval is inside that unit's BSS.

## Evidence that it is unreferenced and uninitialized

- **No direct reference.** A scan of every allocated section of the original finds no
  `MOVW`/`MOVT` pair (opcode mask 0x0ff00000 with 0x03000000 / 0x03400000, same register
  within eight instructions) and no aligned 32-bit word (literal pools, `.data`, GOT,
  pointer tables) whose value lies in the interval.
- **No base+offset reference.** `g_lastBattery`, `g_lastVolumeTime` and `g_loggedWifi` are
  only loaded and stored as words at offset 0 (packets for 0x49cc0, 0x49f24, 0x4a19c), so
  none of them is a larger object reaching into the interval.
- **No initialization.** The unit's static-initialization function constructs its container
  globals and registers their destructors, but no reference from it (or from anywhere else)
  falls in the interval, so it is zero-initialized POD storage with no constructor or
  `atexit` entry.
- **Not an Onion patch.** Onion's `mov r0, r0` (0xe1a00000) patches, which GCC 8 never emits
  for this target, all lie in `createMenu`; none is in `tu_049c3c`, so no NOP'd code ever
  used the interval.
- **No symbol.** `.symtab` is stripped; the only dynamic symbols in BSS are `stderr` and the
  end markers.

## Which definitions can fill it

A GCC 8.2 `-O0` probe with the project toolchain shows that unreferenced zero-initialized
objects are **kept**, and `.bss` is ordered: public objects in definition order (including
ones defined later in the file, after functions), then file-local statics in declaration
order, then function-local statics. The interval, after the last public zero-initialized
object and before a file-local static, can therefore only hold:

- public zero-initialized POD objects defined anywhere in `tu_049c3c` after
  `g_lastVolumeTime`, and/or
- file-local statics declared before `g_loggedWifi`.

The unit has no function-local statics (the next unit's object follows `g_loggedWifi`).

## Second build: stock Miyoo MainUI

`miyoo354/app/MainUI` from the stock SD-card files (kept privately in `~/mainui-stock/`,
not in the repository): 1,443,156 bytes, SHA-256
`607dacd99abaf01753359ad4d171c9e9335504dd534bb93c5b29e530b16fd2bc`, same toolchain stamp,
same `.data` (0x1aac) and `.bss` (0x8b28 at 0x17ff30); `.text` and `.rodata` are laid out
differently. It is a different build of the same code.

Neighbours were paired by reference pattern and by code: `pollVolume_pushEvent` (Onion
0x49f24, stock 0x4a03c) matches 80/80 words and `wifiStatusCallback` (0x4a19c / 0x4a2b4)
88/88 words with branch targets and `MOVW`/`MOVT` immediates masked.

| Object | Onion | Stock |
| --- | --- | --- |
| `g_romListPositions` | 0x181b00 (10 refs) | 0x181b08 (10 refs) |
| `g_lastBattery` | 0x181b10 (2) | 0x181b14 (2) |
| `g_lastVolumeTime` | 0x181b14 (3) | 0x181b18 (3) |
| **unused interval** | **0x181b18–0x181b78** | **0x181b1c–0x181b7c** |
| `g_loggedWifi` | 0x181b78 (2) | 0x181b7c (2) |
| `g_testResults` | 0x181b7c (20) | 0x181b80 (20) |

The stock interval is again exactly 96 bytes, and the same scan finds **no reference** into
it. The storage is therefore not something the Onion build stopped using: it is a definition
in Miyoo's main-program source that neither build uses. (In the stock build `g_running` is
not between `g_romListPositions` and `g_lastBattery`, so the stock source differs slightly;
this does not affect the interval.) The June 28 firmware image (`miyoo354_fw.img`) may hold a
third build; it was not examined, since the SD-card copy is the one that runs.

## Effect on the BSS deficit

Every rebuilt object from `g_loggedWifi` through cJSON's BSS (`g_testResults`,
`g_testBlack`, cJSON's `global_error` and version buffer) sits exactly **100 bytes** before its
original address: the 4-byte device-ID interval from [step 1](bss-step1.md) plus these 96.
SQLite's BSS objects sit at **104**: SQLite's `.bss` is 8-byte aligned, and a 100-byte shift is
not a multiple of 8, so the extra 4 is alignment that follows from the earlier gaps rather than
a missing object. The remaining 56 bytes are unreferenced slots in the C units; see
[step 3](bss-step3.md), which accounts for the whole 160-byte deficit (4 + 96 + 4 + 4 + 4 + 48).

## Decision

The user chose to record these findings without changing source: no placeholder array was
added, so BSS remains 160 bytes short. If a placeholder is wanted later, the evidence bounds
it to 96 bytes of public zero storage after `g_lastVolumeTime` (or file-local statics before
`g_loggedWifi`) in `tu_049c3c`; its name and partition would remain unknown and should be
documented as such. Step 3 has not begun.

## Reproduce

The reference scanner and GCC probe used here live in ignored `build/bss-step2/`
(`refscan.py <elf> <lo_hex> <hi_hex> [ghidra/names.tsv]`, `probe/p.cpp`); they are optional
aids. The stock binary is private and must be supplied separately.
