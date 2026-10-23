# How this was made

## AI use, and how its output was checked

**Most of this repository was written with AI.** The tool was Anthropic's Claude, working in Claude
Code under a human maintainer's direction. AI wrote most of:

- the source code;
- the names and comments;
- the tools;
- this documentation.

**Every piece of AI output was checked by human review and by tools,** and nothing was accepted on
the AI's word:

- **The human maintainer** set the goals, reviewed changes before they were committed, and ran
  the rebuilt binary on a real Miyoo Mini Plus.
- **The compiled code is checked mechanically.** Every function, every source file and finally
  the whole program are compiled with the original compiler and compared byte for byte with the
  original binary. A rewrite that is wrong in any way that changes the machine code fails.
- **CI runs every check on every push:**
  - all function and file matches;
  - the full byte-identical build;
  - tests for the checking tools;
  - a check that the Ghidra names agree with the source.

**What the checks can't prove.** The byte-identical build proves the code is equivalent to the
original. It doesn't prove the names and comments are right, because names and comments don't
change the machine code. Those are interpretations: AI-drafted and reviewed, and they could
still be wrong in places. The confidence tags in `ghidra/names.tsv` say how each name was
established:

- `confirmed`: read in the code, taken from type information, or tied to a byte-exact function;
- `inferred`, `likely`: strong but indirect evidence;
- `guess`: a placeholder.

If a name or comment looks wrong, please open an issue.

## The steps

### 1. Identify the exact compiler and flags

MainUI's `.comment` section names the compiler, Arm GNU Toolchain 8.2-2018.08, and that exact
package is still published by Arm.

Two libraries are compiled into MainUI, and they were used to find the flags:

- **SQLite 3.30.1.** Its source id string is in the binary. Rebuilding it with `-O1`, `-O2`,
  `-O3` or `-Os` matched no functions, and plain `-O0` matched about half. The rest differed
  only because the original treats `char` as signed. With `-O0 -fsigned-char`, all 2,126
  functions (808,104 bytes) came out identical.
- **cJSON 1.7.10** matched only when compiled as C++: all 104 functions.

After that, there was no guessing about the compiler. Any difference in a rewrite is a
difference in the source.

### 2. Map the program

The binary was imported into Ghidra 12.1.4.

- **Classes.** The C++ type information survives, so all 69 classes, their inheritance and
  their vtables were recovered exactly (`tools/rtti.py`).
- **Function names.** Functions were grouped by subsystem using calls, strings and vtables, then
  named. Names verified by reading the code are marked `confirmed`.
- **Source files.** The original source file boundaries come from how GCC lays out code at
  `-O0` (`tools/tusplit.py`). There are 22 of Miyoo's own files.

### 3. Rewrite each file

Each source file was rewritten function by function, in address order. At `-O0` GCC translates
nearly statement by statement, so the machine code shows the original statement order, local
variables, types and control flow.

- **Function check.** `tools/matchfn.py` compiles a rewrite and compares it with the original.
  Only the words the linker fills in are masked.
- **File check.** `tools/unitcheck.py` checks a whole file, including where the compiler places
  template and inline code, which depends on the order of definitions and includes.
- **Patterns.** Recurring code shapes were collected into patterns (`tools/autoderive.py`). A
  pattern's output was accepted only if it compiled to the exact bytes.

[internals/unit-workflow.md](internals/unit-workflow.md) records the `-O0` idioms that mattered.

### 4. Link the whole program

With all 22 files matching, the last step was the link. The work was:

- reproducing the order of uninitialized data, which ld places partly by symbol-name hash;
- recovering the initial data;
- linking the shared libraries in the original order;
- finding that one file was compiled with a different compiler, GCC 7.3.0, whose version stamp
  also appears in the binary.

[internals/byte-identical-build.md](internals/byte-identical-build.md) explains each of these.
When the rebuilt binary's SHA-256 equaled the original's, the source was complete. The rebuilt
binary then booted and ran on a Miyoo Mini Plus.

### 5. Make it readable

The first byte-exact source used the names a disassembler gives things. A readability pass then
went file by file:

- giving every function, field, constant and global a descriptive name;
- replacing numbers with named constants (buttons, events, window ids, strings, settings);
- adding header and function comments;
- marking the original's bugs and quirks;
- unifying the formatting.

Names and comments don't change the generated code, and every step was re-verified with the full
build.

### 6. Compare with stock

`tools/stockdiff.py` compares Onion's MainUI with Miyoo's stock build function by function. The
results are in [onion-changes.md](onion-changes.md).

## Why the source looks the way it does

The goal is the exact original program, so the source keeps some things a person writing fresh
code wouldn't:

- **The original's statement order and redundant steps**, because at `-O0` they show up in the
  machine code.
- **Its bugs.** Each one is marked `Original bug` or `Original quirk`.
- **Inline assembly where Onion patched the binary after compiling.**

Everything else aims to read like ordinary hand-written C++.
