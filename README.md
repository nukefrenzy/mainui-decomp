# mainui-decomp

Reverse engineering and byte-matching decompilation of **MainUI**, the launcher of the
Miyoo Mini Plus, as shipped in [Onion OS](https://github.com/OnionUI/Onion).

MainUI is closed source. This repository holds only original work about it: function names,
class layouts, documentation, tools, and C++ rewrites that compile to the **exact same bytes**
as the original. **The MainUI binary is not included**; a script fetches it from Onion's public
repository and checks its hash.

## Target

| | |
| --- | --- |
| Binary | `MainUI-354-clean` (Miyoo Mini Plus, Onion v4.4.0 beta 20260120, commit `07505ea`) |
| SHA-256 | `98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94` |
| Compiler | Arm GNU Toolchain 8.2-2018.08 (`arm-linux-gnueabihf`, `arm-rel-8.23`) |
| Flags | `-O0 -fsigned-char` (C++ via `g++`; no CPU flags needed) |

The compiler and flags are proven, not guessed. The two libraries compiled into MainUI rebuild
byte-identical from their public sources: SQLite 3.30.1 (808,104 bytes) and cJSON 1.7.10
(23,996 bytes). Every rewrite in `src/` is checked the same way, on every push, by CI.

## What's here

| Path | Contents |
| --- | --- |
| [`docs/program-map.md`](docs/program-map.md) | How MainUI works: startup, the window loop, classes and menus, threads, launching, files, and where to hook in |
| `ghidra/` | `ApplyMainUINames.java` plus names for 4,800+ functions (with confidence tags), 69 classes and their vtables, globals, and exact library names |
| `src/` | Byte-exact C++: `TrimusWindow.h` and `MenuWindow.h` (exact layouts and virtual order), plus the rewritten functions |
| `tools/` | `matchfn.py` (checks a function against the original), library matchers, and the analysis scripts that produced the names |
| `scripts/` | Fetch and verify MainUI and the toolchain; run all matches; rebuild and compare the embedded libraries |

## Quick start (Linux or WSL)

```sh
pip install pyelftools
export MIYOO_TC=$(scripts/fetch-toolchain.sh)   # downloads and verifies the toolchain (~223 MB)
scripts/fetch-mainui.sh                          # fetches MainUI-354-clean from Onion's git history
scripts/verify.sh                                # checks both against their expected hashes
scripts/run_matches.sh                           # every rewrite in src/ must report MATCH
scripts/verify-libs.sh                           # SQLite and cJSON rebuild byte-identical
```

### Using the names in Ghidra (12.1.4 tested)

1. Import `MainUI-354-clean` with the default options (ARM v8, little-endian, 32-bit) and run auto-analysis.
2. In Script Manager, add the `ghidra/` folder, run **ApplyMainUINames**, and choose the `ghidra/` folder.
3. It creates functions that auto-analysis missed, then applies names, class namespaces, comments
   and global labels. It never overwrites names you set yourself, so re-running is safe.

Confidence tags: `library-exact`, `confirmed`, `inferred` and `likely` are reliable. `guess` names
(`<subsystem>_<hint>_<addr>`) are signposts only.

## Contributing a matching function

1. Pick a function. The names in `ghidra/names.tsv` and `docs/program-map.md` are a good place to start.
2. Write it in `src/` using the existing headers.
3. Check it: `python3 tools/matchfn.py src/YourFile.cpp functionName <hex address> --orig MainUI-354-clean`.
   It prints `MATCH`, or a side-by-side disassembly at the first difference.
4. Add it to the list in `scripts/run_matches.sh` and to the table in `src/MATCHES.md`.

Only words the linker fills in (call targets, string and global addresses) are ignored when
comparing; every other byte must be identical.

## Rules

- Never commit MainUI, other Miyoo binaries, or raw decompiler output. `.gitignore` blocks the common cases.
- Addresses are valid only for the exact binary above.

## License

Not yet chosen. Until a license is added, all rights are reserved by the author. SQLite (public domain) and
cJSON (MIT) are downloaded from their own sources at build time and are not redistributed here.
