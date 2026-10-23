# MainUI behavior reference

What the original MainUI (Onion build `MainUI-354-clean`) does, written for people who need to
reproduce or check its behavior without reading all of the source, for example the
[open-mainui](https://github.com/robcodedev/onionos-mainui-opensource) clean-room rewrite or
Onion developers changing MainUI.

Every statement here comes from the recovered source, which compiles to a byte-identical copy of
the original binary (see [building.md](building.md)). Each section names the functions it
describes, with their file and original address, so you can check the details yourself. Where
the original does something surprising, this page says so. The source keeps those quirks
unchanged on purpose, marked with comments that start with `Original quirk` or `Original bug`.

Contents:

- [Start-up and exit](#start-up-and-exit)
- [Files MainUI reads and writes](#files-mainui-reads-and-writes)
- [Launching games and apps](#launching-games-and-apps)
- [System update](#system-update)
- [Input](#input)
- [Status events and timings](#status-events-and-timings)
- [Screens and settings](#screens-and-settings)
- [Original quirks and bugs](#original-quirks-and-bugs)

## Start-up and exit

`main` ([Main.cpp](../src/Main.cpp), 0x4a2fc) does this, in order:

1. It writes `ondemand` to the CPU governor and creates `/tmp/stay_awake`.
2. It runs the **device check** (`waitForProbe`, [I2c.c](../src/I2c.c) and
   [DeviceAuth.c](../src/DeviceAuth.c)). A chip on I2C bus 1 must answer a challenge, with up to
   20 attempts. If it fails, MainUI exits with status 2 before showing anything.
3. If `8188fu.ko` isn't loaded and `/mnt/SDCARD/8188fu.ko` exists, it runs `insmod` on it.
4. It installs a SIGTERM handler that clears the running flag. The window loop then ends
   normally and state is saved.
5. It starts the Wi-Fi monitor thread, then SDL (video and timer) and SDL_ttf. It sets
   `trimui_show=yes`, and sets `AUDIODEV=/dev/dsp1` when a second sound card exists (else
   `/dev/dsp`).
6. **Charging mode** applies when `/tmp/no_charging_ui` is absent, the power key didn't start the
   device (`bootedByPowerKey`), and the charger state is 2 or 3. In that mode MainUI shows only
   the charging screen (see [Screens](#screens-and-settings)).
7. It opens the shared settings (`InitKeyShm`). Audio opens at 48 kHz, S16LSB stereo, with a
   128-sample buffer when `audiofix` is on and 4096 otherwise.
8. It sets key repeat (500 ms delay, 100 ms interval) and opens a 640×480×32 screen plus a back
   buffer of the same size.
9. It loads `/mnt/SDCARD/system.json` and the theme it names, or else the built-in skin in `./`.
   MainUI quits if no skin loads. Then it loads the languages.
10. If Wi-Fi is on in the settings but not running, it powers Wi-Fi up and starts
    `wpa_supplicant`.
11. It starts the status thread and the hotplug thread.
12. It loads `./py.dat` (the pinyin table) and the Recent list. It scans `/mnt/SDCARD/Emu`, `/App`
    and `/RApp`, where each subfolder with a `config.json` is one entry.
13. It restores the list positions from `/appconfigs/romwinidx.json` and the open windows from
    `/tmp/state.json`. Without saved windows it builds the main menu.
14. It runs the window loop (`runWindowLoop`, [MainUI.cpp](../src/MainUI.cpp), 0x36ee8).

**On exit**, the window loop:

- deletes `/tmp/cmd_to_run.sh` if the last result was "close" (1);
- saves the window stack to `/tmp/state.json`;
- saves the list positions to `/appconfigs/romwinidx.json`.

`main` then stops both threads, frees everything, saves the Recent list and exits. MainUI never
runs games itself. A launch writes a command file and exits, and Onion's runtime script runs
the command and starts MainUI again.

## Files MainUI reads and writes

### `/mnt/SDCARD/system.json`: settings

Read by `loadSystemJson` and written by `saveSystemJson`, both in
[Settings.c](../src/Settings.c) (0x14bca8 and 0x14c5fc). Stock Miyoo firmware uses
`/appconfigs/system.json` instead; see [onion-changes.md](onion-changes.md).

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `vol` | number | 20 | Volume, 0–20 |
| `keymap` | string | none | Passed to the `gpio_keys_polled` driver (`setupKeymap`) |
| `mute` | number | 0 | Read back wrongly; see [quirks](#original-quirks-and-bugs) |
| `bgmvol` | number | 8 | Menu music volume, 0–20 |
| `brightness` | number | 7 | Backlight, 1–10 |
| `language` | string | `en.lang` | File name under the language folder |
| `hibernate` | number | 0 | Sleep after 0 (never), 5, 15 or 30 minutes |
| `lumination` | number | 7 | Display color, 0–20 (anything else gets the default) |
| `hue` | number | 10 | 0–20 |
| `saturation` | number | 10 | 0–20 |
| `contrast` | number | 10 | 0–20 |
| `theme` | string | none | Theme folder, e.g. `/mnt/SDCARD/Themes/x/`; ignored if it no longer exists |
| `fontsize` | number | 24 | 24 is applied only when the whole file fails to load |
| `audiofix` | number | 1 | Launch through `libpadsp.so` |
| `wifi` | number | unchanged | Wi-Fi switch |

Missing or mistyped keys get their defaults. If the file can't be read or parsed, every setting
gets its default and the keymap is cleared. Live values sit in shared memory (`libshmvar`,
names in [ShmVar.h](../src/ShmVar.h)), where keymon and other processes read them.

On save the keys are written in this order: vol, keymap, mute, bgmvol, brightness, language,
hibernate, lumination, hue, saturation, contrast, theme, fontsize, audiofix, wifi. The output is
pretty-printed with `cJSON_Print`, and MainUI calls `sync()` afterwards. The file is opened
without truncation; see [quirks](#original-quirks-and-bugs).

### `/mnt/SDCARD/Roms/recentlist.json`: Recent

`loadRecentList`, `saveRecentList` and `addRecent` in [Config.cpp](../src/Config.cpp)
(0x1208a0, 0x120af4, 0x1204c4).

The file holds one unformatted JSON object per line, in this key order:

```json
{"label":"Game","rompath":"/mnt/SDCARD/Roms/GB/game.gb","imgpath":"/mnt/SDCARD/Imgs/GB/game.png","launch":"/mnt/SDCARD/Emu/GB/launch.sh","type":5}
```

- `rompath` and `imgpath` are left out when empty, as they are for apps.
- `type` is the id of the window the item came from (see [WindowIds.h](../src/WindowIds.h)).
- A launch adds the item at the front, first removing any entry with the same label, ROM path
  and type.
- The list keeps at most 20 entries, dropping from the other end.
- "Clear" deletes the file.

### `/mnt/SDCARD/Roms/favourite.json`: Favorites

`AddFavoriteAction::execute` in [MainUI.cpp](../src/MainUI.cpp) (0x1a02c) appends to this
file. The list is read in [MainUI.cpp](../src/MainUI.cpp) and rewritten by `saveFavorites`.
The file name keeps the original British spelling.

The format is one JSON object per line: `{"label":…,"launch":…,"type":…,"rompath":…}`.

- Only items from the Apps (3), ROM list (5), RetroArch list (17) or search (24) windows can be
  added; `type` records which one.
- An item whose ROM path is already listed isn't added again.
- "Clear" deletes the file.
- Recent and app lists mark favorites by reading this file.

### `/tmp/state.json`: open windows across a launch

`saveWindowState` and `restoreWindowState` in [MainUI.cpp](../src/MainUI.cpp). The file is
deleted after it is read.

```json
{"list":[{"title":<label id>,"type":<window id>, ...window fields...}, ...]}
```

Windows are listed bottom to top. Type 8 is recreated as a `GameDetailWindow` and every other
type as a `MenuWindow`.

| Window | Fields |
| --- | --- |
| `MenuWindow` (`saveState`, [MainUI.cpp](../src/MainUI.cpp), 0x2edac) | `currpos`, `pagestart`, `pageend`; `emuname` (type 5) or `raname` (others) when the list belongs to an emulator; `ppath` (parent folder) and `searchstr` when not empty |
| `GameDetailWindow` (`saveState`, [MainUI.cpp](../src/MainUI.cpp), 0x35fe4) | `gamename`, `gamepath`, `thumbpath`, `gameidx`, `gametype`, `favourite` (bool) |

### `/appconfigs/romwinidx.json`: list positions

`loadRomListPositions` and `saveRomListPositions` in [MainUI.cpp](../src/MainUI.cpp). The
format is `{"list":[{"rompath":…,"pos":…,"start":…,"end":…}, ...]}`, one entry per ROM folder: the
selected row, the first visible row, and the last visible row.

### Emulator, app and theme folders

Each subfolder of `/mnt/SDCARD/Emu`, `/App`, `/RApp` and `/mnt/SDCARD/Themes` that holds a
`config.json` is one entry. The parsers are in [Config.cpp](../src/Config.cpp).

| Folder | Parser | Keys (required in bold) |
| --- | --- | --- |
| Emu, RApp | `parseEmu` 0x121ac0 | **label**, labelverbose, **icon**, iconsel, **launch**, rompath + imgpath or gamelist (one of the two is required), useswap (default on), shortname (search by Chinese pinyin), extlist (`"ext1\|ext2"`), hidebios, noaudiofix |
| App | `parseApp` 0x12122c | **label**, **icon**, iconsel, **launch**, description, autoflip, noaudiofix |
| Themes | `parseTheme` 0x122d78 | name; font sections `title`, `path`, `hint`, `currentpage`, `total`, `grid`, `list` (font, fontascii, size, color `#RRGGBB`; `list` adds selectedcolor and highlightcolor; `grid` adds grid1x4/grid3x4 and icon/text offsets) |

Relative paths are made absolute. A later folder with the same launch path replaces an earlier
one. Emulators are kept sorted by label.

### ROM cache databases

Each emulator folder has a SQLite cache `<emu folder>/<name>_cache6.db`, built in
[MainUI.cpp](../src/MainUI.cpp) and read in [MainUI.cpp](../src/MainUI.cpp) and
[GlobalSearch.cpp](../src/GlobalSearch.cpp):

```sql
CREATE TABLE <name>_roms (id INTEGER PRIMARY KEY AUTOINCREMENT, disp TEXT NOT NULL,
  path TEXT NOT NULL, imgpath TEXT NOT NULL, type INTEGER DEFAULT 0, ppath TEXT NOT NULL,
  pinyin TEXT NOT NULL, cpinyin TEXT NOT NULL);
```

- `type` is 0 for a ROM and 1 for a subfolder. Scanning goes one level into subfolders
  (`scanRomDir`, 0x21d50).
- `ppath` is the parent folder, `.` for the top level.
- Lists are read page by page with `ORDER BY type DESC, disp LIMIT offset,count`.
- Search matches `pinyin` (or `cpinyin` when the emulator has `shortname` and the language is
  `ch.lang`) with `LIKE '%text%'`, ROMs only.
- With a `gamelist` in the config, the table is built from that `gamelist.xml` (`importGameList`,
  0x20bd4) instead of the folder.
- "Refresh Roms" deletes every `*_cache.db` … `*_cache6.db` (`removeAllDBFiles`, 0x11e390), so
  the caches are rebuilt.
- In the Onion build, inserts use prepared statements; stock Miyoo builds literal SQL text.

### Languages

`loadLanguages` in [Language.cpp](../src/Language.cpp) and the built-in strings in
[BuiltinStrings.c](../src/BuiltinStrings.c).

- **Built-in strings.** 156 English strings are built in (ids in
  [LangStrings.h](../src/LangStrings.h)). If `/customer/app/lang/en.lang` is missing, it is
  written from them.
- **Lookup.** `languageString(id)` returns the configured language's string, else the built-in
  one. Negative ids give `""` and ids of 156 or more give `wrongbeef`.
- **`.short` files.** For each language, `<file>.short` holds ids 88–155. It is written to
  `/customer/app/lang` and read from `/mnt/SDCARD/miyoo354/app/lang` first.
- **CJK fallback.** Language choices are limited to English when the CJK font
  (`wqy-microhei.ttc`) is missing.

### Other files

| Path | Use |
| --- | --- |
| `/tmp/cmd_to_run.sh` | The launch command ([below](#launching-games-and-apps)) |
| `/tmp/.try_upgrade_file` | Path of a confirmed update zip ([below](#system-update)) |
| `/mnt/SDCARD/test_result.json` | Hardware test results `testitem0`–`testitem4` and the device serial they belong to |
| `/tmp/no_charging_ui` | If present: no charging screen |
| `/tmp/battery_charging` | Created while the charging screen runs |
| `/tmp/no_audioserver` | Created when audiofix is off, or when the launched emulator or app has `noaudiofix` |
| `/tmp/wifimac`, `/tmp/wifi_verbose` | Wi-Fi MAC address written by MainUI; if `wifi_verbose` exists, every Wi-Fi status is logged |
| `/mnt/SDCARD/cachefile` | 128 MiB swap file, created for emulators with `useswap` |
| `./py.dat` | Pinyin table for search |

## Launching games and apps

The code is in [MainUI.cpp](../src/MainUI.cpp): `launchRom` (0x181a0),
`launchApp_buildCommand` (0x18560) and `writeCmdToRun` (0x17fc0). A launch:

1. builds a shell command;
2. writes it to `/tmp/cmd_to_run.sh` with `w+` and no newline;
3. adds the item to Recent and reapplies the keymap;
4. returns result 3, which ends the window loop.

ROM command, with `<cwd>` being MainUI's working directory (`/customer/lib/libpadsp.so` when that
can't be read):

```sh
LD_PRELOAD=<cwd>/../lib/libpadsp.so "<emu launch>" "<rom path>"   # audiofix on
"<emu launch>" "<rom path>"                                          # audiofix off
```

- When `audiofix` is on but the emulator has `noaudiofix`, MainUI creates `/tmp/no_audioserver`
  and uses the plain form.
- In the non-swap path, the `LD_PRELOAD` form has two spaces before the first quote.
- For emulators with `useswap` and no active swap, MainUI offers to create the swap file
  instead of launching.

App command:

```sh
cd <dir>; chmod a+x <file>; LD_PRELOAD=<cwd>/../lib/libpadsp.so [PADSP_NO_DSP=1] [PADSP_EN_FLIP=1] <file>
cd <dir>; chmod a+x <file>; <file>                     # neither audiofix nor autoflip
```

`PADSP_NO_DSP=1` is added when audiofix is off (or the app has `noaudiofix`), and
`PADSP_EN_FLIP=1` when the app has `autoflip`. A launch path without `/` does nothing (result -1).

## System update

Choosing a `TrimuiUpdate*.zip` in the file browser (`FileAction::execute`,
[MainUI.cpp](../src/MainUI.cpp), 0x2d1bc) asks for confirmation and needs 500 MB free. On
confirm, `requestUpgrade` (0x2d158) writes the zip's path to `/tmp/.try_upgrade_file` and
returns 3 (exit MainUI); the shell script around MainUI performs the update. Choosing another
file starts the emulator whose `extlist` matches its extension.

## Input

`handleGlobalInput` ([MainUI.cpp](../src/MainUI.cpp), 0x17744) is the only place windows get
input. The key symbols, button codes and event codes are in [Input.h](../src/Input.h).

- **When buttons report.** Buttons report on release. The exceptions are the arrows, L1/R1 and
  L2/R2, which report on press so held keys repeat (500 ms delay, then every 100 ms).
- **Key sound.** Each key press plays the "change" sound, except power and L1/R1.
- **SELECT/START held.** L2 and R2 are ignored while SELECT or START is held.
- **Power key.** Holding power for over a second opens the power-off dialog. Confirming it
  starts a 10-second countdown (`CountDownWindow`) before powering off.
- **Volume keys.** These are handled by keymon. MainUI only shows the volume indicator when
  keymon flags a change.
- **Audio hotplug.** Plugging or unplugging an audio device reopens audio.
- **Touch.** On devices with a touch panel, a tap at a screen edge becomes an arrow button.
- **List keys.** In lists, B and X close the window, and left/right page or change values (see
  `ListResult` in Input.h).

## Status events and timings

The status thread (`statusPollThread`, [Main.cpp](../src/Main.cpp), 0x4a050) wakes
every second and posts SDL user events:

| Event | When |
| --- | --- |
| `EVENT_BATTERY` (18) | Battery level (from keymon) or charger state changed |
| `EVENT_WIFI_STATUS` (31) | Wi-Fi mode changed, or every second in station mode (see quirks) |
| `EVENT_VOLUME_SHOWN` (40) | keymon flagged a volume change (the flag is then cleared) |
| `EVENT_VOLUME_HIDDEN` (41) | More than 3 s since the last change, repeated every second |

The hotplug thread checks every 100 ms and posts `EVENT_HOTPLUG` (19) when `/dev/dsp1` appears or
disappears.

| Timing | Value | Where |
| --- | --- | --- |
| Key repeat | 500 ms delay, 100 ms interval | `main` |
| Volume preview in Settings | 150 ms after the last change | [MainUI.cpp](../src/MainUI.cpp) |
| Notification window | 5 s | `NotifyWindow`, [MainUI.cpp](../src/MainUI.cpp) |
| Power-off countdown | 10 s, ticking every second | `CountDownWindow`, MainUI.cpp |
| ADC info refresh | every 500 ms | `ADCInfoWindow`, [MainUI.cpp](../src/MainUI.cpp) |
| Wi-Fi monitor | every 3 s | [Wifi.c](../src/Wifi.c) |
| Wi-Fi switch-on timeout | 10 s without a scan result | `EVENT_WIFI_SWITCH_TIMEOUT` |
| Hardware key test / sound test | 1 s ticks / 6 s | [HardwareTests.cpp](../src/HardwareTests.cpp) |

## Screens and settings

- **Main menu**, in order: Recent, Favorites, Games, RetroArch, Apps, Settings. In the Clean
  build the RetroArch icon is never added; see the table of disabled items below.
- **Window ids** are listed in [WindowIds.h](../src/WindowIds.h). `createMenu(id, path)`
  ([MainUI.cpp](../src/MainUI.cpp), 0x37784) builds each screen's list.
- **Settings rows** carry a handler number (`SettingHandler` in
  [MainUI.h](../src/MainUI.h)), handled by `SettingMenu::select` and
  `SettingMenu::handleKey` in [MainUI.cpp](../src/MainUI.cpp):
  - **Rows in this build:** music volume (0–20), brightness (1–10), sleep (never, 5, 15 or 30
    minutes), language, display color, device info, Wi-Fi and power off. The audio fix row
    (which toggles `/tmp/no_audioserver`) belongs to the Fixes submenu, which this build doesn't
    show.
  - **Handled but never added** by `createMenu`: volume (1), Refresh Roms (4), ADC info (9) and
    hotspot (20). Volume is set with the volume keys instead.
  - **Added in stock but disabled by Onion:** key settings, theme, Fixes, factory reset and key
    test (see below). Destructive rows ask for confirmation and then run with a progress window.
  - **Leaving Settings** saves `system.json`.
- **Disabled items.** Some items are prepared but never added in this build. `createMenu`
  allocates them, but their constructor and append calls are NOPs in the binary. The affected
  items are RetroArch (main menu), Key settings, Theme, Fixes, Factory reset and Key test. The
  source keeps every NOP site.
- **Charging screen.** `chargingScreenLoop` (MainUI.cpp, 0x38ff8) shows a battery animation from
  the current level upwards with "Charging...". It powers off if the charger is unplugged, and
  holding power for over half a second reboots into the normal menus.

## Original quirks and bugs

The rebuild must produce the same bytes, so the source keeps all of these. A rewrite may want to
fix them, but should know they exist.

| Behavior | Where |
| --- | --- |
| The `mute` setting is read from the `vol` value: any nonzero volume loads as muted when `mute` exists. A file with `mute` but no `vol` crashes (NULL dereference). | `loadSystemJson`, [Settings.c](../src/Settings.c) |
| `system.json` is written without truncating, so a shorter new file keeps the old file's tail. | `saveSystemJson`, Settings.c |
| In station mode, `EVENT_WIFI_STATUS` is posted every second, because the last reported state is never updated. | `pollWifiStatus`, [Main.cpp](../src/Main.cpp) |
| The battery check's charger-unplug branch can never run. | `pollBatteryStatus`, Main.cpp |
| Language paths are built as root + `./lang/…`, e.g. `././lang/`. | `loadLanguages`, [Language.cpp](../src/Language.cpp) |
| The ROM-list position loader always reports 0 entries restored. | `loadRomListPositions`, [MainUI.cpp](../src/MainUI.cpp) |
| The global search menu's destructor loops with an uninitialized index. | `~MultiDBCachedTextMenu`, [GlobalSearch.cpp](../src/GlobalSearch.cpp) |
| In global search, a position past the last match reads past the end of the database list. | `MultiDBCachedTextMenu::getItem`, GlobalSearch.cpp |
| The connected network's signal bars use different thresholds from the other networks. | `WifiMenu::onUnhandledEvent`, [WifiMenus.cpp](../src/WifiMenus.cpp) |
| On the keyboard screen, X adds a space without checking the 128-byte limit, and A on a key returns 0 instead of -1. | `ImeWindow::handleInput`, [ImeWindow.cpp](../src/ImeWindow.cpp) |
| The ADC info screen always says "Disconnected". | `ADCInfoWindow::draw`, [MainUI.cpp](../src/MainUI.cpp) |
| `loadSkinImage` prints "open ok" for absolute names even when loading failed, and `loadImage` "converts" to the image's own format (a copy). | [Config.cpp](../src/Config.cpp) |
| The file-browser cache trim can empty the list it is trimming. | `FileBrowserCache::trim`, [FileBrowser.cpp](../src/FileBrowser.cpp) |
| `LruCache::clear` clears its list outside the lock. | [MenuHelpers.cpp](../src/MenuHelpers.cpp) |
| The zlib helper `stbi_zlib_decode_malloc` overwrites the start of its buffer on every read, and `openZlib` leaks when given neither input nor file. | [stb_zlib.cpp](../src/stb_zlib.cpp) |
| The zip reader computes and ignores a "remaining" limit for stored entries. | `zipRead`, [ZipReader.c](../src/ZipReader.c) |
| `i2cRead` exits the whole program on some errors, leaks the descriptor on an ioctl error, and truncates the error to a byte. | [I2c.c](../src/I2c.c) |
| `setWifiScanEnable` and `wifiMonitorThread` are declared to return values but return nothing. | [Wifi.c](../src/Wifi.c) |
