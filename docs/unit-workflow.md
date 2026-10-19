# Unit workflow: rewriting one source file byte-exact

MainUI is rebuilt one translation unit (original source file) at a time. A unit is a range in
`docs/tu-map.tsv`; its rewrite is `src/tu_<lo>.cpp`; it is done when `tools/unitcheck.py` prints
`RESULT MATCH`. Proven units so far: `tu_143720` (pinyin search) and `tu_1484d4`
(MultiDBCachedTextMenu); read them as worked examples.

## What a unit looks like in the binary

GNU ld places each object's `.text` first, then its COMDAT sections (template instantiations and
inline functions, `.text._Z*`) in object order, dropping any COMDAT an earlier file already
contributed. So a unit is:

1. **Body**: the file's own functions in source order, ending with the static-init pair
   (`__static_initialization_and_destruction_0`, `_GLOBAL__sub_I_*`) if it has globals with
   constructors.
2. **Tail**: every template/inline function this file used first, in GCC's emission order. It can be
   far larger than the body (`tu_143720`: 1.9 KB body, 18 KB tail) and is never written by hand: it
   appears when the body uses the right types and headers.

Draft boundaries from `tools/tusplit.py` are often wrong at the tail end; `unitcheck` reports the real
end. Neighbours of proven units have one proven boundary, so work outward from them.

## Steps

```sh
export MIYOO_TC=$(scripts/fetch-toolchain.sh)
python3 tools/packet.py --tu tu_<lo>          # briefs in build/packets/ (gitignored)
python3 tools/autoderive.py --tu tu_<lo>      # pattern matches in build/auto/tu_<lo>/matched.cpp
```

1. Read the packets for the body functions (up to the static init, or the first template run).
   Identify the types the tail needs: `std::` containers, tiny-utf8 (`scripts/fetch-tinyutf8.sh`),
   classes from `ghidra/classes.json` (draft layouts: `autoderive.py --headers`, `build/layouts/`).
2. Write `src/tu_<lo>.cpp` with the body functions **in address order**. Shared class declarations
   go in `src/<Class>.h`.
3. Check each function: `python3 tools/matchfn.py src/tu_<lo>.cpp <name|mangled> <addr> --quiet`
   (add `--flags "-O0 -fsigned-char -I<dir>"` for third-party headers).
4. Check the unit: `python3 tools/unitcheck.py src/tu_<lo>.cpp [--verbose]`. Fix body mismatches
   first, then tail mismatches (wrong types or calls), then tail order (below).
   If the next unit fixes the end, pass `--end <hexaddr>` while proving it. The checker enforces
   recorded ends automatically for proven units. A draft's unconstrained `MATCH` can describe a
   shorter layout when pointer templates have identical bytes to earlier instantiations; it does
   not prove the missing tail. Use the required-end walk to settle that ambiguity.
5. Bookkeeping, in the same commit:
   - one line per function in `src/matches.tsv` (`deps` = `-` or `tinyutf8`, `cflags` = `-` or
     extra whitespace-separated compiler flags such as `-fPIC`), then
     `python3 tools/progress.py --matches-md src/MATCHES.md`;
   - the proven range in `UNIT_FIXED` in `tools/tusplit.py` with a one-line reason, then
     `python3 tools/tusplit.py` to regenerate `docs/tu-map.tsv`;
   - `scripts/run_matches.sh` and `scripts/run_units.sh` must pass.
6. Commit as `nukefrenzy <nukefrenzy@users.noreply.github.com>`. Never commit the binary, `build/`,
   or decompiler output.

`cflags` must be identical for every row of a source file. `scripts/run_matches.sh` and
`unitcheck.py` append it to the base flags (`-O0 -fsigned-char`, or `unitcheck --flags`). Keep
`deps` for fetched headers and `cflags` for compiler options; use `-` instead of empty fields.
For a new file not yet registered, probe with `--flags "-O0 -fsigned-char -fPIC"` in both
`matchfn.py` and `unitcheck.py`. Existing relocation masks already cover PIC's GOTPC/GOT32
literal words; no additional masking is needed.

## -O0 idioms learned so far

- Keep statement order, redundant assignments and declaration order exactly: declaration order
  decides stack slots.
- Locals take the callee's exact return type (`size_type`, not `int`): a conversion adds a
  `mov` before the store.
- `if (ok) { ...; return 0; } else { return -1; }` and an early `return -1` compile differently;
  each `return` in its own branch gives its own block. The comparison GCC materialises (`movhi`
  vs `movls`) shows which way the condition was written.
- Calls whose callers look the same can differ only in the tail: `s[i]` and `s.at(i)` produce
  identical calling code in tiny-utf8 but instantiate different functions (unchecked vs checked).
- Return values kept across a destructor call live in `r4`; that is the compiler, not the source.
- Array dimensions affect address calculation even for identical storage: in `tu_14f730`,
  `byte table[2][8]; table[round][i]` adds the table base to the scaled round before adding `i`.
  A flat `table[round * 8 + i]` computes the combined index first and does not match.
- A saved index in `r5` across a call can expose the callee's return type. In the PIC C unit,
  `deviceTx[i * 2] = randomByte()` matches with an `unsigned char` return declaration; an `int`
  declaration changes evaluation order, adds truncation, and removes the saved index.
- A `mov r3, r0` before a byte store does not alone establish C++: the PIC C unit also emits
  it for byte-returning I2C calls. Here both `int` and `unsigned char` return declarations produce
  the same bytes; use the known helper signature. Its loops use C's bottom condition layout.
- Keep switch cases in source order, even when their numeric values look unsorted: ImeWindow's
  input handler emits cases 10, 4, 3, 12, 13, 1, 2, 7, 8, 5, 6, then default. Sorting them preserves
  behavior and total size but moves the blocks and fails the byte comparison.
- Signed loop counters matter even when `vector::size()` makes the comparison unsigned. In
  ImeWindow, changing `int i` to `unsigned int i` reverses `cmp` operands and the materialized
  condition (`movhi` versus `movcc`); some loops also change size.
- Preserve which branch comes first. `if (flag) { A; } else { B; }` and the inverted spelling
  can add Boolean inversion instructions at -O0; disassembly settles the source order even when
  Ghidra prints the equivalent branches in the opposite order.
- Keep header definitions visible when the compiler saw them: an inline `Action` constructor
  containing only vtable/member stores is known not to throw even at -O0. A declaration alone
  makes callers emit extra allocation-cleanup handlers, changing constructors and action bodies.
- Assignment inside a condition matters: `int r; if ((r = action->execute(p)) >= 0) ...` emits a
  materialized sign test in the Wi-Fi drawer. Initializing `r` with the call and then testing it
  separately removes 12 bytes. Keep the variable's scope as well as the statement order.
- Ignored `system()` results are still assigned in the Wi-Fi mode helpers. At -O0, removing
  `rc = system(...)` stores changes every call site even though `rc` is never used.

- Nested switches can share a default exit by fallthrough. In KeySettingWindow's key waiter,
  `switch (event.type) { case KEYUP: switch (sym) { ...; default: break; } default: break; }`
  emits the shared `nop` used by both unmatched event types and unmatched keys. An outer
  `if`, or a `break` between the switches, changes branch targets or adds/removes instructions.
- Chained stores can retain a reload at -O0: `touchX = touchY = -1` reloads `touchY` before
  storing `touchX`; two independent assignments do not match. Conversely, the input-device
  name buffers each require their own zero assignment, rather than a chained byte assignment.

- Separate `continue` statements can have separate trailing `nop` blocks. In the language
  loader, combining the English-file and `.short` filters into one condition preserves most
  instructions but changes branch targets. Keep the two filters and the enclosing file-type test.
- Assignment in a pointer condition also matters: `DIR *d; if ((d = opendir(...)) != NULL)`
  materializes a Boolean test; initializing `d` first and testing it separately removes 16 bytes.

## Tail order

GCC 8 at `-O0` emits COMDAT functions in call-graph node order:

- **Inline non-template functions from headers** get nodes in definition order, so **include
  order** decides theirs. Example: `<map>` before `<algorithm>` (which pulls in `<limits>`), or
  `numeric_limits<>::max()` lands before `_Rb_tree_header`.
- **Templates** get nodes as they are instantiated, breadth-first from the **first odr-use** in the
  source. A copy that is elided still odr-uses the move constructor: `str = T(x)` and `str(x)` place
  `T(T&&)` differently.
- Functions with identical bytes (empty functions, `std::forward`, `numeric_limits::max`) can only
  be told apart by position; `unitcheck` walks the tail strictly in order so they pair correctly.

If `unitcheck` reports displaced sections, use `--verbose`: each tail line shows `obj#k` (our
section index), so the sequence of k shows exactly which functions move. Try include order first,
then where the first use of the displaced function happens.
