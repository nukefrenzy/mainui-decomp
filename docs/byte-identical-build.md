# Byte-identical build

`scripts/build-mainui.sh` builds MainUI from `src/` and proves the result byte-identical to
`MainUI-354-clean` (1,443,156 bytes, SHA-256
`98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94`). First achieved 2026-10-22.

```sh
scripts/build-mainui.sh        # fetches every pinned input, builds, prints build/link-audit/MainUI
```

The binary is Miyoo's program. It is written to the gitignored `build/link-audit/MainUI` and
must never be committed or published; the repository holds source, tools and documentation only.
No device test has been run.

## What the build does

`tools/linkaudit.py --require-ready` does the work; the script only fetches inputs and checks the hash.

1. **Compile** the 22 units with the pinned Arm GNU 8.2-2018.08 toolchain, `-O0 -fsigned-char`, plus
   per-file flags and dependencies from `src/matches.tsv` (`cflags`, `deps`). One unit,
   `tu_14f730` (the `-fPIC` I2C authentication code), uses **GCC 7.3.0** (`deps` = `gcc73`).
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
| BSS order and 160 missing bytes | TU-local/first-unit order, C statics in declaration order, COMMON linkage, unreferenced-storage placeholders | [bss-step1](bss-step1.md), [bss-step2](bss-step2.md), [bss-step3](bss-step3.md) |
| COMMON and GOT order | ld orders COMMON per file then by name hash, and global GOT slots by name hash; eight objects got hash-chosen names (originals are stripped) | [bss-step3](bss-step3.md) |
| Permuted jump table in `ImeWindow::initKey` | case order; plus an unmasked code-address check in the audit | [link-audit](link-audit.md) |
| Two references to the wrong variable (`drawFooter`, `drawHighlighted`) | exposed once BSS lined up | [bss-step3](bss-step3.md) |
| `.dynstr`/`.dynamic` order (`raise`) | `-lgcc` after the other shared libraries, as the gcc driver orders it | [link-audit](link-audit.md) |
| Debug info, relocations, `.symtab`, `.tm_clone_table` | production link without `--emit-relocs`, then `strip --strip-all` | this file |
| `.comment` string `GCC: (GNU) 7.3.0` | compile `tu_14f730` with an upstream GCC 7.3.0 build | below |

## GCC 7.3.0

The original's `.comment` holds `GCC: (GNU) 7.3.0` after the 8.2 toolchain's string: the plain
stamp of a GCC built from upstream sources. No runtime object of the 8.2 toolchain or the SDK
carries it, so one input object was compiled with GCC 7.3.0. `tu_14f730`, the separately built
`-fPIC` vendor code, is the evident candidate; compiled with GCC 7.3.0 its object is identical to
the 8.2 one in code, data, unwind tables, attributes, relocations and symbols, differing only in
`.comment`, and the final link becomes byte-identical. The bytes cannot prove which object
originally carried the stamp; this choice reproduces the binary exactly.

`scripts/fetch-gcc73.sh` downloads gcc-7.3.0, gmp-6.1.0, mpfr-3.1.4 and mpc-1.0.3 (the versions
GCC 7.3.0's `download_prerequisites` uses) from ftp.gnu.org and gcc.gnu.org's infrastructure
mirror, pinned by size and SHA-256, and builds only the C driver and `cc1` for
`arm-linux-gnueabihf` (armv7-a, NEON, hard float), reusing the 8.2 toolchain's assembler, linker
and sysroot. The build takes about 8 minutes; CI caches `build/gcc73/install`.

## Reconstructed names and storage

Names of internal objects are not in the binary (it is stripped), so every such name in `src/` is
a reconstruction. Eight were chosen to satisfy ld's hash ordering and say so in a comment. Five
placeholders (`g_unrefBss_*`, `s_unrefBss_*`, `g_unrefCommon_*`) stand for storage that exists at
those addresses in the Onion and stock builds but that no code references; their size and
position are proven, their names, types and partition are not.
