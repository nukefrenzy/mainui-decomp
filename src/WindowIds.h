// Window ids: the `id` of every TrimusWindow and the menu a MenuWindow shows (createMenu(id),
// tu_015b20/p11.inc). Main-menu icons carry the id of the menu they open, a window's id is
// saved in /tmp/state.json to reopen it after a launch, and the favorites file records the id
// of the list a game was added from. Only WIN_SETTING and WIN_FIXES are original names (they
// survive in log messages); the rest are named after what createMenu builds. Values only.
#pragma once

enum WindowId {
    WIN_MAIN = 0,            // the main menu: Recent, Favorites, Games, RetroArch, Apps, Settings
    WIN_GAMES = 1,           // emulator icons (one per Emu/ folder)
    WIN_FAVORITES = 2,       // /mnt/SDCARD/Roms/favourite.json
    WIN_APPS = 3,            // App/ folders
    WIN_FILES = 4,           // file browser on the SD card
    WIN_ROM_LIST = 5,        // one emulator's games
    WIN_OTHER = 6,           // titled "OTHER"; not built by createMenu
    WIN_SETTING = 7,         // the settings list (SettingMenu)
    WIN_GAME_INFO = 8,       // game detail window
    WIN_DEVICE_INFO = 9,
    WIN_RECENT = 10,
    WIN_HARDWARE_TEST = 11,
    WIN_ADC_INFO = 12,       // titled "ADC INFO"; not built by createMenu
    WIN_ENCDEC_TEST = 13,    // titled "EncDec Test"; not built by createMenu
    WIN_LANGUAGE = 14,
    WIN_RETROARCH = 16,      // RetroArch-app icons (one per RApp/ folder)
    WIN_RETROARCH_LIST = 17, // one RetroArch app's games
    WIN_COLOR = 18,          // display color settings
    WIN_THEME = 19,
    WIN_FIXES = 20,          // the "Fixes" settings (audio fix)
    WIN_WIFI = 21,
    WIN_KEYBOARD = 22,       // ImeWindow
    WIN_HOTSPOT = 23,
    WIN_SEARCH = 24,         // global search results
    WIN_TEST = 25,           // key setting and the hardware test screens
};
