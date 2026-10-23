# Source guide

How MainUI works, and where each part of it lives in `src/`. Read this before diving into a file.
For what MainUI does from the outside (file formats, launch commands, timings), see
[behavior.md](behavior.md).

## The program in one picture

```mermaid
flowchart TD
  main["main\nhardware, SDL, theme, threads"] --> loop["runWindowLoop\ndraw top window + status bar, flip, input"]
  loop --> stack[("window stack\nvector of TrimusWindow*")]
  stack --> mw["MenuWindow\none per screen"]
  stack --> fw["FloatWindow family\ndialogs, pop-ups, progress"]
  stack --> tw["keyboard, game detail, test windows"]
  mw --> lm["ListMenu\nIconMenu, TextMenu, FileBrowserMenu, ROM lists"]
  lm --> cm["createMenu(id, path)\nbuilds each screen's list"]
  lm --> items["rows"]
  items --> act["Action::execute\nopen a screen, pop-up, launch"]
  act --> push["pushWindow"] --> stack
  act --> run["writeCmdToRun\n/tmp/cmd_to_run.sh"] --> exit["MainUI exits; the shell runs the command"]
  thr["status thread + hotplug thread"] -- "SDL user events" --> loop
  db[("SQLite ROM caches")] --- lm
```

MainUI is a C++ program on SDL 1.2. Every screen is a **window** on a stack. Most windows are a
`MenuWindow` holding a **list** (`ListMenu`). A list's rows carry **actions**, and an action
either opens another window or writes a launch command and exits. MainUI never runs games
itself: Onion's shell script runs the command and then starts MainUI again, which reopens the
same screens from `/tmp/state.json`.

## How the source is organized

The original program was built from separate source files (translation units). Each one became a
file named after its start address, `src/tu_<address>`, holding its functions **in the original
address order**. Each file starts with a header comment saying what it contains.
[tu-map.tsv](tu-map.tsv) lists the exact ranges.

| File | Contents |
| --- | --- |
| [tu_015b20.cpp](../src/tu_015b20.cpp) + `tu_015b20/p01–p11.inc` | The main user interface (about 1,100 functions), split into eleven parts by address: |
| [p01](../src/tu_015b20/p01.inc) | text drawing, status-bar indicators, global input (`handleGlobalInput`), launching games and apps |
| [p02](../src/tu_015b20/p02.inc) | list actions (apps, favorites, search, dialogs) and the busy screens |
| [p03](../src/tu_015b20/p03.inc) | favorites, Recent and app lists, ROM deletion, saving/restoring windows |
| [p04](../src/tu_015b20/p04.inc) | `ListMenu` navigation and `TextMenuDrawer` drawing |
| [p05](../src/tu_015b20/p05.inc) | `TextMenu` and building the ROM cache databases |
| [p06](../src/tu_015b20/p06.inc) | ROM lists backed by the cache, icon menus, settings rows |
| [p07](../src/tu_015b20/p07.inc) | the Settings and Device Info menus |
| [p08](../src/tu_015b20/p08.inc) | color, language, theme and file-browser menus |
| [p09](../src/tu_015b20/p09.inc) | `MenuWindow` and `FloatWindow` |
| [p10](../src/tu_015b20/p10.inc) | pop-ups, dialogs, the power-off countdown, the game detail window |
| [p11](../src/tu_015b20/p11.inc) | the window loop, `createMenu`, the charging screen |
| [tu_049c3c.cpp](../src/tu_049c3c.cpp) | `main` and the status and hotplug threads |
| [tu_04c418.cpp](../src/tu_04c418.cpp) | a small circular linked list |
| [tu_04cb68.cpp](../src/tu_04cb68.cpp) | factory hardware tests (keys, LCD, sound, SD card) |
| [tu_11cdc8.cpp](../src/tu_11cdc8.cpp) | reading Emu/App/RApp/Themes `config.json` folders, the Recent list, device and SD card helpers, image loading |
| [tu_129f9c.cpp](../src/tu_129f9c.cpp) | loading a theme: fonts, images, colors, sounds |
| [tu_12ce28.cpp](../src/tu_12ce28.cpp) | reading folders for the file browser |
| [tu_1302cc.cpp](../src/tu_1302cc.cpp) | a DEFLATE (zlib) decoder adapted from stb_image |
| [tu_131e34.cpp](../src/tu_131e34.cpp) | `LruCache` and `WorkerQueue` (thumbnail loading) |
| [tu_134080.cpp](../src/tu_134080.cpp) | user-interface languages |
| [tu_1376fc.cpp](../src/tu_1376fc.cpp) | the button-mapping screen |
| [tu_138c50.cpp](../src/tu_138c50.cpp) | the Wi-Fi and hotspot screens |
| [tu_13edfc.cpp](../src/tu_13edfc.cpp) | the on-screen keyboard (`ImeWindow`) |
| [tu_143720.cpp](../src/tu_143720.cpp) | the pinyin table for ROM search |
| [tu_1484d4.cpp](../src/tu_1484d4.cpp) | global ROM search across all emulators |
| [tu_14acfc.c](../src/tu_14acfc.c) | battery, backlight, volume, audio hotplug (C) |
| [tu_14b860.c](../src/tu_14b860.c) | `system.json` settings and the keymap (C) |
| [tu_14cb24.c](../src/tu_14cb24.c) | a read-only zip reader (C) |
| [tu_14da50.c](../src/tu_14da50.c) | the built-in English strings and `.short` language files (C) |
| [tu_14dc18.c](../src/tu_14dc18.c) | I2C helpers for the device check (C) |
| [tu_14e0cc.c](../src/tu_14e0cc.c) | the Wi-Fi backend: `wpa_cli`, scanning, the monitor thread (C) |
| [tu_14f730.c](../src/tu_14f730.c) | the device check run before anything else (C, vendor code built with GCC 7.3.0) |

### What is in the binary

| Part | Address range | Source |
| --- | --- | --- |
| C start-up code | before 0x15b20 | the toolchain |
| Miyoo's own code, part 1 | 0x15b20–0x51b5c | `src/` (main UI, main, hardware tests) |
| cJSON 1.7.10, compiled as C++ | 0x51b60–0x57920 | downloaded |
| SQLite 3.30.1 | 0x57920–0x11cdc8 | downloaded |
| Miyoo's own code, part 2 | 0x11cdc8–0x150ad8 | `src/` (helpers, theme, languages, Wi-Fi, C units) |
| libgcc helpers | from 0x150ad8 | the toolchain |

Template code from the header-only libraries (tiny-utf8, RapidXml) and from libstdc++ sits after
the functions of each file that uses it.

Not in `src/`:

- **Third-party code compiled into MainUI**, which the build downloads from the projects' own
  releases: SQLite 3.30.1, cJSON 1.7.10, and the header-only libraries tiny-utf8 4.4.3 and
  rapidxml.
- **The C start-up code**, which comes from the toolchain.

### Headers

Class layouts are exact, and each class is declared once:

- [TrimusWindow.h](../src/TrimusWindow.h), [MenuWindow.h](../src/MenuWindow.h), and the others
  in `src/`;
- [tu_015b20.h](../src/tu_015b20.h) for the classes of the main UI unit.

The numbers MainUI passes around have names:

| Header | Names |
| --- | --- |
| [Input.h](../src/Input.h) | Miyoo buttons as SDL keys, button codes, user-event codes, list results |
| [WindowIds.h](../src/WindowIds.h) | window and menu ids (`WIN_SETTING`, `WIN_ROM_LIST`, …) |
| [LangStrings.h](../src/LangStrings.h) | the 156 user-interface strings, with their English text |
| [ShmVar.h](../src/ShmVar.h) | settings shared with keymon (`MONITOR_VOLUME`, …) |
| [PlatformApi.h](../src/PlatformApi.h) | C functions shared between units; Wi-Fi states and modes |
| [TrimusWindow.h](../src/TrimusWindow.h) | what a window's `handleInput` result tells the window loop |
| [SdlApi.h](../src/SdlApi.h), [SDLTypes.h](../src/SDLTypes.h), [CJsonApi.h](../src/CJsonApi.h), [SQLiteApi.h](../src/SQLiteApi.h) | the library functions and structures MainUI uses |

### Conventions you will see

- **Address comments.** `// 0x2edac: …` before a function gives its address in the original
  binary, so you can find it in Ghidra or in a disassembly.
- **Original quirks.** Comments starting `Original quirk` or `Original bug` mark behavior kept
  exactly as the original has it. [behavior.md](behavior.md#original-quirks-and-bugs) lists them.
- **`-O0` style.** MainUI was compiled without optimization, so the machine code follows the
  source statement by statement. The source therefore keeps the original's statement order,
  redundant assignments, and occasional unused locals; "tidying" them changes the bytes.
- **Order matters.** Functions appear in address order. The include order of headers with inline
  functions decides where the compiler places template code, so includes are never sorted.
- **Binary patches.** Calls that Onion replaced by NOPs after compiling are written as small
  `asm()` blocks at the call site. Strings that Onion overwrote keep their original slot size
  (see [onion-changes.md](onion-changes.md)).
- **Explicit string pools.** Some files declare their strings as named arrays (`lit_*`) in the
  original order, where ordinary literals would be merged or reordered.
- **Globals.** `extern` declarations carry their original address (`// 0x181928`). Their initial
  values are listed in [src/data.tsv](../src/data.tsv).

## Start-up

`main` ([tu_049c3c.cpp](../src/tu_049c3c.cpp)) initializes the hardware and SDL, loads settings,
theme and languages, scans the Emu, App and RApp folders, restores the previous windows (or
builds the main menu), and runs `runWindowLoop`. [behavior.md](behavior.md#start-up-and-exit)
gives the exact order.

## The window loop

`runWindowLoop` ([p11.inc](../src/tu_015b20/p11.inc)) works on a `std::vector<TrimusWindow*>`;
the top window is `back()`. While the stack isn't empty and the running flag is set, each pass:

1. calls `update()`, `animateIn()` and `draw(g_backBuffer)` on the top window;
2. draws the status-bar indicators right to left, except on the button-mapping screen;
3. copies the back buffer to the screen;
4. if a window was queued with `pushWindow`, moves it onto the stack and calls its `onShow()`;
   otherwise passes input to the top window's `handleInput()` and acts on the result.

| `handleInput` result | Effect |
| --- | --- |
| -1 | nothing; keep looping |
| `WINDOW_CLOSE` (1) | close this window, if it isn't the last one |
| `WINDOW_QUIT` (2) | leave the loop |
| `WINDOW_LAUNCH` (3) | close this window if its id is negative, then leave the loop (a launch command is waiting) |
| `WINDOW_FORCE_CLOSE` (4) | close this window |

### TrimusWindow

`TrimusWindow` ([TrimusWindow.h](../src/TrimusWindow.h)) is the 12-byte base of every window:
vtable, `titleLabelId` (a language string id), `id` (a [window id](../src/WindowIds.h)). Its
virtual functions are, in vtable order:

1. the destructors;
2. `handleInput`, `draw`, `onHide` and `onShow`;
3. `saveState` and `restoreState`, which read and write `/tmp/state.json`;
4. `update`;
5. `animateIn` and `animateOut`.

## Classes

The C++ type information survives in the binary, so all 69 class names and their inheritance are
the original ones.

| Family (base) | Members | Role |
| --- | --- | --- |
| **TrimusWindow** | MenuWindow; FloatWindow → DialogWindow, PopUpWindow, ProgressDialog, NotifyWindow → CountDownWindow; GameDetailWindow, ImeWindow, KeySettingWindow; test windows (KeyTest, KeyTest2, SDLKeyTest, LCDTest, SoundTest, SDCardTest, ADCInfo) | Everything on the window stack |
| **ListMenu** | IconMenu, FileBrowserMenu; TextMenuDrawer → TextMenu → SettingMenu, ThemeMenu, LanguageMenu, ColorMenu, DeviceInfoMenu, HardwareTestMenu; DBCachedTextMenu, MultiDBCachedTextMenu; WifiMenuDrawer → WifiMenu, HotspotMenu | The list inside a MenuWindow |
| **Action** (`execute`, `progress`, `finished`) | AppAction, EmuAction, FileAction, GeneralAction, SearchAction, GlobalSearchAction, Popup*Action, Wifi*Action, Hotspot*Action; MenuAction → Delete/ClearRecent/ClearFavorite; MenuItemAction → Add/RemoveFavorite | What a row does when chosen |
| **ItemDrawable** | WifiItem, WifiLabel, WifiOnOffLabel, Hotspot*Item | Custom-drawn rows |
| **BGWorker** | LoadSurfaceWorker, DBCacheWorker | Background loading |

### Screens and lists

**`MenuWindow`** ([MenuWindow.h](../src/MenuWindow.h), [p09.inc](../src/tu_015b20/p09.inc), 0x11c
bytes) holds one list (`menu`), the saved list position, a thumbnail loader and cache for game
previews, and the folder path for ROM lists. `onShow` builds the list lazily:

- the global search window (24) builds a `MultiDBCachedTextMenu`;
- ROM lists (5 and 17) build a `DBCachedTextMenu` on the emulator's SQLite cache;
- every other window calls **`createMenu(id, path)`** ([p11.inc](../src/tu_015b20/p11.inc)).

| id | createMenu builds | id | createMenu builds |
| --- | --- | --- | --- |
| 0 | main menu (`IconMenu`) | 9 | Device Info |
| 1 | Games: one icon per emulator | 10 | Recent |
| 2 | Favorites | 11 | Hardware Test |
| 3 | Apps | 14 | Language |
| 4 | file browser | 18 | display color |
| 7 | Settings (`SettingMenu`) | 19 | Theme |
| 16 | RetroArch apps | 20 | Fixes |

Main-menu items are `IconMenuItem`s carrying the id of the menu they open. `IconMenu::onSelect`
creates a `MenuWindow` for that id and pushes it.

**Rows.** `TextMenu`, `IconMenu` and the file browser keep their rows as raw pointers.

- A `TextItem` owns its `Action` only when `ownsAction` is set. Accepting a row runs
  `Action::execute`.
- `EmuAction` launches a ROM or opens a ROM folder; `AppAction` launches an app.
- The pop-up menu on a ROM row (`MenuWindow`'s secondary menu) offers add/remove favorite,
  delete and search. Destructive choices are wrapped in a `PopupDialogAction` confirmation.
- `PopupProgressAction` runs an action in a `ProgressDialog` with a worker thread.

**Settings rows** are `SettingMenuItem`s whose handler number (`SettingHandler` in
[tu_015b20.h](../src/tu_015b20.h)) tells `SettingMenu::select` and `handleKey` what to do:

- **Value rows** change their value with left/right.
- **Submenu rows** push a new `MenuWindow`.
- **Destructive rows** ask for confirmation first.

**Lists in other units.**

| What | Where | Notes |
| --- | --- | --- |
| Wi-Fi and hotspot screens | [tu_138c50.cpp](../src/tu_138c50.cpp) | `WifiMenu` keeps up to 32 scan results |
| Wi-Fi backend | [tu_14e0cc.c](../src/tu_14e0cc.c) | `wpa_cli` and the monitor thread |
| Button-mapping screen | [tu_1376fc.cpp](../src/tu_1376fc.cpp) | `KeySettingWindow` |
| Applying the keymap | `setupKeymap` in [tu_14b860.c](../src/tu_14b860.c) | writes the mapping to the kernel driver |
| File browser | [tu_12ce28.cpp](../src/tu_12ce28.cpp) | Caches up to 40,000 rows of visited folders; loads at most 20,000 items per folder |

### Background work

| Part | Where | What it does |
| --- | --- | --- |
| `WorkerQueue` | [tu_131e34.cpp](../src/tu_131e34.cpp) | One SDL thread that runs `BGWorker` jobs, newest first. When too much work is pending, it drops the oldest jobs. |
| `LruCache` | tu_131e34.cpp | Keeps loaded thumbnails, at most 1,000 entries |
| `DBCacheWorker` | [p06.inc](../src/tu_015b20/p06.inc) | Loads a page of ROM rows from SQLite |
| `LoadSurfaceWorker` | [p08.inc](../src/tu_015b20/p08.inc) | Loads a thumbnail image |

### Threads

| Thread | Where | What it does |
| --- | --- | --- |
| Status thread | `statusPollThread`, [tu_049c3c.cpp](../src/tu_049c3c.cpp) | Every second, posts SDL user events when the battery, Wi-Fi state or volume indicator changes |
| Hotplug thread | tu_049c3c.cpp, with the helpers in [tu_14acfc.c](../src/tu_14acfc.c) | Posts an event when a second sound card appears or disappears |
| Wi-Fi monitor | [tu_14e0cc.c](../src/tu_14e0cc.c) | Every 3 s; starts DHCP once connected |

`handleGlobalInput` ([p01.inc](../src/tu_015b20/p01.inc)) is the single input function every
window calls. It turns SDL events into button codes and handles the power key, sounds and these
user events.

## Shared libraries MainUI links against

- **SDL 1.2**, with SDL_ttf, SDL_image and SDL_mixer: drawing, fonts, PNGs, sound.
- **Miyoo libraries:**
  - `libshmvar`: settings in shared memory, `InitKeyShm`/`GetKeyShm`/`SetKeyShm`;
  - `libgamename`: `GetGameName`, ROM display names;
  - `librsautil`: `encrypt_path`, called on each ROM path at launch.
- **SigmaStar MI libraries:** `MI_DISP_*` for the display color settings, and `MI_SYS_ReadUuid`
  for the chip id shown in Device Info.
- **libstdc++, glibc 2.28, zlib, freetype.**

## Open questions

- What `encrypt_path` (from `librsautil`, set up by `encrypter_init` in `main`) does. MainUI
  calls it on each ROM path at launch, and nothing it returns is used.
- What a few rarely used fields hold. These are commented as such in the headers.
