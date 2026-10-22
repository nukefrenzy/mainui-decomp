# Link-readiness audit

2026-10-21, after `0c73395`, declaration/data recovery and the embedded-library/SDK pass.
All **22 proven launcher units**, verified cJSON and verified SQLite now link into an experimental
ARM executable: `build/link-audit/MainUI.audit.elf`. Its **286 import names and per-symbol version
requirements**, **23 DT_NEEDED entries in order**, interpreter and entry point agree with the
original. This establishes linking and import agreement; final section layout and device behavior
remain unproven. Do not install this audit artifact as MainUI.

Run from the repository root:

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
scripts/fetch-mainui.sh
python3 tools/linkaudit.py --require-link
```

The audit builds the 22 units with their manifest flags/dependency union, rebuilds and strictly
compares both embedded libraries, fetches the pinned target SDK libraries, performs `ld -r`,
then links an executable using the original library order. It does not execute ARM code.
Python 3.9+ and pyelftools, curl and tar are required. Sources and binaries stay in ignored build/.

- `scripts/build-libs.sh` retains SQLite 3.30.1 (C) and cJSON 1.7.10 (C++) sources and objects
  in `build/embedded-libs/`. Source files/package are SHA-256 checked before every build;
  `strictcmp.py` verifies 808,104 and 23,996 text bytes respectively, with only relocated words
  masked. The compatibility command `scripts/verify-libs.sh` invokes the same retained build.
- `scripts/fetch-target-libs.sh` obtains `support/my283.tar.xz` from
  [miyoomini-toolchain-buildroot commit 30e0e41](https://github.com/shauninman/miyoomini-toolchain-buildroot/tree/30e0e41f859f81b7ec390be34006ab1ba5f12f80/support).
  Archive SHA-256: `67c2dcc7aac183c1e3dd893dbb3c2f138ef59ae8fc738b7a266c98e67f32c038`.
  This SDK is bundled by the toolchain used by
  [Onion's setup](https://github.com/OnionUI/dev-miyoomini-toolchain/blob/main/support/setup-toolchain.sh).
  The fetcher creates SONAME links for 17 ARM EABI5 hard-float libraries under `build/target-libs/lib/`.
  Miyoo/MI libraries omit SONAME and use their filenames. It chooses zlib 1.2.8, as in Onion's
  unversioned `libz.so.1`; libc/libstdc++/libgcc_s/libm/libpthread/libdl come from pinned Arm GNU.
  The archive provides link-compatible symbols and versions, but is not proof of the device's
  exact library binaries. Its SDL, RSA and shared-memory libraries differ from the pinned Onion
  tree; freetype, gamename, mad and zlib agree by Git blob hash. No vendor files are committed.
- Embedded objects are inserted at their original addresses in input order: four Miyoo objects,
  cJSON, SQLite, then the remaining Miyoo objects. Arithmetic helpers are selected from static
  `libgcc` before `libgcc_s`, retaining the original dynamic exception-unwinding imports.

Generated files under `build/link-audit/`:

- `report.md`, `audit.json`, `unresolved.tsv`: checks, library hashes/paths and symbol evidence.
- `commands.txt`: reproducible driver/compiler/linker commands; `embedded-libs.log`: strict proofs.
- `launcher.o` / `launcher.map`: combined relocatable object and map.
- `MainUI.audit.elf`, `final-link.map`, `final-link.log`: experimental executable and diagnostics.
- `partial-link.log` and one compile log per Miyoo unit.

Normal exit 0 means compilation, partial link, internal calls, recovered storage and (if linking
succeeds) dynamic import/version/order checks passed. `--require-link` additionally fails when
executable linking fails; CI uses it. `--require-ready` also requires final layout proof and still
fails deliberately. `executable_ready` remains false. Setup/compilation errors return 2.
Known internal/SQLite declaration mismatches, changed final imports/versions/library order or
interpreter, wrong resolved calls and storage mismatches return 1. Failed links remove the ELF.

## Current inventory

With embedded objects included, `ld -r` has **292 undefined names**: 275 original dynamic imports,
15 static-runtime entry points (ten arithmetic helpers and stat/lstat variants), and two linker/CRT
names. These are all resolved by final linking. The count grew from the earlier Miyoo-only 283
because SQLite introduces additional runtime references, while all 35 embedded API names are now
provided. Undefined names in a relocatable object do not measure final executable readiness.
There are no duplicate strong-definition errors.

The original's complete 286-name import set includes COPY-relocated data, which the comparison
handles explicitly. The rebuilt executable has zero added/missing imports and zero per-symbol
version mismatches. Version requirements remain GLIBC_2.4/2.7/2.28 and GCC_3.5 in the same provider
libraries. Symbols with no original version requirement remain unversioned.

Two additional source corrections were required, each retaining full unit/function MATCH:

- `openThemeFont`'s configured filename is `std::string&`, selecting the original non-const
  `at(unsigned int)` import at PLT 0x14e48. Its internal mangled name/manifest changed accordingly.
  The root argument stays const. No shared header changed in this slice.
- `zipOpen` calls `lstat`, not `stat`: original call 0x14d8dc reaches wrapper 0x1518a0, which
  branches to `__lxstat`. The original accepts symlinks explicitly. This restores the missing import
  and preserves link semantics that the masked code comparison alone could not distinguish.

## Resolved declaration mismatches with original call evidence

All **27** internal-call symbol mismatches and **13** mangled SQLite references are resolved.
The audit decodes original body calls and compares their destinations with definitions in all
22 units and both embedded objects. All **3,095** resolved body-call destinations agree with
the original. Static-runtime targets and template/vtable references still need final-layout checks. The following former references had direct address evidence:

| Former reference | Definition at the original destination |
| --- | --- |
| `currentTicks()` | `milliseconds()` |
| `deleteWindows(vector<TrimusWindow*>*)` | `clearWindows(vector<TrimusWindow*>*)` |
| `waitBackgroundThread()` | `stopBackgroundThread()` |
| `fillBackground(SDL_Surface*)` | `drawBackground(SDL_Surface*)` |
| `drawTitle(..., bool)` / `drawTitle(..., int)` | `drawTitleBar(..., bool)` |
| `saveSystemJson(void*, const char*, int)` | C-linkage `saveSystemJson` |
| C-linkage `findProcess` | C++ `findProcess(const char*, int)` |
| `handleGlobalInput(int)` | `handleGlobalInput(SDL_Event*)` |
| `NotifyWindow(..., int)` | `NotifyWindow(..., void (*)())` |
| `TextMenu(int, int, int, const char*)` | `TextMenu(bool, bool, int, const char*)` |
| `TextMenu::addItem(TestMenuItem*)` | `TextMenu::addItem(void*)` |
| Inconsistent `scalePixels` dimension types | Shared unsigned size/stride API with signed arithmetic inside |
| `setSwapMode(int)` | `ensureSwap(void*)` |
| `checkDevice()` / `probe_50504` | `waitForProbe` / `authenticateDevice` |

The full inventory also records drawImeText/drawImeInput, drawIconLabel/drawFooterText,
drawWindowFrame, getLanguageFile, saveLaunchCommand, stopSupplicant, wifiConnectionState and
the inflaterOpen/inflaterRead references. Use each row's exact mangled destination(s), source file
and call address. Constructor aliases can legitimately have two names at one destination.
The fixes use source declarations and calls, with no linker aliases or extra wrapper functions.
Every existing unit still matches.

Shared declarations now live in:

- `PlatformApi.h`: C-linkage process/configuration/device calls. `findProcess` has one C entry
  point shared by its C++ definition and C monitor caller; authentication references use the
  actual byte-returning signatures. Existing C definitions supply getLanguageFile/saveSystemJson.
- `SQLiteApi.h`: all 13 SQLite calls used by p05/p06 and the MultiDB unit, with C linkage.
  MultiDB already had C linkage; the mixed linkage came from the first unit's p05/p06 declarations.
- `TextMenu.h`: the actual class shared by the first and hardware units. The hardware-only
  integer constructor, typed addItem argument and invented vfunc3/vfunc11 overrides are removed.
  Those methods are inherited from TextMenuDrawer, as in the original vtables. Test rows are
  explicitly cast from the shared void-pointer vector.
- `ZlibDecoder.h`: the full decoder layout is C-compatible. openZlib/readZlib have C linkage;
  the C zip reader uses these names and the real history field, replacing its partial Inflater
  struct and inflaterOpen/inflaterRead placeholders.
- `PixelScale.h`: a single unsigned-dimension/stride signature for callers and definition.
  Explicit signed casts inside the scaler preserve the original divisions and byte sequence.
  Simply changing the caller declarations to signed removed three conversion moves at each
  call site and shortened the first unit by 24 bytes. Both the final caller unit and the
  376-byte scaler definition match with the shared signature.

Rendering references use the actual drawText/drawTextLines/drawTitleBar/drawScreenFrame/
drawIconLabel names and parameter types. NotifyWindow's callback is a function pointer, and
handleGlobalInput takes SDL_Event*. The key-mapping window calls the existing inline getTitle
method; its obsolete windowTitle placeholder is gone. Main's loadRomListPositions declaration
also uses its actual integer return type.

All 13 formerly mangled SQLite references now refer to embedded C symbols. Seven of those names
were already referenced elsewhere; six are new names in the unresolved embedded-library group.
This explains why resolving 13 references reduces the unresolved-name total by seven. The cJSON
declarations continue to use C linkage.

## Data and initialization

All **54 formerly unresolved data names** are resolved: 13 duplicate names were reconciled, and
41 storage definitions were recovered. `src/data.tsv` records the original addresses, verified
extents and check kinds. The audit checks **8,421 bytes** across 53 storage extents: 41 newly
defined objects and 12 existing canonical alias providers. These include a 624-byte
English pointer table, and verifies all **156 pointed-to strings**. There are no storage mismatches.

Original MOVW/MOVT evidence identified the following data-name pairs. Twelve had an existing
definition; the two launch-command names now share one recovered 2,048-byte buffer:

| Original address | Names |
| --- | --- |
| 0x17ff54 | g_launchCmd / g_launchCommand |
| 0x182180 | g_chargingFont / g_iconFont |
| 0x1821b4 | g_emptyListImage / g_emptyImage |
| 0x1821c0 / 0x1821c4 | g_emuIconX / g_gameFocused; g_emuIconY / g_gameNormal |
| 0x1821d0 | g_rappIconX / g_raRow |
| 0x1821fc | g_romPreviewBackground / g_previewBg |
| 0x182214 | g_keyStateIcons / g_volumeIcons |
| 0x182278 | g_shadowColor / g_clearColor |
| 0x18228c | g_listNumberColor / g_highlightColor |
| 0x1822a0 | g_progressColor / g_selectedTextColor |
| 0x1822a8 / 0x1822ac | g_pathColor / g_hintColor; g_countColor / g_listColor |

The aliases were replaced with the canonical source names, without linker aliases or duplicate
storage. Types were checked against consumers. IconMenuItem's +8/+12 fields and constructor
arguments are borrowed SDL_Surface pointers (backgrounds), rather than integer coordinates.
The shadow/clear color is one SDL_Color; the theme loader's aggregate zero assignment retains
its original code. Other renamed fonts, colors, surfaces and icon arrays already agree in type.

Recovered definitions cover the original zero-initialized buffers/flags, framebuffer state,
keyboard map, DEFLATE tables, theme constants, language table, and authentication storage.
Nonzero values come from the original ELF. The audit compares ordinary bytes without masks;
BSS/COMMON must match original zero storage. For the language table, every ABS32 relocation is
followed to its string and compared with the original entry. Corrupt bytes or strings fail the
audit, as do missing definitions, unexpected relocations or wrong symbol sizes.

Definitions of the three read-only theme colors live outside their consuming TU. Defining them
in tu_129f9c.cpp makes GCC fold their values even at -O0, removing 36 body bytes. Separate
storage preserves the original loads and all unit boundaries. The original color bytes are
68 68 68 00, f0 f0 f0 00 and aa aa aa 00.

**Extent limits:** g_deviceId has five verified bytes read by getSerial; the original complete
allocation size remains unknown. g_panelInfo is the verified 116-byte prefix read by adjustColor;
its full SDK allocation/layout remains unknown. Both limits are explicit in source and the data
manifest. No unverified tail bytes were invented. Other extents follow the existing declarations
and access ranges. Initial contents matching does not establish final section placement, padding,
all other pre-existing global initializers or complete coverage of original .data/.rodata/.bss.
See [data recovery](data-recovery.md) for the storage ledger and validation boundaries.

## Remaining runtime and layout work

The prior five runtime/theme names are now accounted for by the overload correction, static
arithmetic helpers and final libc wrappers. Adding embedded objects also exposed `lstat`'s wrapper
mismatch, corrected above. Runtime arithmetic call targets still need checking against the static
archive members and original runtime addresses; their code is outside the 22 rewritten units.

The original requests `/lib/ld-linux-armhf.so.3` and **23 DT_NEEDED libraries**; their exact names
are captured in `audit.json`. These include SDL/TTF/image/mixer, libshmvar, libgamename, librsautil,
SigmaStar MI libraries and the standard runtime. Matching a symbol name alone does not establish
that the SDK, library version or ABI is correct.

The original entry is **0x15a30**, before the first rewritten unit at 0x15b20. Recover startup,
embedded-library object order, COMDAT selection, exception tables, init-array order and target
link settings. Important original sections are:

| Section | Address | Bytes |
| --- | --- | --- |
| .text | 0x15a30 | 1,294,000 |
| .rodata | 0x1518e8 | 78,184 |
| .ARM.extab / .ARM.exidx | 0x164a50 / 0x169f94 | 21,828 / 12,936 |
| .init_array | 0x17ddc0 | 28 |
| .got | 0x17e000 | 1,152 |
| .data | 0x17e480 | 6,828 |
| .bss | 0x17ff30 | 35,624 |

## Remaining linked-layout differences and next slice

The entry remains **0x15a30**, and all TU body anchors through **tu_11cdc8 at 0x11cdc8** are at
original addresses. The first displaced TU body is **tu_129f9c: 0x129f9c → 0x129d70 (-556 bytes)**.
That localizes the first boundary deficit to tu_11cdc8's linked template/inline tail; per-unit
backtracking alone did not establish actual cross-object COMDAT selection. Investigate that tail's
kept/discarded sections and shared declarations before adding any padding. No padding workaround
or forced duplicate definitions were added.

| Section | Original bytes | Rebuilt bytes | Difference |
| --- | ---: | ---: | ---: |
| .text | 1,294,000 | 1,293,448 | -552 |
| .rodata | 78,184 | 77,636 | -548 |
| .ARM.extab | 21,828 | 21,784 | -44 |
| .ARM.exidx | 12,936 | 12,904 | -32 |
| .init_array | 28 | 28 | 0 |
| .got | 1,152 | 1,152 | 0 |
| .data | 6,828 | 6,220 | -608 |
| .bss | 35,624 | 35,464 | -160 |

Equal sizes do not establish equal initializers, addresses or order. After COMDAT layout, recover
remaining data/string initializers and allocation extents, compare static runtime/startup code,
exception/init-array order, and verify absolute relocations. Device testing remains outstanding.
No runnable replacement is claimed or deployed.

Validation for the audit tooling:

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
python3 -m unittest discover -s tools/tests -v
scripts/run_units.sh
```

The ARM fixtures also build a versioned shared library and executable with COPY data imports,
then exercise added/missing imports, wrong versions, library order and interpreter detection.
No ARM code is executed. The relocation fixture assembles and links real MOVW/MOVT and call relocations, checking base-address
recovery for positive offsets, a carry across a 64 KiB boundary and a negative offset. It also
rejects a rewritten call whose symbol resolves to a different original function. No ARM
code is executed. All 22 unit checks and 689 registered individual matches pass after the
declaration, storage and import fixes. Matching tools, relocation/call masks and required unit ends are unchanged.
