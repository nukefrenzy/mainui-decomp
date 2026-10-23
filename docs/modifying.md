# Modifying MainUI

The source in `src/` builds into the exact original binary. You can also change it and build a
modified MainUI, for example to fix one of the [original bugs](behavior.md#original-quirks-and-bugs)
or to add a feature for an Onion release. This page shows how.

## Build a modified MainUI

1. Build the unmodified source once, so every input is downloaded and you know your setup works
   ([building.md](building.md)).
2. Edit the source in `src/`. [source-guide.md](source-guide.md) shows where things are.
3. Build:

   ```bash
   scripts/build-modified.sh
   ```

   It uses the same compilers, flags, libraries and link as the byte-identical build. Compile and
   link errors still stop it. The checks that compare bytes with the original are reported but
   not enforced, since a change is supposed to change the bytes. The result is
   `build/MainUI-modified`, and the script says whether it still equals the original.

To see exactly what your change did to the machine code, check the file you edited. It reports
the first difference, with a disassembly of both sides:

```bash
python3 tools/unitcheck.py src/tu_14b860.c
```

## Try it on a device

Keep a backup and install the build as a separate file, so you can always go back.

Onion's `runtime.sh` bind-mounts `.tmp_update/bin/MainUI-354-clean` from the SD card over
`miyoo/app/MainUI` when the device starts. When `.tmp_update/config/.showExpert` exists, it uses
the Expert build instead.

1. Copy the original `.tmp_update/bin/MainUI-354-clean` somewhere safe.
2. Copy `build/MainUI-modified` over it.
3. Check the copy with `sha256sum`.
4. Restart the device.
5. To go back, copy the original back.

## Things to know before you change code

- **Any C++ that GCC 8.2 accepts works.** Matching the original's `-O0` style only matters while
  you want identical bytes. Once a file is changed, write it the way you like.
- **Class layouts are shared.** Windows, menus and actions are used across files through the
  headers. Adding a member to a class changes its size and offsets everywhere, so rebuild
  everything; `build-modified.sh` always does.
- **Fixed addresses no longer apply.** The addresses in comments and in `ghidra/names.tsv` describe
  the original binary only. A modified build places functions differently.
- **Some of Onion's patches are written as assembly.** The calls Onion turned into NOPs
  (`createMenu`, `drawText`, `drawWifiIndicator`, `initBacklightPwm`) are produced by an `asm()`
  block before each function. That block replaces `bl` instructions at listed offsets from the
  function start. To restore a call, remove its offset from the list and lower the count that the
  block checks at the end of the function. If you change code earlier in that function, the
  offsets move, so it's often simpler to delete the block and remove the calls you don't want in
  C++. [onion-changes.md](onion-changes.md) lists every patch.
- **Strings Onion overwrote** keep their original slot size, so you can overwrite them again
  without moving anything. In normal code you can also just change the string.

## Where to make common changes

| Goal | Where |
| --- | --- |
| Global hotkeys | `handleGlobalInput` ([p01.inc](../src/tu_015b20/p01.inc)): every key press of every window passes through it |
| How games and apps launch | `launchRom` and `launchApp_buildCommand` (p01.inc). Or leave MainUI alone and change what Onion's runtime script does with `/tmp/cmd_to_run.sh`. |
| Status bar icons or an overlay | `runWindowLoop` ([p11.inc](../src/tu_015b20/p11.inc)) draws the indicators, then flips; draw onto `g_backBuffer` before the flip |
| A main menu icon | `createMenu`, case `WIN_MAIN` (p11.inc): add an `IconMenuItem` with the window id it opens |
| A Settings row | `createMenu`, case `WIN_SETTING`: add a `SettingMenuItem` with a new `SettingHandler` value, and handle it in `SettingMenu::select` / `handleKey` ([p07.inc](../src/tu_015b20/p07.inc)) |
| A new screen | Derive from `TrimusWindow` ([TrimusWindow.h](../src/TrimusWindow.h)) or `MenuWindow`, and open it with `pushWindow` |
| Settings shared with keymon | [ShmVar.h](../src/ShmVar.h) and `GetKeyShm`/`SetKeyShm`; saved by `saveSystemJson` ([tu_14b860.c](../src/tu_14b860.c)) |
| Reacting to battery, Wi-Fi or volume changes | the status thread in [tu_049c3c.cpp](../src/tu_049c3c.cpp) posts user events, and windows receive them through `handleGlobalInput` |

## Without rebuilding: an LD_PRELOAD library

For small additions you can leave MainUI untouched and load a shared library into it with
`LD_PRELOAD`. The library can wrap any function MainUI imports (`SDL_PollEvent`,
`SDL_UpperBlit`, `fopen`, `system`, …). Build it with the same Arm GNU 8.2 toolchain.

## Keep the repository clean

The built binary (original or modified) contains Miyoo's program. Never commit it here. It stays
in `build/`, which git ignores. Changes to the source are welcome as pull requests; see
[CONTRIBUTING.md](../CONTRIBUTING.md). Note that this repository's own goal is to stay
byte-identical to the original, so it accepts readability improvements, not behavior changes.
