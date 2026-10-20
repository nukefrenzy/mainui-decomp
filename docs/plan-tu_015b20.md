# Plan: the first file, tu_015b20

The last unproven unit, and the largest: 0x15b20–0x49c3c, 212 KB, about 46% of Miyoo's code. It holds
the window and menu classes, the actions, the window loop and `createMenu`. Read `docs/unit-workflow.md`
first; this file only adds what is different about a unit this size.

## Shape

| Part | Range | Functions | Bytes |
| --- | --- | --- | --- |
| Body (own functions, source order) | 0x15b20–0x39240 | 303 | 144,704 |
| Static-init pair (end of body) | 0x39228–0x39240 | 2 | |
| Tail (template + inline) | 0x39240–0x49c3c | 796 | 68,092 |

- 553 tail functions are known `std::` template shapes and come free. The other ~240 are inline class
  methods (getters, empty virtuals, inline ctors/dtors) from the class headers.
- Already matched: `keyEventName`, `writeCmdToRun`, the `TrimusWindow` and `MenuWindow` ctors/dtors
  (body), and `TrimusWindow::animateIn/animateOut/getId` (tail). See "Skeleton findings" below.

## Structure: one unit, several part files

The unit must compile as one translation unit, but sessions need separate files to work in parallel.
So:

```
src/tu_015b20.h          shared declarations for this file: classes, globals, prototypes
src/tu_015b20.cpp        includes (original order) + tu_015b20.h + the parts below, in order
src/tu_015b20/p01.inc    one part per address range, functions in address order
src/tu_015b20/p02.inc    ...
```

`#include` is textual, so the parts compile exactly like one file: function order, first-use order
and the tail are unchanged. Each session owns one `.inc` and adds only the declarations it needs to
the shared header.

**Order-sensitive pieces, handled once in the skeleton:**

- **Globals with constructors** must be defined in their original order: the static-init function
  at 0x39228 (and its `__aeabi_atexit` calls) shows that order. Define them all in `tu_015b20.cpp`
  before the parts, unless the static init shows one defined between functions.
- **Include order** decides the order of inline header functions in the tail (`unit-workflow.md`,
  tail order). This file's tail runs through every class header, so its order reveals the original
  header layout. Read it early: `--verbose` from `unitcheck` on even a partial file lists the inline
  methods in emission order.
- **Shared headers** (`src/TrimusWindow.h`, `MenuWindow.h`, `ListMenu.h`, `Action.h`,
  `MultiDBCachedTextMenu.h`, ...) are already used by proven units. Any change must keep
  `scripts/run_units.sh` passing. Classes whose key function is defined here get their vtable here.

## Skeleton findings (phase 1, done)

**Layout.** `src/tu_015b20.cpp` includes `<iostream>`, `<atomic>`, `<vector>`, `src/tu_015b20.h`,
defines the one constructed global, then includes `src/tu_015b20/p01.inc` … `p11.inc` in order.
Each part file starts with its range; gaps not yet written are marked `// 0x…-0x…: not written yet.`
The part ranges above are unchanged.

**The gap 0x15b20–0x15d00** is one function, missed by the Ghidra export: `keyEventName(int)`
(480 bytes), a switch returning `"key_none"` … `"key_hotkey4"` for 0–15, `"event_battery"`,
`"event_hotplug"`, `"event_notify"` for 18–20, `"key_max_key"` for 45 and `"event_unkown"` otherwise.
Nothing calls it (no `bl`, no stored address). It matches with a plain `int` parameter and is now in
p01, `ghidra/names.tsv` and `tools/manual_names.py`. `render_SDL_Flip` at 0x15d00 is the next function.

**Globals with constructors.** The static init (0x3919c, called by `_GLOBAL__sub_I` at 0x39228)
constructs only two objects:
1. `std::ios_base::Init` at 0x180858 (so the file includes `<iostream>`);
2. the window queue at 0x17ff3c, a `std::vector<TrimusWindow *>` (ctor 0x3b130, dtor 0x49be0, `size()`
   0x3b790; used by `flushPendingWindows` 0x36cb8 and around 0x370e4). The skeleton calls it
   `g_pendingWindows` and defines it before the parts. Its real position among the functions is
   unknown; it only affects where the vector templates land in the tail, so settle it in phase 3.

**Tail inline order and include order.** Inline non-template methods come first in the tail, in
definition order, from 0x39240 to about 0x3b130, where the `std::` template instantiations begin
(starting with the global vector's ctor). The sequence:

| From | What | Suggests |
| --- | --- | --- |
| 0x39240 | six small leaf functions (0x39328 is missing from names.tsv), then 0x3934c `std::operator&(memory_order, __memory_order_modifier)` (48 B, same as a probe) and 0x3937c `std::atomic<bool>::load` (96 B; a default-order probe gives 100 B, unexplored) | `<atomic>` (or a header pulling it in) comes first |
| 0x393dc | leaf ctor/dtor, dtor, deleting dtor of a memberless polymorphic class | a worker base such as `BGWorker` |
| 0x39478 | `TrimusWindow::animateIn`, `animateOut`, `getId` (0x394c8), `getTitle` (0x394f0, calls `languageString`) | `TrimusWindow.h`; define `getId` before `getTitle` when both become inline |
| 0x39520 | dtor (string member) + deleting dtor; getters for +0x0c/+0x10/+0x14, a getter returning a `std::string` copy, a leaf | `ListMenu.h` (current item, page start/end) |
| 0x39678 | six `FileBrowserMenu` inline virtuals | |
| 0x39750 | ctor/dtor/deleting-dtor triple (string member) | an item or action class |
| 0x397fc | two inline virtuals (slots 3, 4), then inline ctor, dtor, deleting dtor of: `AppAction`, `EmuAction`, `GeneralAction`, unnamed (0x39c8c), `DeleteRomAction`, `ClearFavoriteAction`, `ClearRecentAction`, unnamed (0x39f90), `AddFavoriteAction`, `RemoveFavoriteAction`, `FileAction` (dtors only), `SearchAction`, `GlobalSearchAction`, `PopupSearchIME`, `PopupDialogAction`, `PopupProgressAction` | the action classes' header, in this declaration order (not body order) |
| 0x3a678 | `TextMenu` vfunc8, `DBCacheWorker` ctor/dtors, `DBCachedTextMenu` vfunc8, `IconMenu` vfunc8, `DeviceInfoMenu::onSelect`, ... | menu headers |
| 0x3ab64 | five `FloatWindow` and five `ADCInfoWindow` inline virtuals | window headers |
| 0x3af38 | `LoadSurfaceWorker` ctor/dtors, then a few more until the templates | |

So the original order is roughly: `<atomic>` → worker base → `TrimusWindow.h` → `ListMenu.h` → file
browser → actions → menus → float/ADC windows → `LoadSurfaceWorker`. Our current headers do not
follow this yet (`MenuWindow.h` includes `TrimusWindow.h` before `MenuHelpers.h`); the parts can
ignore it, phase 3 arranges the includes.

**Moved matches.** `keyEventName` and `writeCmdToRun` are in p01, the `TrimusWindow` ctor/dtor in p03,
the `MenuWindow` ctor and both dtors in p09; `src/MenuWindow.cpp` and `src/writeCmdToRun.cpp` are gone.
`animateIn`/`animateOut` are now inline in `TrimusWindow.h` (emitted here through the vtable; the
other units still pass, their copies are kept from earlier). `getId` must also become inline, but an
unused inline function is not emitted and its callers (0x35054 in p10; 0x37004, 0x372b8 in p11) are not
written, so it stays matched on its own in `src/TrimusWindow.cpp` until then. Whoever writes the first
caller moves it into the header and points its `matches.tsv` row at `tu_015b20.cpp`.

**Body check.** `python3 tools/unitcheck.py src/tu_015b20.cpp --body-only` now reports
`PREFIX 0x15b20-0x15d00 matches: 1 whole functions`, next ours `writeCmdToRun`, original
`render_SDL_Flip_15d00`. `scripts/run_units.sh` lists the unit in `PARTIAL` and runs it body-only:
a short prefix is reported, only a compile error fails. Remove it from `PARTIAL` in phase 3.

## p02 findings (0x18c54–0x1a44c, done)

All 16 functions match with `matchfn` (6,136 bytes, exactly the range). Rebased on p01:
`unitcheck --body-only` now reports the prefix 0x15b20–0x1a44c (47 whole functions); the next
function is `DeleteRomAction::execute` (p03).

**Functions.** `AppAction::execute`, `AppAction::progress` (slot 3), then four render helpers that
are not actions: `digitCount` 0x18dd4, `animateBusy` 0x18e3c, `busyScreenThread` 0x193a8,
`loadingScreenThread` 0x198a8; then `execute` of RemoveFavorite, Search, GlobalSearch,
PopupSearchIME, PopupDialog, PopupProgress, General, ClearFavorite, ClearRecent, AddFavorite.

**How actions execute.** `Action` (`src/Action.h`) is `{vtable, int result}`; `result` (+4) is
really a constructor argument whose meaning varies (AppAction: passed to the launcher;
AddFavoriteAction: the window id it was opened from). Slots: 2 `execute(void *param)`,
3 `progress(int *, int *)`, 4 `finished()`. Slots 3/4 are inline in `Action` itself
(0x397fc returns -1, 0x39828 returns 0): every action here shares them, except `AppAction`
(slot 3 opens the detail window, slot 4 returns 1). Return codes seen (matching the window
`handleInput` codes in `TrimusWindow.h`):

| Action | Fields | execute does | returns |
| --- | --- | --- | --- |
| `AppAction(AppInfo*, bool detail, int)` | app +8, detail +0xc | `launchApp_buildCommand(app, result)` | its result |
| `AppAction::progress(item, arg)` | | if `detail`: `GameDetailWindow(0x54, 8, app label, item label, launch, iconSelected, arg, result, item->favorite)`, `onShow`, push on `g_windowStack` | 0, else -1 |
| `RemoveFavoriteAction(TextItem*, int)` | item +8 | `item->removed` (+0x1e) = true | 1 |
| `SearchAction(int, int id, void *emu)` | +8 unused, id +0xc, emu +0x10 | `MenuWindow(0x99, id, emu)` (ctor 0x2e424), `setPath(param string)`, `onShow`, push | -1 |
| `GlobalSearchAction()` | | same with menu 0x18, no emu | -1 |
| `PopupSearchIME(int, int, void*)` | as SearchAction | `pushWindow(ImeWindow(0x99, new SearchAction(...), 0x99))` | 4 |
| `PopupDialogAction(int, int, bool, Action*)` | title +8, text +0xc, flag +0x10, action +0x14 | `pushWindow(DialogWindow(title, text, flag, action))` (0x34 bytes) | 4 |
| `PopupProgressAction(int, Action*)` | +8, +0xc | `ProgressDialog(title, action)` (0x3c), `onShow`, `pushWindow` | 4 |
| `GeneralAction(int, int (*)(void*), void*)` | fn +8, arg +0xc, verbose +0x10 | `fn(arg)` | fn's result, -1 if none |
| `ClearFavoriteAction(ListMenu*, int)` | menu +8 | `unlink favourite.json`; on success `menu->vfunc13()` (reload), `sync` | 1 |
| `ClearRecentAction(ListMenu*, int)` | menu +8 | `clearRecent(&g_recentGames)`, `menu->vfunc13()` | 1 |
| `AddFavoriteAction(TextItem*, int win)` | item +8 | win must be 3 (apps), 5, 0x11 or 0x18 (ROM lists); appends `{label, launch, type, rompath}` (apps: no rompath, from `AppAction::app`; ROMs: from `EmuAction`, skipped if already listed via `isFavourite` 0x1b210) to favourite.json, sets `item->favorite` (+0x1d) | 1 |

In short: 4 after `pushWindow` (popup queued), 1 after a list edit, -1 when the new window went
straight onto `g_windowStack` or nothing ran. How callers act on these is for p06/p07 (`IconMenu::onSelect`). Two unnamed abstract bases (vtables with a pure slot 2):
`MenuAction` (ctor 0x39c8c, `{ListMenu *menu, int 0}`) for DeleteRom/ClearFavorite/ClearRecent,
and `ItemAction` (ctor 0x39f90, `{TextItem *item}`) for Add/RemoveFavorite.

**Busy screens.** `busyScreenThread` and `loadingScreenThread` are thread bodies; they animate
until `g_busy` (0x181908) clears. `busyScreenThread` is passed to `SDL_CreateThread` from
`IconMenu::onSelect` (0x277c0), `createRomListMenu` (0x37700) and three sites in `createMenu`
(0x37e6c-0x37f84); no `movw` reference to `loadingScreenThread` was found. `busyScreenThread` takes `{const char *title, int type}`
("sdata"). The progress counter `g_progressCur/Total` (0x18190c/0x181910) is shown as `"%*d/%d"`.

**-O0 idioms.** `g_busy` and the progress counters are `volatile int`: a plain `int` drops the
materialized `movne/moveq/uxtb` test in `while (g_busy)` and `if (showCount && total > 0)`. In
`busyScreenThread` the two popup rectangles must be declared at function scope (they get the slots
before the dimming block's rectangles), and `int x;` is declared before `sdata` but assigned after it.

**Header changes.** `Action.h` slots 3/4 inline and the named `TextItem` bytes (made identically by
p01). `MenuWindow.h`: the 0x2e424 ctor `(int, int, void *emuEntry)` and inline `setPath` (0x3ab30,
after p01's `setStrB4` 0x3aafc). `tu_015b20.h`: the p02 classes in tail declaration order; the p10
windows, `EmuAction` and `GeneralAction` are p01's declarations, and p02 uses p01's names for the
status-bar and text helpers and globals (`g_launchCmd` is also the counter text buffer).

## Checking a part on its own

`matchfn` works per function in a partial file, as long as it compiles (declare what other parts
define). To prove a part's whole range, compile the skeleton with only parts 1..N and compare the
body prefix. `unitcheck` already compares `.text` from the unit start. It needs a small addition,
`--body-only` (stop after `.text`, report how far it matched), so that a growing prefix can be checked
before the tail exists. (Added: it counts whole functions only and names the next function on both sides.)

## Parts

Boundaries fall at class changes; sizes are body bytes. Labels come from `ghidra/names.tsv` and are
partly guesses.

| Part | Range | Bytes | Contents |
| --- | --- | --- | --- |
| p01 | 0x15b20–0x18c54 | 12.6 KB | runtime helpers: render/audio/power, `handleGlobalInput`, launch, `writeCmdToRun`; `EmuAction` |
| p02 | 0x18c54–0x1a44c | 6.1 KB | actions: App, RemoveFavorite, Search, GlobalSearch, Popup*, General, Clear*, AddFavorite |
| p03 | 0x1a44c–0x1d9d8 | 13.7 KB | `DeleteRomAction`, helpers, `TrimusWindow` |
| p04 | 0x1d9d8–0x20500 | 11.0 KB | `ListMenu` base slots, `FileBrowserMenu` (first part), `DBCachedTextMenu` |
| p05 | 0x20500–0x23548 | 12.4 KB | `TextMenu` |
| p06 | 0x23548–0x2849c | 20.3 KB | `DBCacheWorker`, `DBCachedTextMenu`, `IconMenuItem`, `IconMenu`, `SettingMenuItem` |
| p07 | 0x2849c–0x2b788 | 13.0 KB | `SettingMenu`, `DeviceInfoMenu` |
| p08 | 0x2b788–0x2e424 | 11.4 KB | `ColorMenu`, `LanguageMenu`, `ThemeMenu`, `FileAction`, `FileBrowserMenu`, `LoadSurfaceWorker` |
| p09 | 0x2e424–0x3225c | 15.9 KB | `MenuWindow`, `FloatWindow` |
| p10 | 0x3225c–0x36838 | 18.3 KB | `PopUpWindow`, `NotifyWindow`, `CountDownWindow`, `DialogWindow`, `ProgressDialog`, `GameDetailWindow` |
| p11 | 0x36838–0x39240 | 10.8 KB | `ADCInfoWindow`, window stack and loop, `createRootWindow`, `createMenu`; then the static-init pair |

p06 is the "item and action wiring" core (how an item becomes an `Action`), together with p02 and p07.
When those parts are done, update `docs/program-map.md` (items, actions, settings ids and handlers;
the five unconstructed settings entries).

## Phases

1. **Skeleton** (one session):
   - disassemble 0x15b20–0x15d00;
   - read the static init at 0x39228 for the global order;
   - read the tail's inline-method order for the header layout;
   - create `tu_015b20.h`, `tu_015b20.cpp` and empty part files;
   - move the existing matched functions into their parts;
   - add `unitcheck --body-only`.

   Done when the skeleton compiles and `--body-only` matches up to the first unwritten function.
2. **Parts** (one session each, at most two or three at a time, each in its own worktree):
   - write the part's functions in address order and `matchfn` each;
   - check the body prefix once the parts before it are merged.

   Merge parts into `main` one at a time and resolve header conflicts there. Earlier parts first is
   simplest, but any part can be matched function by function in parallel.
3. **Integration** (one session): full `unitcheck`, fix tail order (include order first), and the
   bookkeeping:
   - `UNIT_FIXED`;
   - `matches.tsv` / `MATCHES.md`;
   - `docs/status.md` (the project is then 100% of Miyoo code);
   - the program-map updates above.

## Brief for a part session

> Read CLAUDE.md, docs/status.md, docs/unit-workflow.md and docs/plan-tu_015b20.md. Set up the
> environment, confirm scripts/run_matches.sh and scripts/run_units.sh pass. Write part pNN
> (range from the plan) in src/tu_015b20/pNN.inc, functions in address order, adding only needed
> declarations to src/tu_015b20.h. Every function must MATCH with tools/matchfn.py; check the body
> prefix with tools/unitcheck.py --body-only if the earlier parts are in main. Keep existing
> matches and units passing. Work on a new branch, commit as nukefrenzy, don't push; stop and
> report what matched, what didn't, and header changes. Ghidra exports are in ~/mainui-exports/
> (use tools/packet.py); never read MainUI-354-clean_named.c whole.
