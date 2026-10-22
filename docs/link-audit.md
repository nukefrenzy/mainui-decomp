# Link-readiness audit

2026-10-22, after `0e93978`, ARM call/patch reconciliation and read-only-data recovery.
All **22 proven launcher units**, verified cJSON and verified SQLite now link into an experimental
ARM executable: `build/link-audit/MainUI.audit.elf`. Its **286 import names and per-symbol version
requirements**, **23 DT_NEEDED entries in order**, interpreter and entry point agree with the
original. Actual linked **.text 0x15a30–0x1518e0 (1,294,000 bytes)** now MATCHes with the existing
relocation/call masks, and all 22 TU body anchors are at original addresses. This establishes
code layout and import agreement. Raw ARM call/patch words now also agree at every mapped ARM
instruction site. The entire linked **.rodata 0x1518e8–0x164a50 (78,184 bytes)** is now byte-identical, including
vtable/typeinfo pointers and padding, without masks. All 6,828 `.data` bytes and both ARM
exception sections also match without masks. BSS layout, remaining relocation values and
device behavior remain unproven. Do not install this audit artifact as MainUI.

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
  `--emit-relocs` retains static relocation sections for the actual linked-text comparison; these
  are non-allocated audit metadata and do not change loadable code/data.
- `audit.json` also records `linked_text`: original address/size, masked text result, raw call/patch
  differences and all body placements. The masked comparison observes actual cross-object COMDAT
  retention rather than accepting a plausible standalone unit walk.
- `partial-link.log` and one compile log per Miyoo unit.

Normal exit 0 means compilation, partial link, internal calls, recovered storage and (if linking
succeeds) dynamic import/version/order checks passed. `--require-link` additionally fails when
executable linking fails; CI uses it. `--require-ready` also requires final layout proof and still
fails deliberately. `executable_ready` remains false. Setup/compilation errors return 2.
Known internal/SQLite declaration mismatches, changed final imports/versions/library order or
interpreter, wrong resolved body calls, storage mismatches, a failing linked-text proof or any
raw ARM call/patch difference or any byte/address/size difference in .rodata, .data, .ARM.extab
or .ARM.exidx return 1. Failed links remove the ELF.

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
  Changing the caller declarations to signed removed three conversion moves at each call site
  and shortened the first unit by 24 bytes. The shared API stays unsigned; the implementation
  now also uses unsigned loop counters/division, matching __aeabi_uidiv at 0x11ce1c/0x11ce38.
  A signed row-stride cast is retained for the original index sequence. Both the caller unit
  and 376-byte scaler definition match; no shared-header change was needed for this correction.

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
extents and check kinds. The audit checks **9,102 bytes** across 59 storage extents: 47 recovered
definitions and 12 existing canonical alias providers. These include a 624-byte English pointer
table and a 116-byte legacy suffix table; all **185 pointed-to strings** are verified. There are no storage mismatches.

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

The three read-only theme colors now live in tu_129f9c.cpp as SDL_Color structs, with their
original order and palette neighbors. The prior ThemeColor union definitions folded values at
-O0 when placed in that TU; copying the SDL_Color struct instead retains all original loads.
Their bytes remain 68 68 68 00, f0 f0 f0 00 and aa aa aa 00. The full read-only-data proof now
checks their final placement too.

**Extent limit:** g_deviceId has five verified bytes read by getSerial; its original complete
allocation remains unknown. The full 188-byte panel object, 341-byte command stream and
76-byte MIPI configuration are now recovered. The manifest checks panel and command bytes;
the raw .data proof includes every MIPI field and its command pointer. Named initializer
checks do not establish BSS object sizes/placement. See [data recovery](data-recovery.md)
and [writable-data recovery](writable-data-recovery.md).

## Remaining runtime and layout work

The prior five runtime/theme names are now accounted for by the overload correction, static
arithmetic helpers and final libc wrappers. Adding embedded objects also exposed `lstat`'s wrapper
mismatch, corrected above. The subsequent arithmetic signedness fixes restore the original
static helper destinations; the full linked-text proof also covers the static runtime code.
Absolute data references and runtime behavior still require validation.

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

## Corrected COMDAT identity and linked text proof

The first layout deficit is resolved. The independent global at **0x182170**, still named
`g_recentList` in the rewrite, is **std::list<int>**, not std::list<RecentItem>. Its initializer at
0x123ff4 calls the new list constructor at 0x124118; atexit at 0x124010 registers its destructor
at 0x129f70. Recent games remain the distinct `std::list<RecentItem>` from tu_049c3c.
Only this local definition and its comment changed; no shared header or manifest symbol changed.

The wrong element type gave its generated constructors/destructor the same COMDAT identities as
the earlier recent-games list. The real linker discarded them. The standalone unit walk could
assign those byte-identical sections to later addresses and find an apparently valid end despite
that identity conflict. Correcting the global restores **556 tail bytes** and places tu_129f9c
at **0x129f9c**. Final runtime alignment removes the former four-byte discrepancy, making the
whole .text size exact. No padding, linker aliases or forced duplicate definitions were added.

The audit now compares the **actual linked .text**, using emitted static relocation sites and
the existing ARM BL/BLX call masks. The entire **0x15a30–0x1518e0** range MATCHes: 1,294,000 bytes,
286,793 unmasked words compared, zero differing unmasked words. All 22 unit body anchors are at
their original addresses. This checks the code produced by actual COMDAT selection, including
embedded libraries and static runtime. The masked proof itself does not establish absolute
relocation/call values; the separate raw ARM call/patch comparison now also passes.
All 53 storage checks, 156 strings, 3,095 resolved body calls and final import/version/order checks
continue to pass.

## Production executable and --require-ready

After the audit link, the same command is run without `--emit-relocs` or a map and the result is
stripped with `strip --strip-all` into `build/link-audit/MainUI`. Its SHA-256 is compared with the
original; `executable_ready` is true only when they are equal and every other check passes, and
`--require-ready` fails otherwise. It now passes: the production executable is byte-identical. The
link places static `-lgcc` immediately before `libgcc_s.so.1` (as the gcc driver does), which
reproduces `.dynstr`; `.dynstr` and `.dynamic` are enforced raw. `tu_14f730` is compiled with GCC
7.3.0 (`deps` `gcc73`), which supplies the original's second `.comment` string. See
[byte-identical-build.md](byte-identical-build.md).

## Unmasked code addresses in relocated words

The masked proof ignores every relocated word, so a jump table or function-pointer literal could
point same-shaped bodies at each other unnoticed. `check_code_addresses` resolves each relocated
`.text` word from the linked bytes (`R_ARM_ABS32`, including section-relative jump-table entries,
and `MOVW`/`MOVT` pairs); when the target is in an executable section, the raw words must equal
the original, and any difference fails the audit. **2,819 code-targeting words** now match.
Other relocated words that still differ are reported by target (currently `.bss` 1,768 and
`.got` 14, both waiting on BSS/GOT layout), without failing.

The check found one real error: `ImeWindow::initKey`'s 45-case jump table at **0x140db0**. Every
case set the right strings, but cases 39, 43 and 44 (labels `""`, which the `.rodata` pool cannot
place) were emitted last instead of right after cases 0 and 1, permuting 43 table words.
`src/tu_13edfc.cpp` now lists them in the original order. Run against the old source, the audit
fails with those 43 words. The tooling fixture `test_permuted_jump_table` reproduces the gap:
swapped body layout passes the masked proof and fails this check.

## Resolved ARM call/patch differences

All **14 raw ARM control-flow differences** are resolved. The audit uses ARM mapping symbols to
exclude literal pools and records exact words/targets in `linked_text.control_flow_differences`;
that list is now empty and any future entry fails the audit. The ordinary relocation/call masks
remain unchanged.

| Sites | Correction |
| --- | --- |
| 0x15fd4 | Preserve the shadow-blit NOP in drawText |
| 0x168b4, 0x168cc, 0x16918, 0x16a2c, 0x16a44, 0x16a90 | Preserve background blit/fill NOPs in drawWifiIndicator |
| 0x14aea0 | Preserve the disabled PWM-period write in initBacklightPwm |
| 0x2b748 | Panel frame-rate arithmetic uses promoted signed operands and __aeabi_idiv at 0x150ce4 |
| 0x11ce1c, 0x11ce38 | Scaler uses unsigned counters/division and __aeabi_uidiv at 0x150ad8 |
| 0x12d09c, 0x12d81c | insertFileItem/freeItems use vector<void*>::at at 0x3b310, not operator[] |
| 0x132c10 | Cache put/get use push_front; its inline insertion wrapper calls begin at 0x1329e8 |

The cache now inserts new entries and moves hits to the front, while eviction remains at the
back. This agrees with the original wrapper's call to begin rather than end. The browser's
bounds-checked access also restores exception metadata omitted by operator[].

The eight patched calls use the existing scoped GAS `bl` macro technique: retain GCC argument
setup and cleanup, emit literal **0xe320f000**, omit each call relocation, and assert the expected
patch count (1 shadow, 6 Wi-Fi, 1 PWM). The macro is removed after each function. The existing
12 menu-factory patches remain unchanged. No extra masking, linker aliases, wrappers or padding
were introduced. No shared headers or manifest signatures changed.

## Exact read-only data and remaining work

**All 78,184 bytes of linked .rodata now match exactly at 0x1518e8–0x164a50.** No relocation,
pointer or instruction words are masked. SHA-256 of both original and rebuilt section:
`21ccb7647a7a9b28a7b2f74a29a236696020b25471536f23d28aee123638115d`.
`check_rodata` makes any address, size or byte difference fail the audit. Its fixture deliberately
corrupts a relocated vtable pointer to verify that pointers are compared too.

Read [rodata-recovery.md](rodata-recovery.md) for literal identity/extents, declaration and source
order, assertion diagnostics, and the two corrected WifiMenuDrawer bindings (slot 7 is its own
renderer; slot 15 is pure virtual in the base). There are no new fields or virtual slots, linker
aliases, arbitrary padding or changed text masks. Read-only constants are readable source data;
the build does not extract replacement section bytes from MainUI.

The 29-entry legacy suffix table and full panel/MIPI objects restore the entire writable
initialized section: **6,828 raw bytes at 0x17e480–0x17ff2c**. A Wi-Fi header split restores
both exception sections without changing code, vtables or other header consumers. All three
sections are now enforced by the same raw-section comparator. Read
[writable-data-recovery.md](writable-data-recovery.md) for source evidence, section hashes,
shared-header changes and remaining BSS gaps. The manifest independently checks
**59 extents / 9,102 bytes / 185 strings**. Next: BSS and remaining relocation values.

| Section | Original bytes | Rebuilt bytes | Current result |
| --- | ---: | ---: | --- |
| .text | 1,294,000 | 1,294,000 | MATCH with existing relocation/call masks; zero raw ARM call/patch differences |
| .rodata | 78,184 | 78,184 | Raw byte-identical, including pointers |
| .ARM.extab | 21,828 | 21,828 | Raw byte-identical, including pointers |
| .ARM.exidx | 12,936 | 12,936 | Raw byte-identical, including pointers |
| .init_array / .fini_array | 28 / 4 | 28 / 4 | Raw bytes and addresses agree |
| .data.rel.ro | 128 | 128 | Raw bytes and address agree |
| .got | 1,152 | 1,152 | Size/address exact; 20 raw bytes differ |
| .data | 6,828 | 6,828 | Raw byte-identical, including pointers |
| .bss | 35,624 | 35,464 | 160 bytes short; allocation limits remain |

The incidental matches for init/fini arrays and .data.rel.ro are recorded observations; the
enforced whole-section proofs cover .rodata, .data, .ARM.extab and .ARM.exidx. Matching initial placeholders does
not validate data copied by the dynamic loader or device-library compatibility. No device test,
deployment, push or merge occurred.

Validation for the audit tooling:

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
python3 -m unittest discover -s tools/tests -v
scripts/run_units.sh
```

The ARM fixtures also reject changed code and a missing COMDAT whose instructions are identical
to an earlier function, while preserving the existing data-address/call masks. Changed raw calls and re-enabled original NOPs
remain visible despite the ordinary call mask; either now fails the audit. They also build a versioned shared library and executable with COPY data imports,
then exercise added/missing imports, wrong versions, library order and interpreter detection.
No ARM code is executed. The relocation fixture assembles and links real MOVW/MOVT and call relocations, checking base-address
recovery for positive offsets, a carry across a 64 KiB boundary and a negative offset. It also
rejects a rewritten call whose symbol resolves to a different original function. No ARM
code is executed. All 22 unit checks and 689 registered individual matches pass after the
declaration, storage and import fixes. Matching tools, relocation/call masks and required unit ends are unchanged.

Validation after the COMDAT correction: all **22/22 whole units**, all **56/56 registered functions
in tu_11cdc8**, and **5/5 ARM audit tests** pass. The other 633 registered functions were unchanged
and passed in the preceding 689/689 run. `linkaudit.py --require-link` passes with the new linked
text proof, unchanged imports and zero storage/body-call mismatches.

Validation after the call/patch corrections: all **22/22 whole units**, all **9/9 changed registered
functions**, and **5/5 ARM audit tests** pass. The linked .text proof, all body placements, all
3,095 resolved body calls, storage/string checks and import/version/order checks pass; raw ARM
call/patch differences are **0**. Unchanged registered functions retain the preceding checks.

Read-only-data validation: raw .rodata MATCH, unchanged full linked-text proof and zero raw ARM
call/patch differences; all 22 whole units, all 689 registered matches and six audit tests pass.
The full match run is recorded in build/rodata/matches-*.log. Shared-header changes are listed
in rodata-recovery.md.

Writable-data/unwind validation: all four enforced raw sections MATCH, with unchanged linked
text, raw calls, imports and storage checks. All 22 whole units, 689 registered matches and
six tooling tests pass after the header split.
See writable-data-recovery.md; BSS is still 160 bytes short and no device test occurred.
