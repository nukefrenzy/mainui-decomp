# Status and next steps

Snapshot of 2026-10-19. Live numbers: `python3 tools/progress.py`; unit ranges: `docs/tu-map.tsv`;
method: `docs/unit-workflow.md`. Update this file when a unit is proven or the plan changes.

## Where things stand

- **Goal:** complete MainUI source that rebuilds byte-exact with Arm GNU 8.2-2018.08, `-O0 -fsigned-char`,
  worked file by file (one translation unit per `src/tu_<lo>.cpp` or `.c`).
- **Progress:** 18 whole units proven by `tools/unitcheck.py`, plus 9 single functions in
  `tu_015b20`: 1,320 of 2,605 Miyoo functions, 213,628 of 457,584 bytes (46.69%). CI checks every match
  and every unit on each push.
- **Embedded libraries:** SQLite 3.30.1 and cJSON 1.7.10 rebuild byte-exact (`scripts/verify-libs.sh`);
  tiny-utf8 4.4.x is header-only and appears in the template tails of the files that use it.

| Proven unit | Range | What it is |
| --- | --- | --- |
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

## Next units, in order

Work outward from proven units: a proven neighbour fixes one boundary for free. One fresh session per
unit, each in its own worktree; merge their branches into `main` one at a time.

1. **tu_049c3c** (main program: main, signal handler, status poll), followed by
   **tu_04c12c** (hardware test windows). Confirm their body/tail boundaries before treating
   the draft ranges 0x049c3c–0x04c12c and 0x04c12c–0x051b60 as whole files.
2. **tu_015b20 last:** the first file, 212 KB, about 46% of all bytes (all window/menu classes,
   actions, createMenu). It benefits from the class headers the other units produce. The original
   "item and action wiring" question is answered inside it.

## Open findings and questions

- Draft boundaries from `tools/tusplit.py` are unreliable at the tail end (the pilot's tail ran 5
  units further than drafted); `unitcheck` settles them. Record proven ranges in `UNIT_FIXED`.
- The C units are split where `.rodata` proves a new file; see the comment at the top of
  `src/tu_14e0cc.c`.
- `ghidra/names.tsv` misses some function starts (template functions, PIC code); tools that need
  every start scan the binary or also read the Ghidra export.
- Not done: a `librsautil` + `MI_SYS_ReadUuid` device check, `handleGlobalInput` details, the
  status-bar indicators, the five allocated-but-unconstructed settings entries, and a diff against
  stock Miyoo MainUI (June 10 2023 build).

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
