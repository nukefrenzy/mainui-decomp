# Byte-identical build

`scripts/build-mainui.sh` builds MainUI from `src/` and proves the result byte-identical to
`MainUI-354-clean` (1,443,156 bytes, SHA-256
`98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94`). First achieved 2026-10-22.

```sh
scripts/build-mainui.sh        # fetches every pinned input, builds, prints build/link-audit/MainUI
```

The binary is Miyoo's program. It is written to the gitignored `build/link-audit/MainUI` and
must never be committed or published; the repository holds source, tools and documentation only.
On 2026-10-22 the built binary was copied onto an Onion SD card as `MainUI-354-clean` and ran
on a Miyoo Mini Plus (backup taken first, copy checked with SHA-256 and `cmp`); it boots and runs.
The original was restored afterwards. On that card Onion's `runtime.sh` (`mount_main_ui`)
bind-mounts `.tmp_update/bin/MainUI-354-clean` over the empty `miyoo/app/MainUI`; the expert build
is used instead when `.tmp_update/config/.showExpert` exists. A device test only adds information
for a modified build: install it as a separate file and keep a backup.

## What the build does

`tools/linkaudit.py --require-ready` does the work; the script only fetches inputs and checks the hash.

1. **Compile** the 22 units with the pinned Arm GNU 8.2-2018.08 toolchain, `-O0 -fsigned-char`, plus
   per-file flags and dependencies from `src/matches.tsv` (`cflags`, `deps`). One unit,
   `DeviceAuth.c` (the `-fPIC` I2C authentication code), uses **GCC 7.3.0** (`deps` = `gcc73`).
2. **Rebuild and verify** the embedded SQLite 3.30.1 and cJSON 1.7.10 (`scripts/build-libs.sh`).
3. **Link** with `g++` against the pinned SDK/runtime libraries in the original DT_NEEDED order,
   with static `-lgcc` immediately before `libgcc_s.so.1`.
4. **Audit** the link (kept relocations, `--emit-relocs`): masked and unmasked `.text` proofs,
   exact data/unwind/dynamic sections, imports, storage and call destinations.
5. **Production link**: the same command without `--emit-relocs` or a map, then
   `strip --strip-all`, giving `build/link-audit/MainUI`. Its SHA-256 must equal the original's;
   `executable_ready` is true only then and only if every other check passes.

## What made it identical (after all 22 units were proven)

| Gap | Fix | Where documented |
| --- | --- | --- |
| BSS order and 160 missing bytes | TU-local/first-unit order, C statics in declaration order, COMMON linkage, unreferenced-storage placeholders | [BSS layout](#bss-layout) |
| COMMON and GOT order | ld orders COMMON per file then by name hash, and global GOT slots by name hash; eight objects got hash-chosen names (originals are stripped) | [BSS layout](#bss-layout) |
| Permuted jump table in `ImeWindow::initKey` | case order; plus an unmasked code-address check in the audit | [link-audit](link-audit.md) |
| Two references to the wrong variable (`drawFooter`, `drawHighlighted`) | exposed once BSS lined up | [BSS layout](#bss-layout) |
| `.dynstr`/`.dynamic` order (`raise`) | `-lgcc` after the other shared libraries, as the gcc driver orders it | [link-audit](link-audit.md) |
| Debug info, relocations, `.symtab`, `.tm_clone_table` | production link without `--emit-relocs`, then `strip --strip-all` | this file |
| `.comment` string `GCC: (GNU) 7.3.0` | compile `DeviceAuth.c` with an upstream GCC 7.3.0 build | below |

## GCC 7.3.0

The original's `.comment` holds `GCC: (GNU) 7.3.0` after the 8.2 toolchain's string: the plain
stamp of a GCC built from upstream sources. No runtime object of the 8.2 toolchain or the SDK
carries it, so one input object was compiled with GCC 7.3.0. `DeviceAuth.c`, the separately built
`-fPIC` vendor code, is the evident candidate; compiled with GCC 7.3.0 its object is identical to
the 8.2 one in code, data, unwind tables, attributes, relocations and symbols, differing only in
`.comment`, and the final link becomes byte-identical. The bytes cannot prove which object
originally carried the stamp; this choice reproduces the binary exactly.

`scripts/fetch-gcc73.sh` downloads gcc-7.3.0, gmp-6.1.0, mpfr-3.1.4 and mpc-1.0.3 (the versions
GCC 7.3.0's `download_prerequisites` uses) from ftp.gnu.org and gcc.gnu.org's infrastructure
mirror, pinned by size and SHA-256, and builds only the C driver and `cc1` for
`arm-linux-gnueabihf` (armv7-a, NEON, hard float), reusing the 8.2 toolchain's assembler, linker
and sysroot. The build takes about 8 minutes; CI caches `build/gcc73/install`.

## BSS layout

`.bss` is the original **35,624 bytes** at 0x17ff30, and every relocated `.text` word that targets
it matches without masks. Original addresses come from pairing references site by site (linked
`.text` is identical, so the same `MOVW`/`MOVT` site names the same object); the `-fPIC` unit
`DeviceAuth.c` reaches its objects through the GOT, so they are paired by the GOT offsets in its code
literals, not by slot address. `.rodata` constants that happen to equal BSS addresses (0x182690,
0x184668, 0x186f00) are not references.

### Allocation rules (probed on GCC 8.2 / binutils 2.30)

- **C++ units:** public zero definitions go to `.bss` first; TU-local (`static` or unnamed
  namespace) objects are deferred to the end of the unit's `.bss`. In `MainUI.cpp` this is why
  twelve objects (framebuffer state, settings-dirty, input/status flags, ...) are file-local:
  that puts `std::ios_base::Init` at 0x180858, as the static initializer at 0x3919c requires.
- **C file-local statics** (uninitialized or `= 0`) go to the file's `.bss` in **declaration
  order**, including ones declared after functions.
- **Public uninitialized C globals** are COMMON (`-fcommon`, GCC 8's default); an explicit `= 0`
  makes one an ordinary `.bss` definition (`deviceHasInit` in `DeviceAuth.c`).
- **ld places COMMON per input file, in link order, after all `.bss`;** within one file the order
  is the linker's hash-table order (`bfd_hash` of the name modulo 32,749 buckets for MainUI).
- **Global GOT slots** are assigned in hash-table order across the whole link.

So the C region is: per-file `.bss` of `Device.c`, `Settings.c`, `I2c.c`, `Wifi.c` and
`DeviceAuth.c` (0x1824b8–0x182680), then one COMMON block to the end of BSS (0x182680–0x188a58):
`g_langPath` and 48 unreferenced bytes (`BuiltinStrings.c`), `g_aps`, `g_scanBuffer`, `g_connStatus`
(`Wifi.c`), then `DeviceAuth.c`'s keys and buffers.

### Placeholders for unreferenced storage

No code reads or writes these bytes, but they exist at the same positions in the Onion build and
in stock Miyoo (June 2023), and the following objects' addresses require them. Size and position
are proven; name, type and partition (one object or several) are not. Each definition says so in
a comment.

| Original | Bytes | File | Definition |
| --- | ---: | --- | --- |
| 0x17ff50 | 4 | `MainUI.cpp` | `unsigned int g_unrefBss_17ff50;` after `g_deviceId` |
| 0x181b18 | 96 | `Main.cpp` | `unsigned char g_unrefBss_181b18[96];` after `g_lastVolumeTime` |
| 0x1824b4 | 4 | `Device.c` | `static int s_unrefBss_1824b4;` before `g_pwrKeyBuf` |
| 0x182670 | 4 | `Wifi.c` | `static int s_unrefBss_182670;` between `g_monitorThread` and `g_monitorRunning` |
| 0x182700 | 48 | `BuiltinStrings.c` | `char g_unrefCommon_182700[48];` (COMMON, hashes after `g_langPath`) |

### Hash-chosen names

Internal names are not in the stripped binary, so every such name in `src/` is a reconstruction.
Eight were chosen so that ld's hash order reproduces the original, and each says so in a comment:

- within-file COMMON order: `g_scanBuffer`, `g_connStatus` (`Wifi.c`); `deviceTxData`,
  `deviceCounterBuf`, `deviceRecvBuf` (`DeviceAuth.c`);
- global GOT slot order: `deviceHasInit`, `cipherKeys1`, `cipherKeys2` (`DeviceAuth.c`).

`g_aps`, `deviceKeyA` and `deviceKeyB` already sorted correctly. Names are link-time words, so
renaming changes no match.

### Source errors exposed by the exact layout

Masked checks cannot see which object a reference names. Once BSS lined up, two were wrong:
`drawFooter` blitted `g_navItem` where the original reads the footer image `g_keySettingFooter`
(0x1821ac), and `TextMenuDrawer::drawHighlighted` used `g_highlightColor` where the original uses
`g_accentColor` (0x1822b0).
