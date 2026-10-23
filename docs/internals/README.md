# Internals

Technical records of how the byte-identical build was reached. You don't need these to read,
build or modify the source. They are for anyone who wants to check the method in detail, or
to take a similar project on. They record the state when each step was finished, so they
mention dates and intermediate numbers.

| Document | Covers |
| --- | --- |
| [byte-identical-build.md](byte-identical-build.md) | The final step: what the build does, the last gaps closed (uninitialized-data order, GOT order, the GCC 7.3.0 unit), placeholders for unreferenced storage, and the reconstructed names chosen for link order |
| [link-audit.md](link-audit.md) | What `tools/linkaudit.py` checks, section by section, and how to read its report |
| [unit-workflow.md](unit-workflow.md) | How one source file was rewritten to match: the `-O0` idioms, the template "tail order" rule, and checking a unit |
| [rodata-recovery.md](rodata-recovery.md) | Making the read-only data (strings, tables, vtables, type information) identical |
| [data-recovery.md](data-recovery.md) | Recovering the globals the code refers to, and their initial values (`src/data.tsv`) |
| [writable-data-recovery.md](writable-data-recovery.md) | Making the initialized data and the exception-unwinding tables identical |

The reader-facing documentation is one level up:

- [building.md](../building.md)
- [source-guide.md](../source-guide.md)
- [behavior.md](../behavior.md)
- [modifying.md](../modifying.md)
- [onion-changes.md](../onion-changes.md)
- [tools.md](../tools.md)
- [how-it-was-made.md](../how-it-was-made.md)
