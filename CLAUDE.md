# CLAUDE.md — mainui-decomp

Reverse engineering and byte-matching decompilation of the Miyoo Mini Plus **MainUI** launcher
(Onion OS build). Read `README.md` and `docs/program-map.md` before starting work.

## Working agreement

- Ask clarifying questions before code changes, summarize the planned change, and get approval before making it.
- Keep usage low: work from structure (names, xrefs, strings, scripts) and hand-read only the functions a task needs.
  Never read `MainUI-354-clean_named.c` or whole decompiler dumps; grep and extract single functions.

## Hard rules

- **Never commit** MainUI, any Miyoo/SigmaStar binary, `*.so`, `*.o`, toolchain files, Ghidra projects or raw
  decompiler output. `.gitignore` covers the usual names; check `git status` before every commit.
- Commits: author `nukefrenzy <nukefrenzy@users.noreply.github.com>` (keeps the real email private).
- Addresses are valid only for `MainUI-354-clean`, SHA-256 `98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94`.

## Environment (Linux / WSL)

```sh
pip install pyelftools
export MIYOO_TC=$(scripts/fetch-toolchain.sh)   # Arm GNU 8.2-2018.08, SHA-256 verified
scripts/fetch-mainui.sh                          # from Onion git, commit 07505ea
scripts/verify.sh
```

Compiler: `arm-linux-gnueabihf-g++ -O0 -fsigned-char` (proven via SQLite 3.30.1 and cJSON 1.7.10, byte-identical).
Ghidra 12.1.4: import with defaults, auto-analyze, run `ghidra/ApplyMainUINames.java` on `ghidra/`.

## Adding a byte-exact function

1. Write it in `src/` against the existing headers (`TrimusWindow.h`, `MenuWindow.h`).
2. `python3 tools/matchfn.py src/File.cpp <name|mangled> <hexaddr> --orig MainUI-354-clean` must print `MATCH`.
   Use mangled names to pick a specific ctor/dtor variant (C2/D2 = base, D0 = deleting).
3. Add it to `scripts/run_matches.sh` and `src/MATCHES.md`. CI (`.github/workflows/match.yml`) re-checks all of them.

At `-O0` the code mirrors the source closely: keep statement order, redundant assignments and member order exactly
as the disassembly shows. Externs (calls, globals, strings) are link-time words and are ignored by the checker.

## Names

`ghidra/names.tsv` columns: addr, name, subsystem, confidence, comment. Confidence: `library-exact`, `confirmed`
(read in code / RTTI slot / byte-exact), `inferred`, `likely` (writes the class vtable), `guess` (automatic
`<subsystem>_<hint>_<addr>`; signposts only), `library`. Hand-verified names live in `tools/manual_names.py`
(`M = {...}`); regenerate with `tools/cluster.py` then `tools/manual_names.py` (inputs come from the Ghidra
export scripts in `tools/ghidra/`). Never downgrade a confirmed name to a guess.

## Key facts (details in docs/program-map.md)

- Window stack: `runWindowLoop` 0x36ee8, `pushWindow` 0x36d88, `createMenu(id, path)` 0x37784, `IconMenu::onSelect` 0x27724.
- `TrimusWindow` (12 bytes): vtable, `titleLabelId` +4, `id` +8; 11 virtual slots, all identified.
- `MenuWindow` 0x11c bytes, fully laid out in `src/MenuWindow.h`.
- Libraries inside the binary: cJSON 0x51b60–0x57920, SQLite 0x57920–0x11cdc8 (names in `ghidra/libnames.tsv`).
- Launch path: `AppAction::execute` → `launchApp_buildCommand` 0x18560 → `writeCmdToRun` 0x17fc0 (`/tmp/cmd_to_run.sh`), then MainUI exits.
- Clean vs Expert: two NOPs in `createMenu` at 0x37994 and 0x379a0 (hide the RetroArch icon).

## Roadmap

Done: program map, exact toolchain + flags, match toolkit + CI, phase 2 option 1 (window layout).

Next, in order:
2. **Item and action wiring.** Known so far: `IconMenuItem` ctor 0x256f0 (labelId +0x40, param +0x44, menuId +0x4c,
   icons as strings at +0x50/+0x68); `SettingMenuItem` ctor 0x282d0 (labelId, icon, handler?); item list push 0x27a30;
   `Action::execute` = slot 2. To do: full item layouts, how a selected item becomes an Action, settings handlers and
   label IDs, the five unconstructed settings entries.
3. **Starter LD_PRELOAD hook library.** Same toolchain; wrap `SDL_PollEvent` for a hotkey; inline-hook an internal
   function at a fixed address. Compile-tested here, device-tested by Brad.

Later candidates: input/hotkeys, status bar indicators, ROM lists + DB, launching, themes, settings shared memory,
Wi-Fi, the `librsautil` + UUID device check, and a diff against stock Miyoo (June 10 2023 build).
