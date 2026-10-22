# Status and next steps

Snapshot of 2026-10-22. Live numbers: `python3 tools/progress.py`; unit ranges: `docs/tu-map.tsv`;
method: `docs/unit-workflow.md`. Update this file when a unit is proven or the plan changes.

## Where things stand

**2026-10-22: byte-identical build.** `scripts/build-mainui.sh` builds MainUI from `src/` and it is
byte-identical to `MainUI-354-clean` (SHA-256 `98c85f6c…0c94`); CI requires it. See
[byte-identical-build.md](byte-identical-build.md). The notes below record how the project got
there.

**2026-10-22: device check.** On Brad's Miyoo Mini Plus SD card (Onion OS), the MainUI that
runs is `.tmp_update/bin/MainUI-354-clean`. Its SHA-256 is `98c85f6c…0c94`, the same as the built
binary, so the device already runs a file byte-identical to our build. Onion's
`runtime.sh` (`mount_main_ui`) bind-mounts `MainUI-$DEVICE_ID-<clean|expert>` over the 0-byte
`miyoo/app/MainUI`. The card has no `.tmp_update/config/.showExpert`, so it uses the clean build.
Nothing on the card was changed. A device test adds information only for a modified build.

- **Goal:** complete MainUI source that rebuilds byte-exact with Arm GNU 8.2-2018.08, `-O0 -fsigned-char`,
  worked file by file (one translation unit per `src/tu_<lo>.cpp` or `.c`).
- **Progress:** all **22 whole units** pass `tools/unitcheck.py`: **2,612 of 2,612 tracked
  Miyoo functions, 458,064 of 458,064 bytes (100.00%)**. The final unit, `tu_015b20`, matches
  0x15b20–0x49c3c: 145,184 body bytes and 798 inline/template sections (68,092 bytes).
  All 689 registered individual matches remain checked by `scripts/run_matches.sh`.
- **Embedded libraries:** SQLite 3.30.1 and cJSON 1.7.10 rebuild byte-exact (`scripts/verify-libs.sh`);
  tiny-utf8 4.4.x is header-only and appears in the template tails of the files that use it.

| Proven unit | Range | What it is |
| --- | --- | --- |
| tu_015b20 (C++) | 0x015b20–0x049c3c | Complete launcher UI: parts p01–p11, compiler init pair, 798 inline/template sections |
| tu_04cb68 (C++) | 0x04cb68–0x051b5c | Hardware diagnostics: 56 written functions, constructor/destructor variants, 27 inline tail functions |
| tu_049c3c (C++) | 0x049c3c–0x04c418 | Main program, status/hotplug/Wi-Fi callbacks: ten written functions, init pair, 93 inline/template functions |
| tu_04c418 (C++) | 0x04c418–0x04cb68 | Ten circular singly linked-list helpers, no tail |
| tu_11cdc8 (C++) | 0x11cdc8–0x129f9c | Shared configuration/device helpers: 56 written functions, compiler init pair, 263 inline/template functions |
| tu_129f9c (C++) | 0x129f9c–0x12ce28 | Seven theme/font/image/sound functions, no tail |
| tu_12ce28 (C++) | 0x12ce28–0x1302cc | File-browser directory cache and traversal: 15 body functions plus 105 inline/template functions |
| tu_1302cc (C++) | 0x1302cc–0x131e34 | Streaming stb_zlib decoder: 19 functions, no tail |
| tu_131e34 (C++) | 0x131e34–0x134080 | Keyed cache and worker queue: 11 body functions plus 110 inline/template functions |
| tu_134080 (C++) | 0x134080–0x1376fc | Languages: six body functions, compiler init pair, 110 inline/template functions |
| tu_1376fc (C++) | 0x1376fc–0x138c50 | KeySettingWindow and touch-input worker: 12 written functions, compiler init pair, five inline virtuals |
| tu_138c50 (C++) | 0x138c50–0x13edfc | Wi-Fi/Hotspot menus: 44 body functions plus 122 inline/template functions |
| tu_13edfc (C++) | 0x13edfc–0x143720 | ImeWindow keyboard: 18 body functions plus 67 generated template/inline functions |
| tu_143720 (C++) | 0x143720–0x1484d4 | pinyin search table (tiny-utf8, `std::map`); the pilot |
| tu_1484d4 (C++) | 0x1484d4–0x14acfc | MultiDBCachedTextMenu, global ROM search |
| tu_14acfc … tu_14e0cc (6 C files) | 0x14acfc–0x14f730 | launching helpers, language/keymap, device I2C, Wi-Fi via wpa_cli |
| tu_14f730 (C, `-fPIC`) | 0x14f730–0x150ad8 | I2C device authentication: 8 functions, 5,032 bytes, no tail |

`tu_14f730` matches with `-O0 -fsigned-char -fPIC`; the library/vendor is unidentified. Per-file
extra flags are recorded in `src/matches.tsv` (`cflags`) and used by both match runners. Progress
now counts through the established libgcc boundary at 0x150ad8; the old 0x150000 cutoff omitted
three functions and truncated the block-transform function.

`tu_13edfc` matches with the default `-O0 -fsigned-char`: 12,936 body bytes to 0x142084,
then 5,788 tail bytes. Both boundaries are now proven. `ImeKey` is 100 bytes and `ImeWindow`
is 0xf0 bytes; `draw` and `restoreState` return `int` (shared window headers corrected).

`tu_138c50` matches with the default flags: 14,368 body bytes to 0x13c470, then 10,636 tail
bytes. Its proof requires the full 0x13edfc end: byte-identical pointer templates otherwise allow
a shorter layout. `unitcheck` now enforces recorded ends for proven units and supports `--end`
for a known neighbour when proving a draft. Shared `Action`, SDL prefixes, and the 0x48-byte
`ListMenu` base are laid out; `TextMenuDrawer` keeps its existing 0xf8-byte layout.

`tu_1376fc` matches with the default flags: 5,288 body bytes to 0x138ba4 and five inline
virtuals (172 bytes). `KeySettingWindow` is 0x15c bytes, with eight strings, eight SDL rectangles,
and a 64-byte serialized keymap. The touch worker reads `/dev/input/event0`; the window itself
waits for SDL key releases and saves the map on Escape.

`tu_134080` matches with the default flags: 4,768 body bytes to 0x135320 and 9,180 tail
bytes. The old draft `tu_133f60` began four allocator helpers too early; those 288 bytes belong
to the preceding file. Language records are 60 bytes (two strings plus a string vector); menu
choices are 48 bytes (filename and display name). The loader sorts by filename, creates English
fallback data, and writes `.short` files containing label IDs 88–155.

`tu_1302cc` matches with the default flags: 7,016 bytes, no tail. It is a streaming adaptation
of stb's zlib decoder, identified by its own `stb_zlib.cpp` assertions and `GNZLIB` messages.
`parse_header` retains the original missing return on successful blocks (its caller ignores the
result); the compiler warning is expected.

`tu_131e34` matches with the default flags: 2,348 body bytes and 6,432 tail bytes. It defines the
0x14-byte cache and 0x20-byte worker queue used by previously proven menu classes. The worker
helper has one thread; its constructor argument limits pending work, with a default of five.
The old `tu_12fe90` draft combined these two files with 1,084 bytes of the preceding file's
template tail. Its corrected preceding neighbour is `tu_12ef8c`, ending at 0x1302cc.

`tu_12ce28` matches with the default flags: 4,780 body bytes to 0x12e0d4 and 8,696 tail
bytes. The old `tu_12ef8c` draft was entirely part of this tail. The 0x1c-byte file-browser
helper holds a path stack, cached directory vectors, and a Boolean file-filter callback.
It bounds a directory load at 20,000 entries and trims old cached directories before navigation.
The inline `FileAction` constructor also matches; theme code before 0x12ce28 remains separate.

`tu_129f9c` matches with the default flags: 11,916 bytes, seven functions, no generated tail.
`Theme.h` records the 0x18c-byte configuration layout. Font paths support absolute overrides and
relative theme paths, with built-in fallback files. Image loading converts/composites surfaces;
sounds and background music also have explicit fallback paths. The loader retains its unused
font-size pointer argument, and cleanup retains the original resource order.

`tu_11cdc8` matches with the default flags: 29,296 body bytes to 0x124038 and 24,420 tail
bytes to 0x129f9c. Its 56 written functions cover scaling, SD-card/platform paths, mount/module
checks, ROM-cache cleanup, swap/CPU/memory helpers, recent-list load/save, app/emulator/theme
JSON parsing, factory-test results, serial formatting, image loading, and process lookup. The
compiler init pair constructs a string vector and the global recent-item list. The four later
drafts were all part of this unit's generated tail; the SQLite end proves its start.

`EmuInfo` now has its full 0xdc-byte layout: eight strings, three integer options, a string
vector, and the audio-fix option. The JSON `shortname` option is the field previously named
`hasCpinyin` by its search consumer. Recent items are 100 bytes; app records are 128 bytes.
The image helper's debug flag is Boolean; earlier caller declarations have been reconciled.
All Miyoo code after SQLite, 0x11cdc8–0x150ad8, is now covered by proven units.

`tu_049c3c` matches with the default flags: 4,740 body bytes to 0x04aec0 and 5,464 tail
bytes to 0x04c418. Its ten written functions include `main`, SIGTERM/exit handling, status and
volume polling, the hotplug worker, and Wi-Fi callbacks. The init pair constructs the global
recent list, app/emulator/theme vectors, default theme, and ROM-position vector. ROM-position
records are 36 bytes (a path string and three integer indices). `loadTheme` returns Boolean;
its shared declaration and definition are corrected without changing its matched code.

`tu_04c418` matches with the default C++ flags: 1,872 bytes, ten circular-list helpers and no
tail. The Ghidra export omitted the starts at 0x04c418 (indexed access) and 0x04c7a8 (count).
Both are now recorded in `ghidra/names.tsv` and `tools/manual_names.py`; the function denominator
therefore increases from 2,605 to 2,607.
The list nodes contain a value pointer and a next pointer. Allocation, sorted/unique insertion,
append/prepend, removal, traversal and cleanup retain their original edge-case behavior.

`tu_04cb68` matches with the default flags: 19,208 body bytes to 0x051670, then 27 inline
functions through 0x051b5c. The four zero bytes before cJSON at 0x051b60 are linker padding.
The written functions cover USB/device keyboards, key masks, LCD colors, sound, SD-card workers,
and the hardware-test menu. All 108 string-literal references were checked separately.
The SD-card workers retain the original use of an uninitialized end timestamp inside the loop.

## Next steps

The tracked Miyoo code is fully covered by proven units. `tu_015b20` integration is complete;
`docs/plan-tu_015b20.md` records the header order, shared row vector and bounds-checked overloads.
The program map now incorporates the item/action wiring, settings selectors and disabled entries.

The next engineering milestone is a complete linked executable: resolve cross-unit declarations,
original import/link settings, data and relocation targets, then compare the linked output. The
current proofs compare function code and unit layout with link-time words masked; 100% coverage
alone does not establish a byte-identical linked ELF or successful operation on hardware.

The [link audit](link-audit.md) now builds an **experimental executable** from all 22 units plus
strictly verified SQLite/cJSON objects and pinned SDK/runtime libraries. Run
`python3 tools/linkaudit.py --require-link`. All **286 final import names and versions**, the
**23-library DT_NEEDED order**, interpreter and entry point agree with the original. The audit
also verifies **59 storage extents / 9,102 bytes**, **185 strings** and **3,095 resolved body-call
destinations**, with zero mismatches. The theme filename argument now selects non-const string::at;
zipOpen uses the original lstat wrapper. All 22 units and 689 function matches are retained.

Actual linked **.text 0x15a30–0x1518e0 (1,294,000 bytes)** now MATCHes with existing relocation/call
masks. All 22 TU body anchors are exact. The 556-byte COMDAT deficit came from the independent
global g_recentList being declared as list<RecentItem> instead of **list<int>**; changing only that
local definition fixes real linker selection while preserving the complete tu_11cdc8 proof.
The audit retains static relocations and checks actual linked text, with a regression fixture
covering distinct byte-identical COMDATs. All 22 units, 56 affected function matches and five audit
tests pass; the other registered functions are unchanged from the prior 689/689 run.

All **14 raw ARM call/patch differences are resolved**: the eight original NOPs are preserved;
scaler/panel division signedness, browser vector::at calls and cache front insertion now agree with
the original. A future raw call/patch difference fails the audit. All 22 units, nine changed function
matches and five audit tests pass. No shared-header or manifest-signature changes were needed.

The full linked **.rodata 0x1518e8–0x164a50 (78,184 bytes)** is now **raw byte-identical**, including
vtable/typeinfo pointers: no masks. The audit enforces matching address, size and contents. The
recovery fixes constant/literal ordering, fixed patched string extents, assertion/function-name
data, hardware/Wi-Fi declaration order and two WifiMenuDrawer virtual bindings; see
[rodata-recovery.md](rodata-recovery.md). It also recovers the 29-entry legacy suffix pointer table.
The storage ledger now checks 58 extents / 8,689 bytes and 185 strings. All 22 whole units and six
audit tests pass alongside linked text, raw call/patch and import/version/order checks.

The full linked **.data 0x17e480–0x17ff2c (6,828 bytes)** and both exception tables
(**21,828-byte .ARM.extab / 12,936-byte .ARM.exidx**) now match without masks, and the
audit enforces all three. The panel object, command stream and MIPI configuration restore
the initialized-data deficit; a Wi-Fi header split restores unwind order without changing
other consumers. See [writable-data-recovery.md](writable-data-recovery.md).
The storage ledger now checks 59 extents / 9,102 bytes and 185 strings.

Next settle **BSS (160 bytes short)** and remaining GOT/code relocations. The device-ID
allocation, original iostream initializer placement, a 96-byte unreferenced interval and
C helper/COMMON ordering remain open. No guessed padding was added. .init_array,
.fini_array and .data.rel.ro still agree byte-for-byte. No device test or deployment has
occurred. The data/unwind work was committed as `e6e91d9`; the BSS follow-up is recorded in the step 1 commit accompanying [the handoff](handoff.md).

BSS step 1 restores the first-unit iostream/private-state order through TU-local linkage;
its absolute placement remains four bytes early. Only five device-ID bytes are proven,
and the four-byte gap before the launch buffer remains unexplained. See
[bss-step1.md](bss-step1.md). All 22 units, 310 affected function matches and the full link audit pass; BSS is still
160 bytes short.

BSS step 2 proves the 96-byte interval at 0x181b18 is zero storage in `tu_049c3c` that no
code references or initializes, in this build and in a second (stock) Miyoo build; its name,
type and partition are unrecoverable, and no placeholder was added. See
[bss-step2.md](bss-step2.md). BSS step 3 puts the C units' file-local statics in the original
`.bss` order, gives four objects the original COMMON linkage and `deviceInitialized` its
ordinary `.bss` slot; every object before the COMMON block now sits at a constant offset from
its original. ld orders COMMON per file in link order and by name hash within a file, so the
remaining COMMON order depends on the (stripped) names. The whole 160-byte deficit is
accounted for as unreferenced slots (4 + 96 + 4 + 4 + 4 + 48). All matches, 22 units, tooling
tests and the full link audit pass. See [bss-step3.md](bss-step3.md). Open decisions: names
for the within-file COMMON order, placeholders for unreferenced storage, then GOT/code
relocations. Read [handoff.md](handoff.md) to resume with another agent.

## Open findings and questions

- Draft boundaries from `tools/tusplit.py` are unreliable at the tail end (the pilot's tail ran 5
  units further than drafted); `unitcheck` settles them. Record proven ranges in `UNIT_FIXED`.
- The C units are split where `.rodata` proves a new file; see the comment at the top of
  `src/tu_14e0cc.c`.
- `ghidra/names.tsv` misses some function starts (template functions, PIC code); tools that need
  every start scan the binary or also read the Ghidra export.
- Further analysis: a `librsautil` + `MI_SYS_ReadUuid` device-check explanation, fuller hotkey/status
  documentation, and a diff against stock Miyoo MainUI (June 10 2023 build). The settings selectors
  and six disabled items are documented in the program map.

## Deferred tooling

Built and working: `matchfn`, `unitcheck`, `tusplit`, `packet`, `autoderive` (shapes, draft headers,
patterns), `progress`. Deferred until needed: `autoderive --like` (reuse one hand match across a
shape group) and `--promote` (assemble a unit file from matches automatically).

## Environment notes for a new machine or agent

- Follow `CLAUDE.md` (toolchain, MainUI and tiny-utf8 are fetched and hash-checked by scripts).
- The Ghidra exports `packet.py` and `autoderive.py` read (`functions.tsv`, `calls.tsv`,
  `strings.tsv`, `symbols.tsv`, `MainUI-354-clean_named.c`) are **not** in the repo, because they are
  decompiler output of the proprietary binary. Default location `~/mainui-exports` (or set
  `MAINUI_EXPORTS`); copy them privately or regenerate them in Ghidra 12.1.4 with the scripts in
  `tools/ghidra/` after applying `ghidra/ApplyMainUINames.java`.
- Start a session with: read `CLAUDE.md`, this file and `docs/unit-workflow.md`; confirm
  `scripts/run_matches.sh` and `scripts/run_units.sh` pass; take the next unit above; stop for review
  before committing.
