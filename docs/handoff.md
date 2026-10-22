# Handoff: linked-image recovery

Snapshot: 2026-10-22. Start here, then read `CLAUDE.md`, `docs/status.md`,
`docs/unit-workflow.md`, `docs/link-audit.md`, `docs/writable-data-recovery.md`,
`docs/bss-step1.md`, `docs/bss-step2.md` and `docs/bss-step3.md`. Read current branch/commit
state from Git rather than old chat. Updated for the BSS steps 2 and 3 commit following `b24cb99`.

## Authorization and stopping points

The user requested three BSS steps, **stopping for confirmation before each next
step**. All three have been worked on and committed locally (not pushed): step 1 is
partially resolved, step 2 is fully investigated (no source change, by the user's choice),
and step 3's approved fixes are applied. No further step is approved. Renaming objects to
reproduce COMMON order, adding placeholder storage, and the GOT/relocation audit each need
the user's explicit decision. Do not commit, merge or push additional work without a new
request. If committing, use both author and committer
`nukefrenzy <nukefrenzy@users.noreply.github.com>`.

## Proven state and limits

- All **22 Miyoo whole units** match (2,612 tracked functions / 458,064 tracked bytes).
- Actual linked `.text`: **1,294,000 bytes**, MATCH with established relocation/call
  masks; all 22 body anchors exact. Separate raw ARM call/patch differences: **zero**.
- Raw byte-identical, including pointers and original addresses/sizes: `.rodata`
  **78,184 bytes**, `.data` **6,828**, `.ARM.extab` **21,828**, `.ARM.exidx` **12,936**.
  The link audit enforces these four section proofs without masks.
- **286 import names/versions**, interpreter and **23-library DT_NEEDED order** agree;
  **3,095 body calls** and **59 storage extents / 9,102 bytes / 185 strings** pass.
- BSS is **35,464 versus 35,624 bytes**, still **160 bytes short**, at the correct
  section base 0x17ff30. The deficit is fully accounted for as unreferenced slots
  (4 + 96 + 4 + 4 + 4 + 48, see `docs/bss-step3.md`); equal zero bytes alone would not prove
  object allocation/order.
- Init/fini arrays and `.data.rel.ro` also agree; these are observations, not additional
  enforced whole-section checks. GOT and code relocation values remain incomplete.
- The last full individual sweep passed **689/689** before BSS step 1. After step 1,
  **310/310 affected first-unit matches**, **22/22 whole units** and the full link audit
  passed. The remaining 379 registered functions were unchanged. The six ARM tooling
  tests passed during the preceding data/unwind recovery; tools did not change in step 1.
- No whole-ELF byte identity or hardware/device validation is claimed. Do not deploy
  the experimental audit ELF as MainUI.

## Step 1 findings: preserve these

Twelve framebuffer/private UI state definitions in `src/tu_015b20.cpp` are now
TU-local. GCC's local `.comm` allocation puts them after iostream's initializer,
restoring relative order without adding storage. Their redundant extern declarations
were removed from `tu_015b20.h` and p11; the data manifest uses their local mangled
symbols. This header is included only by its owner TU. No fields, function signatures,
allocation sizes or masks changed.

`g_deviceId` at **0x17ff48** retains **five proven bytes**. The formatter at 0x1237a8
loads offsets 3, 2, 1, 0, 4. The next known original object is `g_launchCmd` at
**0x17ff54**; it currently links at **0x17ff50**. That twelve-byte bounded interval
does not prove a twelve-byte array. Do not enlarge the ID or add reserve padding just
to close the four-byte gap.

The iostream initializer should be **0x180858** and is now **0x180854**, immediately
before `g_fbVar` (**0x18085c**, rebuilt **0x180858**). The remaining first-unit state
and the next TU's status flag are consequently four bytes early. Details and exact
addresses are in `docs/bss-step1.md`. Step 1 did not reduce the total BSS deficit.

## Steps 2 and 3: done, preserve these

**Step 2** (`docs/bss-step2.md`): 0x181b18–0x181b78 is 96 bytes of zero storage in
`tu_049c3c`, between `g_lastVolumeTime` and the file-local `g_loggedWifi`, that no code
references or initializes. A stock Miyoo MainUI (private, `~/mainui-stock/`, SHA-256
`607dacd9…fd2bc`) has the same unused 96 bytes at the same position. Name, type and partition
are unrecoverable; the user chose not to add a placeholder.

**Step 3** (`docs/bss-step3.md`): file-local statics in `tu_14acfc.c` and `tu_14e0cc.c` are in
the original `.bss` order; `g_aps`, `g_scanBuf`, `g_wifiStatus` and `g_langPath` are public
(COMMON); `deviceInitialized` is `= 0` (ordinary `.bss`, still public so `-fPIC` code is
unchanged). Every object before the COMMON block now has a constant offset from its original.
Rules verified with probes: C statics follow declaration order; ld places COMMON per file in
link order, by `bfd_hash` name order within a file. For the PIC unit, pair objects through the
GOT offsets in code literals, not by GOT slot address.

## Next work, each needing the user's decision

- **Within-file COMMON order:** needs names for `tu_14e0cc`'s three and `tu_14f730`'s five
  COMMON objects whose hash order matches the original (the originals are stripped).
- **Unreferenced storage:** placeholders for the device-ID slack, step 2's 96 bytes, and the
  4 + 4 + 48 bytes in the C units would make BSS size and addresses exact; their names and
  partition would remain unknown.
- **After BSS:** audit the remaining GOT and code relocation values.

For direct ARM MOVW/MOVT address scans, use the opcode mask **0x0ff00000** with
0x03000000 / 0x03400000. **0x0fff0000 is wrong**: it includes immediate bits and
misses references. `tools/linkaudit.py:source_evidence` already uses the correct
mask. Earlier ad hoc gap classifications should be rechecked with the correct mask.

## Reproduce the environment and checks

Use the pinned compiler and fetched dependencies, never substitute a host compiler:

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
scripts/fetch-mainui.sh
scripts/fetch-tinyutf8.sh
scripts/fetch-rapidxml.sh
scripts/run_matches.sh
scripts/run_units.sh
python3 -m unittest discover -s tools/tests -v
python3 tools/linkaudit.py --require-link
```

Python requires pyelftools. `CLAUDE.md` has the compiler/flags and manifest workflow.
`--require-ready` deliberately fails because final layout proof is incomplete.
Do not relax masks, required unit ends or the raw section checks to obtain a pass.

The link audit generates `build/link-audit/audit.json`, `report.md`, `commands.txt`,
`final-link.map` and `MainUI.audit.elf`. They can be regenerated; they are not committed.
The JSON contains exact compile/link commands and original-address evidence.
`src/data.tsv` checks initializers and declared extents, not all final BSS addresses.
Use original instructions plus the final symbol table/map for placement evidence.

Local diagnostic files may remain under ignored `build/bss-step1/`, `build/writable/`
and `build/rodata/`; they are optional aids, not portable dependencies. In particular,
old probe scripts reuse cached objects and can miss cross-object COMDAT effects.
Always validate a retained change with the full fresh link and whole-unit checks.

## Ghidra exports and repository rules

Private exports are in `~/mainui-exports/` or `$MAINUI_EXPORTS`: `functions.tsv`,
`calls.tsv`, `strings.tsv`, `symbols.tsv`, `MainUI-354-clean_named.c`. Another computer
needs these copied privately or regenerated. `functions.tsv` names are pre-rename;
`ghidra/names.tsv` has current names. **Never read the .c export whole.** Use per-function
packets, for example:

```sh
python3 tools/packet.py 1237a8 3919c --asm --stdout
```

`tools/fn.py` and `tools/fstr.py` are alternatives. Do not commit binaries, SDK libraries,
objects, toolchains, raw decompiler output or generated build reports. The only supported
original is `MainUI-354-clean`, SHA-256
`98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94`.
