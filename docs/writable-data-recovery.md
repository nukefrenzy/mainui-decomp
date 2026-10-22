# Writable data and unwind recovery

2026-10-22, following [read-only-data recovery](rodata-recovery.md).
The linked `.data`, `.ARM.extab` and `.ARM.exidx` now match every original byte,
including relocated pointers. `linkaudit.py --require-link` enforces each section's
address, size and contents without masks.

| Section | Range (exclusive end) | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| .data | 0x17e480–0x17ff2c | 6,828 | `de88cc81476af77c889e7ffd876f6d165a6f2ab0050b19f7f6ab2d5c697254d5` |
| .ARM.extab | 0x164a50–0x169f94 | 21,828 | `c9cd2819ffea61695c1ffa3b5dd9f7839ad8b528b822f2ab1bafc77d27336d97` |
| .ARM.exidx | 0x169f94–0x16d21c | 12,936 | `26eb82e5e315f9a0e55660859bac9cffb78f2f561debc0003d36b1a87dd4ed8b` |

## Panel objects and initialized globals

The old 116-byte panel prefix omitted three independently bounded objects. The
pinned SDK's `mi_panel_datatype.h` supplies compatible field layouts; the original
ELF supplies the initializers and object boundaries:

- `g_panelInfo`, 0x17e48c: full 188-byte panel parameter object. Its existing timing
  prefix stays unchanged; trailing PWM, format, clock-set and channel fields are restored.
- `g_panelCommands`, 0x17e548: 341-byte command/length/payload stream. `FE FE` introduces
  a delay byte and `FF FF` terminates the stream. Commands are written as readable rows.
- `g_panelMipiConfig`, 0x17e6a0: 76-byte MIPI configuration, including a real relocation
  to `g_panelCommands` and command size 341. Compiler alignment places it after the stream.
- `g_customDisplayTiming`, 0x17e6ec, follows these objects instead of preceding the panel.

These restore the missing initialized extent without linker padding or extracted
section blobs. The firmware's Wi-Fi status globals and diagnostic gray color also
needed their original definition order. The entire `.data` comparison checks the
MIPI pointer and all string-table pointers, beyond the manifest's initializer checks.
The manifest now covers **59 extents / 9,102 bytes / 185 strings**, including the full
panel object and command stream. It does not separately list the pointer-bearing
MIPI structure; the raw section proof covers all of it.

Known BSS ordering corrections place diagnostic results before black, theme volume
resources before colors, and the fixed theme color before text color. Configuration
I/O buffers use external definitions so GCC emits them before the later recent-list
object; local `.comm` allocation had deferred them until after public objects.

## Wi-Fi unwind section order

The remaining exception-table differences were confined to the Wi-Fi unit. Its
shared item constructor must precede the body unwind section, while menu/action
inline records must follow it. Moving individual inline definitions changed COMDAT
selection in other consumers, so that approach is not retained.

`WifiMenuItems.h` now holds the initial shared drawable declarations and definitions.
`WifiMenus.h` includes it and retains the remaining menu/action classes. Other consumers
see the same complete definitions in the same order. In `tu_138c50.cpp`, `wifiMode`
appears between those includes: this establishes the body unwind section before
menu/action inline definitions, preserving code, vtables and all shared declarations.
No function bodies, object layouts, virtual slots or exception behavior were changed
for this unwind correction. No hand-authored exception bytes or linker overrides are used.

## Validation and remaining BSS work

The full link audit passes: `.text` retains the established 1,294,000-byte masked
proof and zero raw ARM call/patch differences; `.rodata` retains its 78,184-byte raw
proof; imports, storage checks and 3,095 body-call destinations remain correct.
All **689/689 registered matches**, **22/22 whole units** and **6/6 ARM tooling tests** pass.
The tooling fixtures also reject changed writable bytes, pointers, section addresses
and sizes through the shared raw-section comparator.

BSS remains **35,464 bytes versus 35,624**, a 160-byte deficit. Its base now agrees
at 0x17ff30 because `.data` is restored. BSS byte equality alone would only prove zero
initialization; symbol placement and allocation extents must be established too.
Specific evidence to investigate next:

- `g_deviceId` still has only its five consumed bytes proven. `g_launchCmd` starts four
  bytes too early; increasing the ID array without evidence would guess its allocation.
- The original iostream initializer is at 0x180858; it is currently deferred to the
  end of the first unit's BSS. `g_fbVar` and later fields consequently have wrong addresses.
- Original 0x181b18–0x181b78 is a 96-byte unreferenced interval between the last volume
  timestamp and Wi-Fi logging state. Its object partition/type is not established.
- C helper buffers, monitor state and authentication COMMON objects still need their
  original order/extents. The input/key/monitor gaps require further evidence; no
  guessed reserve arrays were added to force a matching BSS size.

Remaining GOT and code relocation values also need comparison after BSS is settled.
No device execution, deployment, commit, push or merge occurred in this pass.
