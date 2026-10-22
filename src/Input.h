// MainUI's input vocabulary: the SDL 1.2 event types and key symbols it receives, the Miyoo
// buttons behind them, the button codes windows see, and the user-event codes that MainUI's own
// threads post to the window loop. Values only (enums), so C and C++ units can share this header
// and no generated code changes.
#pragma once

// SDL 1.2 event types (SDL_events.h) that MainUI handles.
enum {
    SDL_KEYDOWN = 2,
    SDL_KEYUP = 3,
    SDL_MOUSEBUTTONUP = 6,   // a tap on the touch panel (devices that have one)
    SDL_USEREVENT = 24,      // posted by MainUI itself; event.user.code holds an EVENT_* below
};

// The Miyoo buttons as SDL 1.2 key symbols (SDL_keysym.h values), as the Miyoo SDL port delivers
// them (Onion: src/common/system/keymap_sw.h). The shoulder buttons assume the default keymap,
// which the kernel driver applies (see setupKeymap in tu_14b860.c).
enum MiyooKey {
    MIYOO_KEY_R2 = 8,          // SDLK_BACKSPACE
    MIYOO_KEY_L2 = 9,          // SDLK_TAB
    MIYOO_KEY_START = 13,      // SDLK_RETURN
    MIYOO_KEY_MENU = 27,       // SDLK_ESCAPE
    MIYOO_KEY_A = 32,          // SDLK_SPACE
    MIYOO_KEY_L1 = 101,        // SDLK_e
    MIYOO_KEY_R1 = 116,        // SDLK_t
    MIYOO_KEY_UP = 273,        // SDLK_UP
    MIYOO_KEY_DOWN = 274,      // SDLK_DOWN
    MIYOO_KEY_RIGHT = 275,     // SDLK_RIGHT
    MIYOO_KEY_LEFT = 276,      // SDLK_LEFT
    MIYOO_KEY_X = 304,         // SDLK_LSHIFT
    MIYOO_KEY_SELECT = 305,    // SDLK_RCTRL
    MIYOO_KEY_B = 306,         // SDLK_LCTRL
    MIYOO_KEY_Y = 308,         // SDLK_LALT
    MIYOO_KEY_LSUPER = 311,    // SDLK_LSUPER: not a Miyoo button; ignored
    MIYOO_KEY_RSUPER = 312,    // SDLK_RSUPER: not a Miyoo button; ignored
    MIYOO_KEY_POWER = 320,     // SDLK_POWER; repeats while held
};

// Button codes returned by handleGlobalInput and passed to every window's input handling.
// The original names survive as strings in keyEventName (0x15b20): "key_a", "key_hotkey1", ...
enum MainButton {
    BUTTON_NONE = 0,
    BUTTON_A = 1,
    BUTTON_B = 2,
    BUTTON_X = 3,
    BUTTON_Y = 4,
    BUTTON_UP = 5,
    BUTTON_DOWN = 6,
    BUTTON_LEFT = 7,
    BUTTON_RIGHT = 8,
    BUTTON_SELECT = 9,     // keyEventName says "key_start", but handleGlobalInput logs KEY_SELECT
    BUTTON_START = 10,     // keyEventName says "key_coin" (arcade naming); logged as KEY_START
    BUTTON_MENU = 11,      // "key_nenu_key" (sic)
    BUTTON_HOTKEY1 = 12,   // L2
    BUTTON_HOTKEY2 = 13,   // R2
    BUTTON_HOTKEY3 = 14,   // L1
    BUTTON_HOTKEY4 = 15,   // R1
};

// User-event codes (SDL_USEREVENT, event.user.code). handleGlobalInput returns them to the
// window like button codes, so they share one number space.
enum UserEventCode {
    EVENT_BATTERY = 18,         // battery level or charger state changed (status thread)
    EVENT_HOTPLUG = 19,         // an audio device was plugged or unplugged: reopen audio
    EVENT_NOTIFY = 20,          // "event_notify" in keyEventName
    EVENT_POWER_OFF = 21,       // power-off requested (power-off dialog confirmed)
    EVENT_POWER_DIALOG = 27,    // returned (not posted) when holding power opened the power-off dialog
    EVENT_WIFI_STATUS = 31,     // Wi-Fi mode or connection state changed (status thread)
    EVENT_VOLUME_SHOWN = 40,    // volume keys used: show the volume indicator
    EVENT_VOLUME_HIDDEN = 41,   // 3 s without volume change: hide it
};
