# Building MainUI and proving it byte-identical

This page takes you from a clean Linux machine to a MainUI binary built from `src/` whose SHA-256
equals the original's. A matching hash shows that the source in this repository is a complete
and exact description of the original program.

## What you need

- **Linux x86-64**, or WSL 2 on Windows. The pinned compiler only runs on x86-64 Linux hosts.
- **About 2.5 GB of free disk space:**
  - about 1.6 GB for the Arm toolchain (archive plus unpacked);
  - about 400 MB to build GCC 7.3.0;
  - about 100 MB for the other inputs and the build output.
- **Host packages:** `git`, `curl`, `tar`, `xz-utils`, `bzip2`, `make`, `m4`, a host C and C++
  compiler (`gcc`, `g++`, only to build GCC 7.3.0), and Python 3 with `pyelftools`.

On Debian or Ubuntu:

```bash
sudo apt install git curl xz-utils bzip2 make m4 gcc g++ python3 python3-pyelftools
```

If your distribution has no `python3-pyelftools` package, install it with
`pip install pyelftools`, inside a virtual environment if pip refuses to install system-wide.

## Build and prove it

```bash
git clone https://github.com/nukefrenzy/mainui-decomp.git
```

```bash
cd mainui-decomp
```

```bash
scripts/build-mainui.sh
```

That's all. The script downloads every input, checks each one against a pinned SHA-256, builds,
and fails unless the result is byte-identical to the original. The first run takes 10–15
minutes, mostly building GCC 7.3.0; later runs take under a minute. It ends like this on success:

```
build/link-audit/MainUI: OK
/home/you/mainui-decomp/build/link-audit/MainUI
```

`build/link-audit/report.md` lists every check; its last line reads
`Executable ready (byte-identical and every check passing): True.`

Check it yourself if you like:

```bash
sha256sum MainUI-354-clean build/link-audit/MainUI
```

Both lines should show
`98c85f6c573bdeabd3762e8d9b596f354014e666cc873d0f758cbf3752620c94`.

## What gets downloaded

Nothing comes from this repository's authors except the source. Every input is fetched from its
public origin and verified against a pinned SHA-256:

| Input | From | Why |
| --- | --- | --- |
| `MainUI-354-clean` (1.4 MB) | Onion's GitHub repository, commit 07505ea | The original to compare against |
| Arm GNU Toolchain 8.2-2018.08 (223 MB) | Arm | The compiler Miyoo used, identified by its version stamp in the binary |
| GCC 7.3.0, gmp, mpfr, mpc (67 MB) | ftp.gnu.org, gcc.gnu.org | One unit was compiled with GCC 7.3.0; it is built from source |
| `my283.tar.xz` (Miyoo SDK libraries, 2 MB) | the miyoomini-toolchain-buildroot repository | The shared libraries MainUI links against, used only for linking |
| SQLite 3.30.1 | the npm `sqlite3` 4.1.1 package, which bundles that exact amalgamation | Compiled into MainUI |
| cJSON 1.7.10 | the cJSON GitHub repository | Compiled into MainUI |
| tiny-utf8 4.4.3, RapidXml 1.13 | GitHub, SourceForge | Header-only libraries compiled into MainUI |

Downloads go to `toolchain/` and `build/`, both ignored by git. Once fetched, an input is reused.

## What the build does

`scripts/build-mainui.sh` runs [`tools/linkaudit.py --require-ready`](../tools/linkaudit.py):

1. It compiles the 22 source files with `arm-linux-gnueabihf-g++ -O0 -fsigned-char`, using the
   per-file extra flags in [`src/matches.tsv`](../src/matches.tsv). `tu_14f730.c` gets `-fPIC`
   and GCC 7.3.0.
2. It rebuilds SQLite and cJSON and checks them against the copies inside the original.
3. It links in the original library order and strips the result.
4. It audits the result section by section, and finally compares the whole file.

The build succeeds only when every check passes and the hashes are equal.
[internals/byte-identical-build.md](internals/byte-identical-build.md) explains what each check
proves and what it took to get every byte right.

## Faster checks while you work

Building the whole program isn't necessary after every edit. These are quicker:

| Command | Checks | Time |
| --- | --- | --- |
| `python3 tools/unitcheck.py src/tu_14b860.c` | One source file, as the linker lays it out | seconds |
| `scripts/run_units.sh` | All 22 source files | about a minute |
| `python3 tools/matchfn.py src/tu_14b860.c loadSystemJson 14bca8 --orig MainUI-354-clean` | One function | seconds |
| `scripts/run_matches.sh` | Every function listed in `src/matches.tsv` | several minutes |
| `scripts/build-mainui.sh` | The whole program, byte for byte; the only check that sees string and data changes | under a minute |

These scripts need `MIYOO_TC` set to the toolchain's `bin` directory, which
`scripts/fetch-toolchain.sh` prints:

```bash
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
```

## Continuous integration

[`.github/workflows/match.yml`](../.github/workflows/match.yml) runs the same steps on every push:

- every function match and unit check;
- the tool tests;
- the full byte-identical build;
- a check that `ghidra/names.tsv` agrees with the names in the source.

## Troubleshooting

| Symptom | Cause and fix |
| --- | --- |
| `sha256sum: WARNING: 1 computed checksum did NOT match` while fetching | A download was interrupted or changed. Delete the file named in the message and run again. |
| `pip: externally-managed-environment` | Your distribution blocks system-wide pip. Install `python3-pyelftools` with apt, or use a virtual environment. |
| GCC 7.3.0 build fails | Look at `build/gcc73/build.log`. Usually `m4` or a host compiler is missing. |
| `cannot execute binary file` for the toolchain | You aren't on x86-64 Linux. Use WSL 2 or an x86-64 machine. |
| `Executable ready … False` in `build/link-audit/report.md` | Something in `src/` differs from the original. The report shows which section differs and where. If you changed the code on purpose, see [modifying.md](modifying.md). |

## The binary stays private

The built file is Miyoo's program, rebuilt. It is written to `build/` (ignored by git) and must
never be committed to this repository or published. The repository contains only source, tools
and documentation.
