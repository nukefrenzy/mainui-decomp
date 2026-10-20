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
and the tail are unchanged. Each session owns one `.inc`; shared declarations are already in the
headers (see "Shared declarations" below).

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

## p03 findings (0x1a44c–0x1d9d8, done)

All 27 functions match with `matchfn` (13,708 bytes, the whole range); `unitcheck --body-only` now reports
the prefix 0x15b20–0x1d9d8 (75 whole functions). Next is ListMenu slot 2 at 0x1d9d8 (p04).

**Functions.** `DeleteRomAction::execute` (calls `menu->vfunc12(field0c)`); favourites: `power_label_1a4bc`
rewrites a favourites file from a `TextItem` list, `launch_label_1a75c` removes the lines whose label starts
with a given label, `buildMenu_1a924` builds the favourites list (type 3 apps, 5/17 ROMs; rewrites the file
when `addUnique` rejects duplicates), `isFavourite` searches `"rompath"` in a buffer or file; `buildMenu_1b488`
(recent games), `buildMenu_1bbfc` (apps, row height 90); `display_getlcd_1bea4` (MI_DISP LCD luma/hue/
saturation/contrast); `initAudioSettings` (keymap, volume, backlight, music volume); `refreshMenu_1c0ac`
(deletes removed favourites, menu id 2); `restoreWindowState`/`saveWindowState` (`/tmp/state.json`,
`{"list": [{title, type, ...}]}`; type 8 restores a `GameDetailWindow`, others a `MenuWindow`); ROM-list
positions (`/appconfigs/romwinidx.json`, keys `rompath`, `pos`, `start`, `end`); `loadThumbnail` (RGBA
copy, scaled to width w); `makeThumbPath`; `createSurface`; `TrimusWindow` ctor, dtors, `dump`; `ListMenu`
ctor and `setRect`.

**-O0 idioms.**
- GCC's C++ front end marks a function nothrow once its whole body has no throwing call, and callers anywhere
  in the unit (also earlier ones) then drop the cleanup pad of `new T(...)`. `restoreWindowState` only matches
  with `GameDetailWindow(int, int)` (0x34a14, member stores only) defined in the unit, so p03 wrote it in p10.
- Nested `new` keeps the inner object in a callee-saved register: `new TextItem(label, 0, new AppAction(...), true)`;
  a named local for the action adds a stack slot.
- `return img;` (implicit `std::string` from `const char *`) changes argument evaluation for the whole function
  (`img` copied through a register for `strstr`, `printf` and every `std::string(img)`); `return std::string(img)`
  does not.
- `std::string::size_type` positions: `int` adds conversion moves before `substr`.
- A `do { ... break; ... } while (0)` gives each `break` its own `nop` block (state/position loaders).
- Locals assigned more than once get the first stack slots regardless of declaration order.

**Shared declaration changes.**
- `tu_015b20.h`: p03 prototypes retyped (names kept): `power_label_1a4bc(const char *, std::vector<TextItem *> *)`,
  `launch_label_1a75c(const char *, const char *)`, `buildMenu_1b488(std::list<RecentItem> *)`,
  `buildMenu_1bbfc(std::vector<AppInfo> *)`, `isFavourite(char *buf, FILE *, const char *)`,
  `setRomListPosition(..., const std::string &path, ...)`, `saveRomListPositions(const std::vector<...> *)`,
  `makeThumbPath(std::string *game, const char *img)`.
- 0x20a3c (p05's `TextMenu::addUnique`, called `hasItem` while p03 was written) appends the item unless its label is listed, true if added.
- `TrimusWindow.h`: `titleLabelId` and `id` are public (`saveWindowState` reads them directly).
- `SDLTypes.h`: full `SDL_PixelFormat` (moved from `tu_11cdc8.cpp`) and `SDL_Surface::pitch`/`pixels`.
- `scalePixels` (0x11cdc8) is declared here with `unsigned` sizes (conversion moves); its tu_11cdc8 definition
  matches with `int`, so the two prototypes mangle differently.
- p10.inc: `GameDetailWindow(int, int)` 0x34a14, see above.

## p05 findings (0x20500–0x23548, done)

All 18 functions match with `matchfn` (12,360 bytes, exactly the range). p03/p04 were not merged, so
the body prefix could not reach p05; the function sizes add up to the whole range with no gap.

**TextMenu** (0x20500–0x20b1c): ctor, dtor, deleting dtor; slot 18 `vfunc18` is `void` (frees every
item, printing `TextMenu::freeListItem`), slot 13 calls it, slot 5 `getItem`, slot 12 erases one item;
`addItem` (push_back), `insertSorted(item, cmp)` 0x208d0 (no caller found) and `addUnique(item)`
0x20a3c (false if an item with that label exists; was `hasItem(std::string*)`).

**ROM cache** (0x20b1c–0x23548). Each emulator folder has `<path>/<name>_cache6.db` with a table
`<name>_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin)`:

| Addr | Name | Does |
| --- | --- | --- |
| 0x20b1c | `database_strlen_20b1c` | last path component (same code as tu_1484d4's) |
| 0x20bd4 | `importGameList(xml, arg, table, db)` | DROP/CREATE, then rows from `gamelist.xml` via rapidxml (`gameList/game/{path,name,image}`), only `.zip` or known file types; pinyin from `pinyinInitials` |
| 0x21818 | `isHiddenRomFile(emu, path, name, hideBios)` | scan filter: `[BIOS]`, dot files, neogeo/gngeo, COPYING/LICENSE; extension list, or a save/db/png blacklist plus `.bin` with a `.cue` |
| 0x21d50 | `scanRomDir(dir, imgDir, emu, table, sub, db, ...)` | readdir walk, one level of subfolders (type 1 rows); pinyin columns from `GetGameName("en.lang"/"ch.lang", name)` when `EmuInfo::hasCpinyin` |
| 0x2291c | `rebuildRomCache(emu, arg, table, db)` | DROP/CREATE, then `scanRomDir(path, imagePath, ...)` in one transaction |
| 0x22c88 | `isRomFile(name, dir)` | `countDirectoryItems` filter for the progress total |
| 0x22d90 | `database_lang_22d90(emu, arg, dir, search, buf, size)` | open the cache, count rows (folder `ppath` or pinyin/cpinyin search); empty: `importGameList`, else `rebuildRomCache` |
| 0x23318 | `buildRomList_23318(emu, arg, &dir, &search)` | `create_rom_browser_menu`: `DBCachedTextMenu(emu->label, emu, arg, search)`, count, `setTotal`, `open(db, name, dir or ".")` |

The `arg` passed down from `createRomListMenu` is never used by these functions.

**rapidxml 1.13** is now fetched by `scripts/fetch-rapidxml.sh` (pinned SHA-256, into `build/`) and
included by p05 (with `<fstream>`, `<iterator>`, `<sys/time.h>`). All p05 rows in `matches.tsv` carry
`deps` `rapidxml`; `run_matches.sh` now gives a file the union of its rows' deps (as `unitcheck`
already did), so the other `tu_015b20.cpp` rows keep `-`.

**-O0 idioms.**
- `goto done;` reproduces both early exits in 0x22d90 (one after the COUNT step, one after
  `importGameList`), including the `nop` landing blocks.
- `if (isHiddenRomFile(...)) continue;` gives the `nop; b loop` block; `!f()` on a bool function
  emits `eor`.
- `if (strncasecmp(ext, ".zip", 4) != 0) { if (fileType(path) < 0) continue; }` emits the
  materialized `lsr #31` test; the `&&` spelling does not.
- Loop counters over `items.size()` are `int` (`movhi`); an `int` return from the scan filter adds
  `moveq/movne`, so it is `bool`. `rfind` results are `size_t`. A size parameter passed straight to
  `snprintf` is `int` (a `size_t` one is reloaded after the call arguments).
- Address-taken locals (`err`, `db`, `stmt`, `timeval`) and strings get slots after the scalars, in
  declaration order; `char *err` is the first declaration in 0x20bd4, and an uninitialized `int cost;`
  holds its slot before `bufSize`.

**Shared declaration changes.** `TextMenu::vfunc18` is `void`, so the slot-18 overrides in
`SettingMenu` (0x2a6ec), `DeviceInfoMenu` (0x2b504) and `ColorMenu` (0x2c760) are `void` too.
`fileType_2cfc0` returns `int` (tested `< 0`). `database_lang_22d90(EmuInfo*, int, std::string,
std::string, char*, int)`. `buildRomList_23318(EmuInfo*, ...)`. The `DBCachedTextMenu` ctor is
`(const char *, EmuInfo *emu, int, std::string search)` (by value; p06 should retype `field414`).
`TextMenu::hasItem` became `insertSorted` and `addUnique`.

## Shared declarations (header pass, before p03-p11)

p01 and p02 each declared the same classes differently in `tu_015b20.h` and had to be reconciled at
merge. To let p03-p11 run in parallel, every class, global and cross-part function they share is now
declared once: `src/ListMenu.h` (ListMenu, TextMenuDrawer), `src/FileBrowserCache.h` (TextItem,
FileAction), `src/MenuHelpers.h` (BGWorker), `src/TrimusWindow.h`, `src/MenuWindow.h`, and the
section "Shared declarations for parts p03-p11" at the end of `src/tu_015b20.h` (globals,
prototypes, menus, workers, windows, then `static_assert`s on every size).

**Rule for part sessions.**
- Use the shared declarations as they are. Put only part-local things in your own `.inc`: static
  helpers, file-local structs, `extern`s for globals no other part uses, string tables.
- You own the classes listed for your part below. You may add non-virtual member declarations to
  them and rename or retype a member marked `unverified`, keeping the `static_assert`s true. Never
  reorder virtual slots or members.
- If a declaration outside your classes must change (a signature, a type, a class you do not own),
  make the smallest change, keep `scripts/run_units.sh` and every match passing, and list it under
  "Shared declaration changes" in your report so the merge can check it.
- `unverified` marks a choice from incomplete evidence (argument types, names, unknown members as
  `raw` arrays). Correcting it is expected; say so in the report.

| Part | Owns |
| --- | --- |
| p03 | `DeleteRomAction`, `TrimusWindow` (ctor, dtor, `dump`), `ListMenu` ctor and `setRect`, helpers 0x1a4bc-0x1d720 |
| p04 | `ListMenu` slots (0x1d9d8-0x1dc30, 0x1de34), `TextMenuDrawer` (0x1e128-0x204d4) |
| p05 | `TextMenu`; the database/scan helpers 0x20b1c-0x23318 |
| p06 | `DBCacheWorker`, `DBCachedTextMenu`, `TextItem` ctor/dtor, `IconMenuItem`, `IconMenu`, `SettingMenuItem` |
| p07 | `SettingMenu`, `DeviceInfoMenu` |
| p08 | `ColorMenu`, `LanguageMenu`, `ThemeMenu`, `FileAction`, `FileBrowserMenu`, `LoadSurfaceWorker` |
| p09 | `MenuWindow`, `FloatWindow` |
| p10 | `PopUpWindow`, `NotifyWindow`, `CountDownWindow`, `DialogWindow`, `ProgressDialog`, `GameDetailWindow` |
| p11 | `ADCInfoWindow`; window stack, loop, `createMenu` |

**What the pass found** (from vtables, RTTI, the ctors and the tail):
- Inline methods now in the headers (tail, phase 3 orders them): `ListMenu` dtor, slots 6 and 11-15,
  getters `getCur/getPageStart/getPageEnd/getSearch/getRect`; `TextMenuDrawer` setters at 0x3a678 and
  0x3a6a8; `count()` of TextMenu, DBCachedTextMenu, IconMenu; `TextMenu::getItems` (printf);
  `DBCachedTextMenu::setTotal`; three `IconMenu` setters; `DeviceInfoMenu::vfunc4`; five slots each of
  `FloatWindow` and `ADCInfoWindow`; `DBCacheWorker` and `LoadSurfaceWorker` ctors and dtors;
  `BGWorker` dtor; `FileAction` dtor; `BusyData` ctor (0x3adc4, `{0, -1, 0}`, so it has a third
  field); the free function `isListMenuId` (0x3ad44).
- ListMenu slots 4, 5, 7 and 8 are pure; slots 2, 9, 10, 16 and 17 (0x1d9d8...) are ListMenu's own,
  not TextMenuDrawer's or FileBrowserMenu's (IconMenu and FileBrowserMenu use them directly). Ghidra's
  "FileBrowserMenu::" and "DBCachedTextMenu::" labels on 0x1d9d8-0x204d4 are inherited slots.
- `FileAction` has no slot 3/4 overrides (the vtable shows Action's); its old declarations are gone.
- The base Ghidra left unnamed in p02 is `MenuItemAction` in RTTI (renamed from `ItemAction`).
- `rapidxml` is included: `rapidxml::parse_error` ctor/`what` sit at 0x3acc0/0x3ad1c and its
  templates later in the tail. The file includes the header-only rapidxml; the part that parses XML
  adds the include (not yet fetched by any script).
- The tail getter at 0x3aad4 (+0xc, called by `GameDetailWindow::update`) has no class yet.

**Part ranges.** The boundaries hold (no function straddles one), but some labels were wrong:
- p03 also holds `TrimusWindow::dump` 0x1d898, the `ListMenu` ctor 0x1d8fc and `setRect` 0x1d99c.
- p04 is ListMenu's own slots plus `TextMenuDrawer`, not FileBrowserMenu/DBCachedTextMenu.
- p05 is `TextMenu` (0x20500-0x20b1c) and then about 10 KB of database/ROM-scan helpers.
- Three functions are missing from the Ghidra export: 0x1ec00 (p04, 512 bytes, unreferenced: cuts a
  UTF-8 string to a width and appends "..."), 0x25c68 (p06, returns `g_iconFont`, stored in
  `IconMenu::fontFn`) and 0x37760 (p11, returns `g_smallFont`, passed to `setFontFn` in `createMenu`).

## p04 findings (0x1d9d8–0x20500, done)

All 21 functions match with `matchfn` (11,048 bytes, exactly the range, including the unexported
`truncateUtf8` 0x1ec00, 484 bytes). p03 was written in parallel, so the body prefix was not checked
past p02; it should run straight through p04 once p03 is merged.

**How a selected text-menu item becomes an Action.** Every text menu (TextMenu, DBCachedTextMenu,
MultiDBCachedTextMenu, the settings/theme/language/colour menus) inherits:
1. key input `ListMenu::vfunc3` 0x1de34 (`TextMenuDrawer::vfunc3` 0x204d4 only forwards to it):
   `key = handleGlobalInput(0)`, `recordKey(key)`; keys 1 and 4 (A) print `"a pressed at %dms"` and
   return `vfunc4()`; 2/3 return 1; 10/11 are returned as they are; 5/6 move up/down (by six once
   a key has repeated more than 25 times within 200 ms each, `keyRepeating`), 12/13 page by six;
   7/8 move when `flag08` is set (IconMenu) and otherwise return 5/6 to the window; any other key goes
   to slot 15 (`vfunc15(key)`, -1 in ListMenu). Everything else returns -1.
2. onSelect `TextMenuDrawer::vfunc4` 0x1e518: `item = (TextItem *)getItem(cur)`; if the item has an
   `action` (+0x18), return `item->action->execute(item)` when it is >= 0, else -1. So an Action's
   `execute` gets its own TextItem as `param`, and its return code (4 popup queued, 1 list edited,
   -1 nothing / window pushed directly; see p02) goes back from `handleInput` to the window.

**TextMenuDrawer.** Ctor `(bool noSelectedBg, bool stripes, int rows, const char *background)`:
loads the selection background (`skin/bg-list-s.png` unless given) and the favourite mark, sets
`rect` = {0, 60, 640, 360}, row height 60. `fieldC4` is the rows per page, `flagC1` draws
`g_listStripe` between rows, `flagC0` passes "selected" as the text shadow flag. Draw (0x1f6f0)
clamps `pageStart/pageEnd` around `cur`, then draws per row: `surface48` (row background, never set
here), the selection background, `surface54` (set only by `setBackground` 0x1f5e0, no caller found),
the item icon (`TextItem::value`, an `SDL_Surface *`), and the label through `drawHighlighted`
0x1ede4 (search `str18` matched with `pinyinFind` 0x143cf4 against `str40`, the pinyin initials,
else the label; labels over 48 bytes are cut to 64 bytes in `shortLabel` +0x5d). Items with
`flag24` show a 71x71 thumbnail (`field70`, loaded once from `str58` by `loadImage`), the label
and `str28` below in `g_selectedColor`. A null item shows `languageString(0x4c)`.

**-O0 idioms.**
- `TextMenuDrawer` redeclares `getItem` and `count` as pure virtuals: a call through the inherited
  ListMenu declaration evaluates `this` before the arguments and does not match (0x1e518, 0x1f6f0).
- `bool && (i == cur)` passed as an argument: the original is `flagC0 ? i == cur : false`.
- `if (field70 == NULL) if (str58.length() != 0)` (nested): the `&&` form materializes the whole
  condition.
- A `struct timeval` declared in a case body without braces (braces add a `nop`); `int now =
  milliseconds(); if ((unsigned long)(now - field38) < 200)` (an `unsigned long now` adds a `mov`).
- `int i` against `strlen()` (not `unsigned`) in `drawListNumber`; `strlen(text) > start - text + len`
  (strlen first); `sr.x = (short)x + sr.w + 20`.
- Stack order follows declaration order: `int w, h;` before `SDL_Rect r = rect;`; in the draw, `sr`,
  `color`, `dr` are declared at function scope in that order, and an unused `int size = 71` is kept.

**Tail.** `recordKey` is the first user of `std::vector<int>` `size`/`operator[]`/`push_back`/`clear`
(0x3ba0c, 0x3ba44, 0x3ba7c, 0x3bb00); `<sys/time.h>` is included in p04.inc for `gettimeofday`.

**Shared declaration changes.**
- `ListMenu.h`: slot 2 parameter names; non-virtual `recordKey`/`keyRepeating`; `field38` kept
  (commented). TextMenuDrawer (owned): `field48/50/54` -> `surface48`, `favoriteMark`, `surface54`
  (all `SDL_Surface *`), `raw5d` -> `shortLabel`, pure `getItem`/`count` redeclared, new non-virtual
  `makeSelectedBackground`, `drawHighlighted`, `setBackground`, `freeSelectedBackground`.
- `FileBrowserCache.h` (TextItem, p06): `value` (+0x20) and `field70` (+0x70) retyped from `int`
  to `SDL_Surface *`, names kept; `struct SDL_Surface;` forward declaration added. The 0x25510 ctor
  still declares `int value` (p06 to settle).
- `tu_015b20.h`: `drawListNumber_1e868(SDL_Surface *, SDL_Rect *, int, int)` ->
  `drawListNumber(int n, SDL_Rect *rect, SDL_Surface *dst, TTF_Font *font)` (p10's caller).
- Part-local in p04.inc: `g_listNumberColor` 0x18228c, `g_highlightColor` 0x1822b0,
  `SDL_CreateRGBSurface`, `TTF_SizeText`, `loadImage` 0x123a38, `pinyinFind` 0x143cf4.

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
| p03 | 0x1a44c–0x1d9d8 | 13.7 KB | `DeleteRomAction`, helpers, `TrimusWindow`, `ListMenu` ctor |
| p04 | 0x1d9d8–0x20500 | 11.0 KB | `ListMenu` base slots, `TextMenuDrawer` |
| p05 | 0x20500–0x23548 | 12.4 KB | `TextMenu`, database/ROM-scan helpers |
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
> (range from the plan) in src/tu_015b20/pNN.inc, functions in address order, using the shared
> declarations and following the rule in "Shared declarations" (part-local helpers only in your
> .inc; report any shared declaration you had to change). Every function must MATCH with tools/matchfn.py; check the body
> prefix with tools/unitcheck.py --body-only if the earlier parts are in main. Keep existing
> matches and units passing. Work on a new branch, commit as nukefrenzy, don't push; stop and
> report what matched, what didn't, and header changes. Ghidra exports are in ~/mainui-exports/
> (use tools/packet.py); never read MainUI-354-clean_named.c whole.
