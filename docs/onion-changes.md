# How Onion's MainUI differs from stock

Onion OS ships its own MainUI rather than the one in Miyoo's firmware. This page lists every
difference between the two, found by comparing the binaries function by function.

| | Binary | SHA-256 |
| --- | --- | --- |
| Onion | `MainUI-354-clean` (Onion v4.4.0 beta 20260120, commit 07505ea) | `98c85f6c…0c94` |
| Stock | Miyoo Mini Plus firmware, June 10 2023 build (`miyoo354/app/MainUI`) | `607dacd9…d2bc` |

Both files are 1,443,156 bytes, but they are **separate builds**. Every function after the first
change sits at a slightly different address, so about 1.3 MB of bytes differ even though most of
the code is the same.

## Method

Run the comparison yourself:

```sh
python3 tools/stockdiff.py <path to stock MainUI>
```

[`tools/stockdiff.py`](../tools/stockdiff.py) takes each of Onion's 4,863 named functions and
masks everything that changes when code moves: call and branch targets, `movw`/`movt` immediates
and literal-pool addresses. It then searches for the result in the stock binary.

- **4,840 functions are identical.** This includes all of SQLite and cJSON.
- **23 differ.** Six of them differ only in GOT offsets or alignment.
- **No stock function is missing from Onion.** Every gap in stock's coverage lines up with one of
  the 23.

Masking addresses hides changed strings, so the tool also lists `.rodata` strings that occur a
different number of times in the two builds. I didn't compare `.data` beyond its size, which is
the same in both builds.

## Two kinds of change

Onion's MainUI shows two kinds of change:

1. **Source changes.** These were compiled in, so the code around them was laid out again. Onion
   has no MainUI source, so these presumably came from a build Miyoo made for Onion.
2. **Binary patches** made after compiling. A call instruction is replaced by a NOP (`nop` or
   `mov r0, r0`), or a string is overwritten in place within its original slot. The recovered
   source reproduces these with inline assembly and padded strings; see the comments at each
   site.

## Source changes

| Change | Functions | Effect |
| --- | --- | --- |
| Settings file moved to the SD card | `loadSystemJson`, `saveSystemJson`, `loadThemeAndFontSize` ([tu_14b860.c](../src/tu_14b860.c)) | `/mnt/SDCARD/system.json` instead of `/appconfigs/system.json` |
| Clean shutdown on SIGTERM | `onSIGTERM`, `main`, `runWindowLoop` ([tu_049c3c.cpp](../src/tu_049c3c.cpp), [p11.inc](../src/tu_015b20/p11.inc)) | Stock installs no signal handler. Onion's `main` installs one with `sigaction`. It clears a `std::atomic<bool>` running flag, which `runWindowLoop` now takes as a parameter, so the loop ends normally and saves the open windows and list positions. The four small `std::atomic<bool>` functions exist only in Onion. |
| No SD card folder setup at start-up | `main` | Stock calls `initSdCardFolders` (creates App, Roms, Imgs and the per-system folders). Onion still contains the function but never calls it. |
| L2/R2 ignored while SELECT or START is held | `handleGlobalInput` ([p01.inc](../src/tu_015b20/p01.inc)) | New `g_selectHeld`/`g_startHeld` flags, so button combinations handled elsewhere in Onion don't also reach MainUI's menus |
| Dot files hidden from ROM lists | `isHiddenRomFile` ([p05.inc](../src/tu_015b20/p05.inc)) | A name starting with `.` is never a ROM (for example macOS `._` files) |
| ROM cache inserts use prepared statements | `importGameList`, `scanRomDir` (p05.inc) | Stock builds `INSERT … VALUES('%s',…)` text and doubles quotes with `replaceAll`. Onion binds the values with `sqlite3_prepare_v2`/`sqlite3_bind_*`, so names containing quotes are stored as they are. The error text becomes `db exec at %d error : %s`. |
| A log line number | `prepareRomCache` (p05.inc) | Prints 4100 instead of 4000 (the `__LINE__` of a moved source line) |

## Binary patches

| Patch | Where | Effect |
| --- | --- | --- |
| Six menu items disabled | `createMenu` ([p11.inc](../src/tu_015b20/p11.inc)), 12 `mov r0, r0` | Stock adds them, Onion doesn't: RetroArch (main menu; 0x37994/0x379a0), Key settings, Theme, Fixes, Factory reset, Key test (Settings). The objects are still allocated and leaked. |
| Text shadow off | `drawText` ([p01.inc](../src/tu_015b20/p01.inc)), 1 `nop` | The shadow blit under text is skipped |
| Wi-Fi indicator background off | `drawWifiIndicator` (p01.inc), 6 `nop` | Four background blits and two fills are skipped; only the icon is drawn |
| Backlight writes go nowhere | `initBacklightPwm` ([tu_14acfc.c](../src/tu_14acfc.c)), 1 `nop`, and a string | The PWM period write is skipped and the duty-cycle path `/sys/class/pwm/pwmchip0/pwm0/duty_cycle` becomes `/dev/null`. Onion controls the backlight from outside MainUI. |
| Power off handed to Onion | `powerOff` (p01.inc), string | `/customer/app/axp_test poweroff` becomes `mainui_off_order` |
| Battery icons hidden | theme loader ([tu_129f9c.cpp](../src/tu_129f9c.cpp)), strings | The five `skin/power-*-icon.png` paths all become `skin/.batt-perc.png`, so Onion's own battery display is used |

## Clean and Expert

Onion ships two MainUI binaries. They differ only in the first two `createMenu` NOPs above:

- **Expert** keeps the calls at 0x37994 and 0x379a0, so the RetroArch icon appears.
- **Clean** (this repository's target) hides it.

The other ten NOPs are the same in both.

## Not differences

- The device check (`tu_14f730.c`) and the C start-up code (`call_weak_fn`) are the same code.
  Their GOT offsets move by 4 because Onion imports one more function (`sigaction`).
- `GameDetailWindow::animateIn` gains one alignment NOP before its `double` constants.
