# Status and next steps

Snapshot of 2026-10-19. Live numbers: `python3 tools/progress.py`; unit ranges: `docs/tu-map.tsv`;
method: `docs/unit-workflow.md`. Update this file when a unit is proven or the plan changes.

## Where things stand

- **Goal:** complete MainUI source that rebuilds byte-exact with Arm GNU 8.2-2018.08, `-O0 -fsigned-char`,
  worked file by file (one translation unit per `src/tu_<lo>.cpp` or `.c`).
- **Progress:** 9 whole units proven by `tools/unitcheck.py`, plus 9 single functions in
  `tu_015b20`: 347 of 2,605 Miyoo functions, 55,588 of 457,584 bytes (12.15%). CI checks every match
  and every unit on each push.
- **Embedded libraries:** SQLite 3.30.1 and cJSON 1.7.10 rebuild byte-exact (`scripts/verify-libs.sh`);
  tiny-utf8 4.4.x is header-only and appears in the template tails of the files that use it.

| Proven unit | Range | What it is |
| --- | --- | --- |
| tu_143720 (C++) | 0x143720–0x1484d4 | pinyin search table (tiny-utf8, `std::map`); the pilot |
| tu_1484d4 (C++) | 0x1484d4–0x14acfc | MultiDBCachedTextMenu, global ROM search |
| tu_14acfc … tu_14e0cc (6 C files) | 0x14acfc–0x14f730 | launching helpers, language/keymap, device I2C, Wi-Fi via wpa_cli |
| tu_14f730 (C, `-fPIC`) | 0x14f730–0x150ad8 | I2C device authentication: 8 functions, 5,032 bytes, no tail |

`tu_14f730` matches with `-O0 -fsigned-char -fPIC`; the library/vendor is unidentified. Per-file
extra flags are recorded in `src/matches.tsv` (`cflags`) and used by both match runners. Progress
now counts through the established libgcc boundary at 0x150ad8; the old 0x150000 cutoff omitted
three functions and truncated the block-transform function.

## Next units, in order

Work outward from proven units: a proven neighbour fixes one boundary for free. One fresh session per
unit, each in its own worktree; merge their branches into `main` one at a time.

1. **tu_13edfc, ImeWindow** (0x13edfc–0x143720, about 18.7 KB): the on-screen keyboard. The end is
   proven; 0x1424e8–0x143720 is its template tail (not a separate file). Body is about 13 KB by hand:
   ctor 1,060 B, dtor 688 B, handleInput 1,716 B, draw helpers 1,756 B and 3,716 B, render 1,036 B.
   The start 0x13edfc is not proven yet.
2. **tu_138c50, Wi-Fi/Hotspot menus** (0x138c50–0x13edfc, 25 KB), then going down through
   tu_1376fc (KeySettingWindow), tu_133f60 (language) and so on toward 0x11cdc8.
3. **Small units with draft boundaries** (tu_126c60 1.5 KB, tu_1297a8 2 KB, tu_12ef8c 3.8 KB) are
   cheap but unbounded; take them when a neighbour is proven.
4. **tu_049c3c** (main program: main, signal handler, status poll) and **tu_04c12c** (hardware test
   windows) after the 0x11cdc8+ region.
5. **tu_015b20 last:** the first file, 212 KB, about 46% of all bytes (all window/menu classes, actions,
   createMenu). It benefits from the class headers the other units produce. The original
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
