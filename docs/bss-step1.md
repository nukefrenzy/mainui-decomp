# BSS step 1: device ID and iostream initializer

2026-10-22, after `e6e91d9`. This pass is limited to step 1; the 96-byte interval
and C helper/COMMON ordering await the user's confirmation for steps 2 and 3.

## Initializer placement correction

The original static-initialization function at 0x3919c constructs iostream's
`std::ios_base::Init` at **0x180858**, then constructs `g_pendingWindows` at
0x17ff3c. Its packet and disassembly establish both addresses independently of
masked function matching.

GCC emits the public zero definitions in `.bss`, but defers the local `.comm`
objects to the end of that input section. The first UI unit had defined its
framebuffer and subsequent private state as public objects. Consequently they
preceded the iostream initializer, which was incorrectly placed at 0x181910.

These twelve definitions are now TU-local: framebuffer state, settings-dirty,
power-repeat count, four input/status flags, working-directory buffer, busy flag,
two progress counters and charging state. They have no consumers in another TU.
The unused extern declarations were removed from `tu_015b20.h` (included only by
this TU), along with the redundant charging declaration in p11. The manifest uses
the resulting local mangled symbols. No fields,
function signatures, sizes, alignment attributes or masks changed.

The reconstructed order now puts the initializer immediately after the loading
thread and immediately before the framebuffer, with the same relative offsets
as the original. It does **not** yet prove exact absolute BSS placement:

| Object | Original address | Rebuilt after correction | Delta |
| --- | --- | --- | ---: |
| g_windowStack | 0x17ff38 | 0x17ff38 | 0 |
| g_pendingWindows | 0x17ff3c | 0x17ff3c | 0 |
| g_deviceId (five consumed bytes) | 0x17ff48 | 0x17ff48 | 0 |
| g_launchCmd | 0x17ff54 | 0x17ff50 | -4 |
| g_countBuffer | 0x180754 | 0x180750 | -4 |
| g_loadingThread | 0x180854 | 0x180850 | -4 |
| iostream initializer | 0x180858 | 0x180854 | -4 |
| g_fbVar | 0x18085c | 0x180858 | -4 |
| g_settingsDirty | 0x1808fc | 0x1808f8 | -4 |
| g_busy | 0x181908 | 0x181904 | -4 |
| g_charging | 0x181914 | 0x181910 | -4 |
| next TU's g_statusThreadRun | 0x181918 | 0x181914 | -4 |

## Device-ID extent remains unresolved

The per-function packet for `getSerial` at 0x1237a8 shows five byte loads from
0x17ff48, at offsets 3, 2, 1, 0 and 4. A direct-address scan of allocated ELF
sections finds those five MOVW/MOVT pairs and no literal pointers into
0x17ff48–0x17ff54. The matching source and original initialization function give
no write or constructor that establishes a larger ID object.

The next known original object starts at 0x17ff54, bounding a twelve-byte physical
interval. This is not proof of a twelve-byte array: for example, an ID array of
nine through twelve bytes, or a smaller array followed by a separate unused word,
could give the same next address. A five-byte array plus natural alignment currently
puts the next object at 0x17ff50; the additional four bytes are unexplained.

The source and manifest deliberately retain **five proven bytes**. No reserve
array, larger guessed ID allocation or linker padding was added. Settling the
original object's full extent requires additional evidence; step 1 is therefore
only partially resolved. BSS remains **160 bytes short**, unchanged by the order fix.

## Verification

The complete executable link audit passes: masked linked `.text`, zero raw ARM
call/patch differences, exact `.rodata`, `.data`, `.ARM.extab` and `.ARM.exidx`,
all import checks, 59 storage extents / 9,102 bytes / 185 strings, and 3,095 resolved
body calls. All **22 whole units** and **310/310 affected registered functions**
pass. The function checks are recorded under ignored `build/bss-step1/`.

This work is included in the BSS step 1 commit accompanying `docs/handoff.md`.
Neither step 2 nor step 3 has begun; each next step requires user confirmation.
