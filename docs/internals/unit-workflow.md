# Unit workflow: rewriting one source file byte-exact

MainUI is rebuilt one translation unit (original source file) at a time. A unit is a range in
`docs/tu-map.tsv`; its rewrite is `src/tu_<lo>.cpp`; it is done when `tools/unitcheck.py` prints
`RESULT MATCH`. Worked examples include `tu_143720` (pinyin search), `tu_1484d4`
(MultiDBCachedTextMenu), and `tu_11cdc8` (shared configuration). All 22 units are proven;
`scripts/run_units.sh` checks them.

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
end. Neighbors of proven units have one proven boundary, so work outward from them.

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
  `deviceTxData[i * 2] = randomByte()` matches with an `unsigned char` return declaration; an `int`
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

- Operand order can keep an extra saved register across a call. The worker queue needs
  `workers.size() >= limit`; `limit <= workers.size()` saves `limit` across `size()` and changes
  the prologue and stack frame.
- Byte pointer types matter even when every value is ultimately stored with `strb`. The zlib
  decoder uses unsigned input/history bytes and signed output bytes; its unsigned byte locals
  produce sign-extending loads when stored through the signed output pointer.
- Preserve original fallthrough from a non-void function when the evidence establishes it.
  The decoder's assertions identify `int parse_header(zbuf*)`, but successful switch cases
  reach the end without returning. Its ignored result and compiler warning are documented in
  source; adding a return changes bytes.

- Pointer value categories affect vector calls: `insert(pos, (TextItem*)item)` and
  `push_back((TextItem*)item)` create pointer temporaries and select rvalue overloads. Passing
  the pointer variable directly changes the caller frame and generated tail.
- GCC still folds some C string builtins at -O0. The browser's `strlen(name) != 0` becomes
  a first-byte test through the array address; `strncmp(name, ".", 1)` becomes a difference of
  unsigned bytes, including a load from the literal. Replacing either with `name[0]` changes code.
- Nested file-type and Boolean filter tests preserve direct branches in the browser loader;
  combining them with `&&` materializes an extra Boolean block and adds 20 bytes.

- Four-byte aggregate alignment can decide copy instructions and evaluation order. Theme
  defaults match with whole aligned `ThemeColor` union copies from external constants. Scalar
  member assignment reverses the loads; a byte-aligned `SDL_Color` copy adds address setup.
  Defining the color constants in this same file also folds their values at -O0 and removes
  36 bytes, so retain the external declarations seen by the original compiler.

- A zero-offset base conversion can still add a move before an inherited method call. In the
  popup, TextMenu* is required: TextMenuDrawer* removes moves around inherited setRowHeight,
  draw and input calls, while the redeclared count call stays unchanged. The detail parent menu
  is TextMenuDrawer*, whose inherited previous/next calls show the same conversion.
- Floating-point literal suffixes matter at -O0. GameDetailWindow eases with double `0.05` and
  `1.0`, narrows the result to float, then multiplies by `640.0f`; unsuffixed `640.0` changes the
  VFP instructions. Keep integer rectangle arithmetic on the original x, not a truncated short
  SDL_Rect field: reloading the field changes both instructions and, for out-of-range values,
  behavior.
- A final switch case can fall through to `default: break`, producing a shared NOP block.
  GameDetailWindow's last case needs this form; an explicit break in that case changes the
  NOP/branch layout. A return after an if/else can likewise retain a branch-end NOP that an
  early return inside the branch removes (DialogWindow::draw).

- `std::atomic<bool>`'s implicit conversion to bool takes only `this`; `load()` passes the default
  memory-order argument as well. The window loop uses `*running`, which emits the 96-byte inline
  conversion, rather than the similar 100-byte load wrapper.
- A known post-compilation NOP patch must be checked literally. The matcher's BL/relocation mask
  can otherwise accept a restored call in place of a disabled instruction. createMenu scopes an
  assembler macro around its twelve patched calls, retaining GCC's argument/cleanup code and
  asserting the patch count; each resulting word is also compared without the call mask.

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

- A predecrement in a loop condition can materialize a Boolean that a separate decrement
  statement does not: the scaler needs `do { ... } while (--remaining != 0)`. Moving
  `--remaining` into the body removes 16 bytes per loop despite preserving behavior.
- The first use of an inline/template function affects the tail even when body bytes match.
  The configuration unit's list scans need `while (it != end) { ... *it ...; it++; }`:
  a `for` increment instantiates `operator++` before `operator*` and swaps their tail order.
  Its global recent-list declaration also follows the first local recent-item construction;
  moving it ahead changes the order of generated constructors and destructors.

- An apparently large exported function can contain an unexported function start. The main
  program tail ends with a 36-byte destroy helper at 0x04c3f4; the next 180 bytes are a separate
  circular-list access function at 0x04c418. The unique-insert wrapper at 0x04c76c likewise ends
  at 0x04c7a8, where a 120-byte count function begins. Check prologues and call/return boundaries
  before trusting sizes derived solely from adjacent Ghidra-exported starts.

- Hardware keyboard mapping needs an enum parameter whose values promote to int; a plain int
  parameter adds 84 bytes of switch reloads at `-O0`. Keep aggregate working variables at their
  original scope: a loop-local SDL_Color can add an exception-cleanup path even when it is POD.
- Inline virtual declarations may precede their definitions. Defining the hardware no-op virtuals
  and destructors out of class, in original emission order, reproduces the 27-function tail.

- Match the concrete SDL event type used at each site: a 16-byte user-event struct, 20-byte
  event union and full keyboard-event union can carry the same user fields but give different
  local offsets (`FileAction::execute` 0x2d1bc versus `LoadSurfaceWorker::finished` 0x2e3ec).
- A pointer-to-int argument cast can add `mov r3, r0` before copying a call result to a saved
  register, even though both types are 32 bits. The NotifyWindow button-label parameter must
  be `const char*` for its p08 callers to match; an integer placeholder plus cast is not equivalent.

- A by-value `std::string` parameter has the same indirect ARM ABI as a string reference/pointer
  inside a callee. A matching callee body alone does not settle its signature: caller-side copy
  construction and destruction do (`MenuWindow::onShow`, `LoadSurfaceWorker` construction).
- Returning a virtual call's result directly can differ from storing it in the common result
  local and returning later. MenuWindow's progress dispatch needs the direct return; the latter
  adds a store and moves branches even though the result is identical.

- Standard-header order can control template first use as well as inline emission. In the first
  UI unit, `<string>` precedes the UI declarations and `<iostream>` follows them; system-error
  string concatenation otherwise jumps ahead of the vector destructor/size templates.
- Masked caller matches cannot distinguish `vector::at()` from `operator[]`, or two pointer
  vector specializations. Trace original callee destinations and match the complete tail:
  shared TextMenu/IconMenu row helpers expose their common `vector<void*>`, while `at()` adds
  a range-check wrapper and changes later template order. Keep rvalue/const-reference append
  overloads as well as element types consistent.
- An inline special member can be declared in its class and defined later in the same TU to
  preserve tail order. LoadSurfaceWorker's definitions follow p08, after the first implicit
  RomListPosition/xml_document/DeviceInfoItem special-member uses. No class layout change or
  artificial template instantiation is needed.

- A matching callee can hide a wrong public signedness. The scaler's callers need unsigned
  dimensions/stride to retain their conversion moves. The implementation also needs unsigned
  counters/division to call __aeabi_uidiv, while retaining its signed row-stride interpretation.
  A signed implementation passed masked checks but selected __aeabi_idiv. Casting only the
  divisor to unsigned added a move at each division; declaring cx/cy unsigned preserves the
  original direct loads into r1 and the 376-byte body. Conversely, panel timing's uint16 operands
  promote to int: explicit unsigned casts selected the wrong unsigned division helper there.
  Changing the public caller prototype to signed had removed 24 bytes from the first unit.
- C and C++ units must share explicit linkage declarations. A masked function proof can pass
  while a C caller references a different symbol from its C++ definition. Use common C-compatible
  headers for the shared entry points (process lookup and streaming zlib here), preserving the
  original code rather than introducing aliases or wrapper functions.

- A const global definition can trigger constant folding even at -O0, including when placed
  after its consumers in the same file. The three theme colors require SDL_Color struct
  definitions and byte-member assignments in their original theme unit. Union definitions
  there removed 36 bytes of original global loads; moving them to another TU hid the type
  error until read-only data was checked. Verify data initializers through src/data.tsv and linkaudit, including
  relocation targets for string tables; a code-only match cannot prove their contents.
- A shared address can expose a wrong recovered field type. IconMenuItem's +8/+12 values are
  borrowed background surfaces, not x/y coordinates; their globals are g_gameFocused,
  g_gameNormal and g_raRow. Correcting the declarations and callers retains both constructor
  matches and the complete first-unit proof.

### Check the linked imports and runtime wrappers

A masked MATCH cannot distinguish a call to the wrong same-shaped overload or libc wrapper.
The executable import audit caught `openThemeFont` using const string::at instead of non-const
(original PLT 0x14e48), and `zipOpen` using stat instead of lstat (original wrapper 0x1518a0 at
call 0x14d8dc). Correcting argument constness and the wrapper call preserved every code byte
outside relocations. Compare original destinations and final per-symbol versions; do not accept
a new import just because linking succeeds. These are linkage/type corrections, not new -O0
instruction patterns. Actual COMDAT selection also needs checking after full linking, beyond
per-unit backtracking; see the example below.

### Equal instructions do not establish COMDAT identity

The linked tu_11cdc8 tail was 556 bytes short despite a standalone whole-unit MATCH. Its independent
global at 0x182170 had been declared `std::list<RecentItem>`; it is `std::list<int>`. The mistaken
type reused COMDAT constructor/destructor names from tu_049c3c's recent-games list, so ld discarded
sections that the unit walk could assign to byte-identical code later in the binary. Correcting
the local global type restores the original tail and all later body addresses without padding.
A valid walk proves a byte arrangement exists, not that actual symbol identities select it.

Use `linkaudit.py --require-link` alongside unitcheck: the final link keeps static relocations
with `--emit-relocs` and compares actual .text at its original address/size, using the same
relocation/BL/BLX masks. It also compares raw ARM call/patch words separately, and now fails on
any difference. The retained code and direct call words match; data addresses and indirect targets
remain a separate proof. This is a
cross-object type/identity issue, not a new -O0 instruction idiom.

### Preserve post-build NOPs and inspect wrapper calls

Eight further target patches disable one shadow blit, six Wi-Fi background blit/fill calls and
one PWM-period write. Simply removing the source calls loses argument setup and code size.
Use function-scoped GAS bl interception as for createMenu: emit literal 0xe320f000 at only the
known offsets, omit their relocation, assert the exact count, then purge the macro. No new masks
are needed, and the NOP words themselves now participate in the code proof.

Equal caller instructions can still select different template operations. FileBrowserCache's two
row lookups require vector::at rather than operator[]; switching to at restores exception-table
sizes too. LruCache's insertion wrapper calls begin, proving push_front rather than push_back
for both put and cache hits. Inspect calls inside generated wrappers as well as handwritten bodies;
actual linked raw-call comparison catches these differences. No manual template instantiation or
linker alias is needed.

### Prove literal identity and virtual bindings with read-only data

Instruction matches do not establish literal order or indirect targets. The IME switch
needed its original source case order even though numeric case order produced matching
instructions. WifiMenuDrawer needed its renderer in existing slot 7 and pure-virtual event
slot 15; only raw vtable-pointer comparison established those bindings.

Strings patched in place retain allocation extents and sometimes distinct identities even
when their text becomes identical. Use readable named arrays with recovered bounds for
those slots; ordinary literal pooling can shrink the section or merge separate objects.
Preserve first-use order and deferred function-name strings. Class declaration/include
order also controls vtable and RTTI emission. Scope assertion diagnostic filenames when
the original SDK path is embedded, without modifying the fetched dependency.

The full linked .rodata comparison now checks every byte, including relocated pointers,
without masks. Keep this check alongside unit and linked-text proofs; see
[rodata-recovery.md](rodata-recovery.md) for examples and shared-header changes.

### Recover initialized storage and check unwind section order

A consumed prefix need not be a whole global object. The 116-byte panel prefix extends to
188 bytes in the SDK layout, followed by a separate 341-byte command stream and 76-byte
MIPI configuration. Preserve the real pointer relocation and natural alignment; compare
all initialized bytes instead of adding padding to close a section-size deficit.

GCC can establish an ARM unwind section before emitting its associated template code.
In the Wi-Fi unit, the initial drawable definitions precede the first body function, and
menu/action definitions follow it. Splitting the shared item header and including the full
menu header after wifiMode reproduces this ordering. Other consumers still see all inline
definitions in their original order. Deferring selected inline definitions without preserving
other consumers can alter COMDAT retention even when an isolated link probe passes. Require
the full link, raw vtables and all unit checks before accepting an unwind-order correction.

Local zero definitions can use assembler .comm and be allocated after public objects.
Check actual symbol addresses, not only the apparent source order or all-zero BSS bytes.
Do not infer an unknown allocation from the size deficit alone. Details and unresolved
extents: [writable-data-recovery.md](writable-data-recovery.md).

The first UI unit's iostream object illustrates local .comm ordering: public definitions
for framebuffer/private state moved ahead of it, whereas TU-local definitions place them
after it with the original relative offsets. This fixes order without settling an earlier
four-byte allocation gap. A next-object address bounds an interval; it does not establish
that every unreferenced byte belongs to the preceding array. See [BSS layout](byte-identical-build.md#bss-layout).
