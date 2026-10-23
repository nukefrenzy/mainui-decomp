# mainui-decomp

Readable C++ source for **MainUI**, the launcher of the Miyoo Mini Plus as shipped in
[Onion OS](https://github.com/OnionUI/Onion). It rebuilds into a binary that is **byte-identical**
to the original.

MainUI is closed source. This repository reconstructs its source from the binary, with
descriptive names and comments, so that it can be read, checked and modified. The original
compiler is used to prove the reconstruction exact: `scripts/build-mainui.sh` compiles `src/`
and requires the SHA-256 of the result to equal the original's. CI repeats this on every push,
and the rebuilt binary has been run on a Miyoo Mini Plus.

> **AI was used for most of this work.** The source, names, comments, tools and documentation
> were largely written with Anthropic's Claude, under human direction. Human review and plenty
> of tools verified all AI output: every function, every file and the whole program are compiled
> and compared byte for byte with the original. Names and comments can't be proven that way and
> may still contain mistakes. [How this was made](docs/how-it-was-made.md) explains what was
> checked and how.

## Who this is for

- **Onion developers** who want to fix or extend MainUI: see [modifying.md](docs/modifying.md).
- **The [open-mainui](https://github.com/robcodedev/onionos-mainui-opensource) project** and
  anyone else checking how the original behaves: see [behavior.md](docs/behavior.md).
- **Anyone curious** how the launcher works, or how a byte-exact decompilation is done.

## Build it and prove it

On Linux x86-64 or WSL 2, with git, curl, make, m4, a host gcc/g++ and Python 3 with
pyelftools:

```bash
scripts/build-mainui.sh
```

The script downloads the original binary and the exact toolchain, checks both against pinned
hashes, builds, and fails unless the result is identical. The first run takes 10–15 minutes.
Full instructions are in [building.md](docs/building.md).

## Documentation

| Document | What's in it |
| --- | --- |
| [building.md](docs/building.md) | Requirements, the build, what gets downloaded, troubleshooting |
| [source-guide.md](docs/source-guide.md) | How MainUI works and where each part lives in `src/` |
| [behavior.md](docs/behavior.md) | What MainUI does: files and formats, launching, input, timings, original bugs |
| [modifying.md](docs/modifying.md) | Building a changed MainUI and trying it on a device |
| [onion-changes.md](docs/onion-changes.md) | How Onion's MainUI differs from Miyoo's stock build |
| [tools.md](docs/tools.md) | Every script and tool, including the Ghidra names |
| [how-it-was-made.md](docs/how-it-was-made.md) | The method, and how AI output was checked |
| [internals/](docs/internals/README.md) | Detailed records of the byte-identical build |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |

## The target

| | |
| --- | --- |
| Binary | `MainUI-354-clean` from Onion v4.4.0 beta 20260120 (commit `07505ea`) |
| SHA-256 | `98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94` |
| Compiler | Arm GNU Toolchain 8.2-2018.08 (`arm-linux-gnueabihf`), `-O0 -fsigned-char`; one file with GCC 7.3.0 |
| Source | 22 files, about 2,600 functions of Miyoo's own code, plus SQLite 3.30.1 and cJSON 1.7.10 |

Addresses in the source and in `ghidra/` refer only to this exact binary.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/` | The source: one file per original source file (named by content, except `stb_zlib.cpp`, the one original name known; address ranges in `docs/tu-map.tsv`), and shared headers |
| `docs/` | Documentation (see above) |
| `ghidra/` | Function and class names for Ghidra, and the script that applies them |
| `scripts/` | Fetch inputs, build, run all checks |
| `tools/` | The checking tools and the analysis tools used to recover the source |

## What is not here

The repository doesn't include MainUI, other Miyoo or SigmaStar binaries, the toolchain, or any
decompiler output. The build downloads what it needs from public sources and verifies each file.
The binaries you build stay in `build/`, which git ignores, and must not be committed here.

## License

The contents of this repository are licensed under the
[GNU General Public License v3.0](LICENSE), the same license as Onion OS.

Third-party code compiled into MainUI is downloaded at build time and isn't redistributed here:

- SQLite (public domain);
- cJSON (MIT);
- tiny-utf8 (BSD-3-Clause);
- RapidXml (Boost Software License or MIT).
