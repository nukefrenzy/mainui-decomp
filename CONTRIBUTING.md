# Contributing

Thanks for helping. This repository keeps a source that rebuilds the original MainUI **byte for
byte**. Contributions that keep it that way are welcome:

- better names and comments;
- corrections, with the evidence;
- documentation;
- notes on MainUI's behavior for other projects;
- tool improvements.

Changes to what MainUI *does* don't belong here, because they would break the byte-identical
build. Make those in your own fork or branch; [docs/modifying.md](docs/modifying.md) shows how.

## Before you open a pull request

1. Build once as described in [docs/building.md](docs/building.md).
2. Make your change.
3. Run the checks CI runs:

   ```bash
   export MIYOO_TC=$(scripts/fetch-toolchain.sh)
   ```

   ```bash
   scripts/run_units.sh
   ```

   ```bash
   scripts/build-mainui.sh
   ```

   ```bash
   python3 tools/syncnames.py --check
   ```

   ```bash
   python3 -m unittest discover -s tools/tests
   ```

4. If you renamed functions, run `python3 tools/syncnames.py` to update `ghidra/names.tsv`.
5. If you added or renamed a function listed in `src/matches.tsv`, regenerate the table with
   `python3 tools/progress.py --matches-md src/MATCHES.md`.

`scripts/build-mainui.sh` must pass. The unit checks can't see strings, so a rename that touches
a string literal passes them but fails the full build. String literals, file names and log
messages are part of the original program; keep them exactly as they are, including their
spelling (such as `favourite.json`).

## Style

- **Formatting.** Format with the repository's `.clang-format` (any recent clang-format):

  ```bash
  clang-format -i src/*.h src/*.cpp src/*.c src/tu_015b20/*.inc
  ```

  It only changes whitespace and never reorders includes, because include order matters here.
- **Spelling.** Use American English in names, comments and documentation.
- **Order.** Keep functions in address order, with the `// 0x…` address comment before each.
- **Comments** explain what code does and why, not how it was recovered. Mark kept original
  defects with `Original bug, kept as-is:` or `Original quirk, kept as-is:`.
- **Changing a name.** Give the evidence in the pull request (a caller, a string, the type
  information). Never downgrade a `confirmed` name to a guess.

## Rules

- **Never commit binaries:** MainUI, other Miyoo or SigmaStar files, `*.so`, `*.o`, toolchains,
  Ghidra projects, or decompiler output. `.gitignore` covers the usual names; check
  `git status` before committing.
- **Addresses** refer only to `MainUI-354-clean` (SHA-256 `98c85f6c…0c94`).
- **AI tools** may be used, as they were for most of this repository
  ([docs/how-it-was-made.md](docs/how-it-was-made.md)). The same rule applies: review the change
  yourself, and the checks must pass.

## License

By contributing, you agree that your contribution is licensed under the GNU General Public
License v3.0, like the rest of the repository ([LICENSE](LICENSE)).
