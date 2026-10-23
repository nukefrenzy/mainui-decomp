// Settings that MainUI shares with the keymon daemon through libshmvar (System V shared memory,
// key "KMON"). keymon applies them to the hardware (volume, backlight, display color) and reports
// back through a few of them (volume changed, battery level). MainUI reads and writes them with
// GetKeyShm/SetKeyShm, passing the libshmvar handle it keeps in g_keyShm.
// Key names and order are libshmvar's own (Onion: include/shmvar/shmvar.h). Values only, so C and
// C++ units can share this header without changing any code.
#pragma once

enum MonitorValue {
    MONITOR_VOLUME,           // 0: speaker volume, system.json "vol"
    MONITOR_BRIGHTNESS,       // 1: backlight level, "brightness"
    MONITOR_KEYMAP,           // 2: (unused by MainUI)
    MONITOR_MUTE,             // 3: mute flag, "mute"
    MONITOR_VOLUME_CHANGED,   // 4: set by keymon when the volume keys were used; MainUI clears it
    MONITOR_BGM_VOLUME,       // 5: menu background-music volume, "bgmvol"
    MONITOR_HIBERNATE_DELAY,  // 6: sleep timeout setting, "hibernate"
    MONITOR_ADC_VALUE,        // 7: battery level reported by keymon
    MONITOR_LUMINATION,       // 8: display luminance 0-20, "lumination"
    MONITOR_HUE,              // 9: display hue 0-20, "hue"
    MONITOR_SATURATION,       // 10: display saturation 0-20, "saturation"
    MONITOR_CONTRAST,         // 11: display contrast 0-20, "contrast"
    MONITOR_UNUSED,           // 12
    MONITOR_AUDIOFIX,         // 13: audio pop/latency workaround flag, "audiofix"
};
