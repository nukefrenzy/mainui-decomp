# Tools

Every script in `tools/` and `scripts/`, what it is for, and how to run it. Each one also
explains itself in its header comment or `--help`. The tools that compile need `MIYOO_TC`; run
`export MIYOO_TC=$(scripts/fetch-toolchain.sh)` first.

## Building and checking

These are the ones you need to build MainUI and prove the source is exact.

| Command | What it does |
| --- | --- |
| `scripts/build-mainui.sh` | Fetches every input, builds MainUI from `src/`, and fails unless it is byte-identical to the original ([building.md](building.md)) |
| `scripts/build-modified.sh` | The same build without requiring identity, for changed source ([modifying.md](modifying.md)) |
| `python3 tools/linkaudit.py [--require-ready]` | The engine behind both scripts. It compiles and links, then compares each section with the original. Output goes to `build/link-audit/`, including `report.md`. |
| `python3 tools/unitcheck.py src/<file>` | Compiles one source file and compares it with the original, laid out as the linker would place it: the body, then the template and inline functions in object order |
| `scripts/run_units.sh` | `unitcheck` on all 22 files |
| `python3 tools/matchfn.py src/<file> <function> <hex address> --orig MainUI-354-clean` | Compiles one function and compares it, with a side-by-side disassembly at the first difference |
| `scripts/run_matches.sh` | `matchfn` on every function listed in `src/matches.tsv` |
| `scripts/verify.sh` | Checks that `MainUI-354-clean` and the toolchain are the exact expected ones |
| `scripts/verify-libs.sh`, `scripts/build-libs.sh` | Rebuild SQLite 3.30.1 and cJSON 1.7.10 and compare them with the copies inside MainUI |
| `python3 -m unittest discover -s tools/tests` | Tests for the checking tools themselves |

Function and unit checks mask the words the linker fills in: call targets, and addresses of
strings and globals. Only the full build compares those words too, so run
`scripts/build-mainui.sh` before you trust a change that might touch strings or data.

## Fetching inputs

Each script downloads from the original source, checks a pinned SHA-256, and prints where the
result is.

| Script | Fetches |
| --- | --- |
| `scripts/fetch-mainui.sh` | `MainUI-354-clean` from Onion's GitHub history (commit 07505ea) |
| `scripts/fetch-toolchain.sh` | Arm GNU Toolchain 8.2-2018.08 into `toolchain/` |
| `scripts/fetch-gcc73.sh` | GCC 7.3.0 sources, built into `build/gcc73/` (C compiler only, about 8 minutes) |
| `scripts/fetch-target-libs.sh` | The Miyoo SDK shared libraries MainUI links against |
| `scripts/fetch-tinyutf8.sh`, `scripts/fetch-rapidxml.sh` | The two header-only libraries compiled into MainUI |

## Names and progress

| Command | What it does |
| --- | --- |
| `python3 tools/syncnames.py [--check]` | Copies every function name from the source into `ghidra/names.tsv`. CI runs `--check` so the two never disagree. |
| `python3 tools/progress.py [--matches-md src/MATCHES.md]` | Counts matched bytes and regenerates the function table in `src/MATCHES.md` |
| `python3 tools/stockdiff.py <stock MainUI>` | Compares Onion's MainUI with a stock Miyoo build, function by function ([onion-changes.md](onion-changes.md)) |

## Ghidra

| File | What it does |
| --- | --- |
| `ghidra/ApplyMainUINames.java` | Applies the names, classes, comments and global labels to a Ghidra project. Run it from the Script Manager after auto-analysis. It never overwrites names you set yourself. |
| `ghidra/names.tsv` | Address, name, subsystem, confidence and comment for 4,800+ functions |
| `ghidra/libnames.tsv` | Exact names of the SQLite and cJSON functions |
| `tools/ghidra/ExportStructure.java`, `ExportDecomp.java`, `ExportFew.java` | Export functions, the call graph, strings and decompiled code to text files for the analysis tools below |

To use the names:

1. Import `MainUI-354-clean` into Ghidra 12.1.4 with the default options, and let auto-analysis
   run.
2. Add the `ghidra/` folder in the Script Manager.
3. Run **ApplyMainUINames**.

Confidence tags:

- **Reliable:** `library-exact`, `confirmed`, `inferred` and `likely`.
- **Signposts only:** `guess`.

## Analysis tools used to recover the source

These produced the names, layouts and file boundaries. You don't need them to build. Some read
Ghidra exports of the original binary, which aren't in the repository because they are derived
from Miyoo's code. Regenerate them with the scripts in `tools/ghidra/` and point
`MAINUI_EXPORTS` at the folder.

| Tool | What it does |
| --- | --- |
| `tools/rtti.py` | Recovers the C++ classes, base classes and vtables from the binary's type information |
| `tools/cluster.py`, `tools/manual_names.py` | Assign subsystems and first names to functions; `manual_names.py` holds the hand-verified ones |
| `tools/tusplit.py` | Finds the original source file boundaries and writes `docs/tu-map.tsv` |
| `tools/packet.py` | Writes a one-file brief for a function (its calls, strings, disassembly and decompilation) to start a rewrite from |
| `tools/autoderive.py` | Groups functions by code shape and tries known source patterns on them, keeping only those that compile to the exact bytes |
| `tools/matchobj.py`, `tools/strictcmp.py`, `tools/firstdiff.py` | Lower-level comparisons of a compiled object against the original |
| `tools/fn.py`, `tools/fstr.py` | Print a decompiled function, or the strings and imports a function uses, from the exports |
| `tools/probe/std_probe.cpp` | A test file used to learn how GCC 8 at `-O0` orders functions and template code |

[how-it-was-made.md](how-it-was-made.md) describes how these fit together.
