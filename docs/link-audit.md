# Link audit

`tools/linkaudit.py` builds MainUI from `src/` and checks the result section by section against
`MainUI-354-clean`. With `--require-ready` (what `scripts/build-mainui.sh` and CI run) it fails unless
the stripped production executable is **byte-identical** to the original. How the last gaps were
closed: [byte-identical-build.md](byte-identical-build.md).

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
scripts/fetch-mainui.sh
python3 tools/linkaudit.py --require-ready
```

It does not execute ARM code. Python 3.9+ with pyelftools, curl and tar are required; sources and
binaries stay in the ignored `build/`.

## Inputs

- **The 22 Miyoo units**, compiled with their manifest flags and dependencies (`src/matches.tsv`);
  `tu_14f730` uses GCC 7.3.0 (`scripts/fetch-gcc73.sh`).
- **Embedded libraries:** `scripts/build-libs.sh` builds SQLite 3.30.1 (C) and cJSON 1.7.10 (C++)
  in `build/embedded-libs/` from SHA-256-checked sources; `strictcmp.py` verifies their 808,104
  and 23,996 text bytes with only relocated words masked. `scripts/verify-libs.sh` runs the same
  build.
- **Target SDK libraries:** `scripts/fetch-target-libs.sh` fetches `support/my283.tar.xz` from
  [miyoomini-toolchain-buildroot commit 30e0e41](https://github.com/shauninman/miyoomini-toolchain-buildroot/tree/30e0e41f859f81b7ec390be34006ab1ba5f12f80/support)
  (SHA-256 `67c2dcc7aac183c1e3dd893dbb3c2f138ef59ae8fc738b7a266c98e67f32c038`), the SDK that
  [Onion's toolchain setup](https://github.com/OnionUI/dev-miyoomini-toolchain/blob/main/support/setup-toolchain.sh)
  bundles, and creates SONAME links for 17 ARM hard-float libraries in `build/target-libs/lib/`.
  Miyoo/MI libraries have no SONAME and use their filenames; zlib is 1.2.8, as in Onion's
  `libz.so.1`; libc, libstdc++, libgcc_s, libm, libpthread and libdl come from the pinned Arm GNU
  toolchain. These provide link-compatible symbols and versions; they are not the device's exact
  binaries (SDL, RSA and shared-memory libraries differ from Onion's tree). No vendor files are
  committed.

## Link recipe

1. `ld -r` of the units with the embedded objects inserted at their original positions: four
   Miyoo objects, cJSON, SQLite, then the remaining Miyoo objects.
2. Executable link with `g++` against the original **23 DT_NEEDED libraries in order** and
   interpreter `/lib/ld-linux-armhf.so.3`. Static `-lgcc` comes immediately before
   `libgcc_s.so.1`, as the gcc driver orders it: that supplies the arithmetic helpers statically,
   keeps the original dynamic unwinding imports and reproduces `.dynstr` (the position of `raise`).
3. **Audit ELF** `MainUI.audit.elf`, linked with `--emit-relocs` so every relocated word can be
   resolved and checked.
4. **Production executable**: the same command without `--emit-relocs` or a map, then
   `strip --strip-all`, giving `build/link-audit/MainUI`. Its SHA-256 must equal the original's;
   `executable_ready` is true only then and only if every other check passes.

## Checks

Any failure returns 1 (setup or compile errors return 2; a failed link removes the ELF).
`--require-link` also fails when linking fails; `--require-ready` also requires `executable_ready`.

| Area | Check |
| --- | --- |
| `.text` 0x15a30–0x1518e0 (1,294,000 bytes) | Masked compare of the actual linked code (relocation sites and BL/BLX masked), all 22 body anchors at their original addresses |
| Raw ARM calls and patches | Every mapped ARM instruction site raw-identical (literal pools excluded via mapping symbols); `linked_text.control_flow_differences` must be empty |
| Code addresses in relocated words | Every relocated `.text` word that targets code (`R_ARM_ABS32`, jump tables, `MOVW`/`MOVT` pairs) raw-identical |
| `.rodata` (78,184), `.data` (6,828), `.ARM.extab` (21,828), `.ARM.exidx` (12,936), `.dynstr`, `.dynamic` | Raw bytes, address and size, including pointers |
| Imports | 286 import names with per-symbol versions (GLIBC_2.4/2.7/2.28, GCC_3.5; COPY-relocated data handled), interpreter, DT_NEEDED order |
| Body calls | All 3,095 resolved body-call destinations equal the original's |
| Storage (`src/data.tsv`) | 59 extents / 9,102 bytes / 185 strings, language table followed through each ABS32 relocation |
| Production executable | SHA-256 equal to `MainUI-354-clean` |

Generated in `build/link-audit/`: `report.md`, `audit.json` (checks, exact commands, library hashes,
original-address evidence, `linked_text` placements), `unresolved.tsv`, `commands.txt`,
`embedded-libs.log`, `launcher.o`/`launcher.map`, `final-link.map`, the two executables and a
compile log per unit. None of it is committed.

## Source facts the audit established

The masked unit checks cannot see these; the link did.

**Declarations.** All internal-call and SQLite symbol mismatches were resolved from original call
destinations, using source declarations only (no linker aliases or wrappers). The shared headers
are `PlatformApi.h` (C-linkage process/configuration/device calls; `findProcess` has one C entry
point), `SQLiteApi.h` (all SQLite calls, C linkage), `TextMenu.h` (one class for the first and
hardware units; test rows cast from the shared `void*` vector), `ZlibDecoder.h` (C-compatible
decoder; `openZlib`/`readZlib`) and `PixelScale.h` (unsigned dimensions and stride, matching
`__aeabi_uidiv`). Examples of corrected callees: `milliseconds`, `clearWindows`,
`stopBackgroundThread`, `drawBackground`, `drawTitleBar(..., bool)`, `handleGlobalInput(SDL_Event*)`,
`NotifyWindow(..., void (*)())`, `TextMenu(bool, bool, int, const char*)`, `ensureSwap(void*)`,
`waitForProbe`/`authenticateDevice`. `openThemeFont` takes `std::string&` (non-const `at` import);
`zipOpen` calls `lstat`, not `stat`.

**Storage aliases.** Pairs of names that referred to one original object were merged into one
canonical definition: `g_launchCmd` (0x17ff54, 2,048 bytes), `g_iconFont`, `g_emptyImage`,
`g_gameFocused`/`g_gameNormal` (borrowed `SDL_Surface*` backgrounds, not coordinates), `g_raRow`,
`g_previewBg`, `g_volumeIcons`, `g_clearColor` (one `SDL_Color` for shadow and clear),
`g_highlightColor`, `g_selectedTextColor`, `g_hintColor` and `g_listColor`. The three read-only theme colors in
`tu_129f9c.cpp` are `SDL_Color` structs (a union folded their loads at `-O0`). Details:
[data-recovery.md](data-recovery.md).

**COMDAT identity.** The global at 0x182170 (`g_recentList`) is `std::list<int>`, not
`std::list<RecentItem>`. With the wrong type its generated constructor and destructor shared
COMDAT identities with the recent-games list and were discarded, shifting `tu_129f9c`.

**Jump table.** `ImeWindow::initKey`'s 45-case table at 0x140db0 needs cases 39, 43 and 44 right
after cases 0 and 1; otherwise 43 table words are permuted while the masked proof still passes.

**Preserved NOPs and call targets.** The original has calls patched to NOP (`0xe320f000`). The
source keeps GCC's argument setup and emits the NOP through a scoped GAS `bl` macro that asserts
the patch count; the macro is removed after each function. The 12 menu-factory patches use the
same technique.

| Sites | Source |
| --- | --- |
| 0x15fd4 | shadow-blit NOP in `drawText` |
| 0x168b4, 0x168cc, 0x16918, 0x16a2c, 0x16a44, 0x16a90 | background blit/fill NOPs in `drawWifiIndicator` |
| 0x14aea0 | disabled PWM-period write in `initBacklightPwm` |
| 0x2b748 | panel frame-rate arithmetic: promoted signed operands, `__aeabi_idiv` |
| 0x11ce1c, 0x11ce38 | scaler: unsigned counters and division, `__aeabi_uidiv` |
| 0x12d09c, 0x12d81c | `insertFileItem`/`freeItems` use `vector<void*>::at`, not `operator[]` |
| 0x132c10 | cache put/get use `push_front` (inline insertion wrapper calls `begin`) |

**Data sections.** Literal identity, source order and virtual bindings for `.rodata`:
[rodata-recovery.md](rodata-recovery.md). Initialized data and the exception sections:
[writable-data-recovery.md](writable-data-recovery.md). BSS order and size:
[byte-identical-build.md](byte-identical-build.md#bss-layout).

## Tooling tests

```sh
python3 -m unittest discover -s tools/tests -v
```

The ARM fixtures assemble and link real code and check that the audit rejects changed code, a
missing COMDAT identical to an earlier function, changed raw calls and re-enabled NOPs, a permuted
jump table (`test_permuted_jump_table`) and a corrupted relocated vtable pointer in `.rodata`. A
versioned shared-library fixture exercises added or missing imports, wrong versions, library order
and interpreter detection; the relocation fixture covers `MOVW`/`MOVT` base recovery (positive and
negative offsets, a carry across 64 KiB) and a call that resolves to a different function.
