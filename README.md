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

All **22 Miyoo translation units** pass whole-unit checks (**2,612 tracked functions, 458,064
bytes**), and the whole program now links **byte-identical** to the original:
`scripts/build-mainui.sh` builds MainUI from `src/` and requires its SHA-256 to equal the one
above, and CI does the same on every push. The rebuilt binary has also run on a Miyoo Mini Plus.
See [docs/byte-identical-build.md](docs/byte-identical-build.md) for how, and the
[link audit](docs/link-audit.md) for what is checked.

## What's here

| Path | Contents |
| --- | --- |
| [`docs/program-map.md`](docs/program-map.md) | How MainUI works: startup, the window loop, classes and menus, threads, launching, files, and where to hook in |
| `ghidra/` | `ApplyMainUINames.java` plus names for 4,800+ functions (with confidence tags), 69 classes and their vtables, globals, and exact library names |
| `src/` | Byte-exact C++: `TrimusWindow.h` and `MenuWindow.h` (exact layouts and virtual order), plus the rewritten functions |
| `tools/` | `matchfn.py` (one function) and `unitcheck.py` (a whole file) against the original; `tusplit.py` (source-file map), `packet.py` (per-function briefs), `autoderive.py` (patterns), `progress.py`; library matchers and the analysis scripts that produced the names |
| `scripts/` | Fetch and verify MainUI and the toolchain; run all matches; rebuild and compare the embedded libraries |

## Reading the source

Each `src/tu_<address>` file is one of MainUI's original source files, rebuilt in address order;
its header comment says what it does, and `docs/program-map.md` gives the overall picture. The
numbers MainUI passes around have names, defined in a few small headers:

| Header | Names |
| --- | --- |
| `src/Input.h` | SDL key symbols of the Miyoo buttons, MainUI's button codes, user-event codes, ListMenu results |
| `src/WindowIds.h` | window and menu ids (`WIN_SETTING`, `WIN_ROM_LIST`, ...) |
| `src/LangStrings.h` | the 156 user-interface strings, with their English text |
| `src/ShmVar.h` | settings shared with the keymon daemon (`MONITOR_VOLUME`, ...) |
| `src/PlatformApi.h` | Wi-Fi states and modes |
| `src/TrimusWindow.h` | what a window's `handleInput` result tells the window loop |

At `-O0` the code mirrors the original closely, so some statements look odd on purpose; comments
marked "kept as-is" point out original bugs and quirks that the byte match requires.

## Quick start (Linux or WSL)

```sh
pip install pyelftools
export MIYOO_TC=$(scripts/fetch-toolchain.sh)   # downloads and verifies the toolchain (~223 MB)
scripts/fetch-mainui.sh                          # fetches MainUI-354-clean from Onion's git history
scripts/verify.sh                                # checks both against their expected hashes
scripts/run_matches.sh                           # every rewrite in src/ must report MATCH
python3 tools/progress.py                        # matched bytes of Miyoo's own code
scripts/build-mainui.sh                          # build MainUI from src/; must equal the original byte for byte
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
4. Add one line to `src/matches.tsv` (its `deps` column names third-party headers such as
   `tinyutf8`; `cflags` is `-` or per-file extra flags such as `-fPIC`, identical across that file's rows),
   then regenerate the table in `src/MATCHES.md` with
   `python3 tools/progress.py --matches-md src/MATCHES.md`.
5. Once a whole file is written (`src/tu_<lo>.cpp`, ranges in `docs/tu-map.tsv`), check it as a unit with
   `python3 tools/unitcheck.py src/tu_<lo>.cpp`.

Only words the linker fills in (call targets, string and global addresses) are ignored when
comparing; every other byte must be identical.

## Working notes

Run every check the way CI does (pinned compiler and fetched dependencies only, never a host
compiler):

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
scripts/fetch-mainui.sh && scripts/fetch-tinyutf8.sh && scripts/fetch-rapidxml.sh
scripts/run_matches.sh                            # every registered function
scripts/run_units.sh                              # every whole unit
python3 -m unittest discover -s tools/tests -v    # tooling tests
python3 tools/linkaudit.py --require-ready        # full link audit + byte-identical executable
```

Source style is set by `.clang-format` (whitespace only; it never reorders includes, which fix the
`-O0` template-tail order). Format with `clang-format -i src/*.h src/*.cpp src/*.c src/tu_015b20/*.inc`
(any recent clang-format, e.g. `uv tool install clang-format`), then rerun the checks below.

Never relax masks, required unit ends or the raw section checks to obtain a pass. The link audit
writes `audit.json` (exact compile/link commands and original-address evidence), `report.md`,
`final-link.map` and the binaries to `build/link-audit/`; none of it is committed.

**Ghidra exports.** `tools/packet.py`, `autoderive.py`, `fn.py` and `fstr.py` read private exports
(`functions.tsv`, `calls.tsv`, `strings.tsv`, `symbols.tsv`, `MainUI-354-clean_named.c`) from
`~/mainui-exports/` or `$MAINUI_EXPORTS`. They are decompiler output of the proprietary binary, so
they are not in the repo: copy them privately or regenerate them in Ghidra with the scripts in
`tools/ghidra/` after applying `ApplyMainUINames`. Names in `functions.tsv` are pre-rename
(`ghidra/names.tsv` is current), and `names.tsv` misses some function starts (templates, PIC
code). Never read the `.c` export whole; extract single functions, for example
`python3 tools/packet.py 1237a8 3919c --asm --stdout`.

## Rules

- Never commit MainUI, other Miyoo binaries, or raw decompiler output. `.gitignore` blocks the common cases.
- Addresses are valid only for the exact binary above.

## License

Not yet chosen. Until a license is added, all rights are reserved by the author. SQLite (public domain) and
cJSON (MIT) are downloaded from their own sources at build time and are not redistributed here.
