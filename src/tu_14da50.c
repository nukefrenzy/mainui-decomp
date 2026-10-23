// tu_14da50 (0x14da50-0x14dc18): the built-in English strings, and a reader for <lang>.short files.
//
// g_defaultStrings holds the English text of all 156 user-interface strings (ids: LangStrings.h);
// MainUI falls back to it when no language file can be read. The three functions read the
// reduced <lang>.short files that genShortLangFile (tu_134080.cpp) writes for ids 88-155, the
// in-game menu's strings ("Continue", "Save", "Load", "Exit Game", ...). Nothing in MainUI calls
// them; they are presumably there for another program that shares this code.
// Its .rodata starts with the table's strings (0x163e5c).
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "LangStrings.h"

typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
cJSON *cJSON_Parse(const char *value);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
void cJSON_Delete(cJSON *c);
char *cJSON_GetStringValue(cJSON *item);

int readWholeFile(const char *name, FILE *f, char **out);  // 0x14b8a4

char g_langPath[128];           // 0x182680, COMMON: scratch buffer for a path or a key
char g_unrefCommon_182700[48];  // 0x182700, COMMON: unreferenced storage at this address in the Onion and stock builds; size and position proven, name, type and partition unknown; hashes after g_langPath
const char *g_defaultStrings[156] = { // 0x17fc9c, built-in English strings
    "RetroArch", // 0
    "Favorite", // 1
    "Game", // 2
    "ARCADE", // 3
    "NEOGEO", // 4
    "FC", // 5
    "SFC", // 6
    "GB", // 7
    "GBA", // 8
    "MD", // 9
    "GPSP", // 10
    "SMS", // 11
    "PS", // 12
    "PCE", // 13
    "OTHER", // 14
    "Setting", // 15
    "GAME INFO", // 16
    "DEVICE INFO", // 17
    "Recent", // 18
    "FILE", // 19
    "ADC INFO", // 20
    "EncDec Test", // 21
    "Keys Map", // 22
    "Language", // 23
    "Volume", // 24
    "BGM Volume", // 25
    "Brightness", // 26
    "Refresh Roms", // 27
    "Factory Reset", // 28
    "Flash Mode", // 29
    "Device Info", // 30
    "Hardware Test", // 31
    "ADC Info", // 32
    "Status:", // 33
    "Delay:", // 34
    "OK Rate:", // 35
    "Fail Rate:", // 36
    "STOPPED", // 37
    "RUNNING", // 38
    "ADC:", // 39
    "Charging", // 40
    "OTG", // 41
    "Disconnected", // 42
    "Error", // 43
    "favorite", // 44
    "Cancel", // 45
    "OK", // 46
    "seconds", // 47
    "in", // 48
    "PLayList", // 49
    "Clear", // 50
    "Open", // 51
    "Remove", // 52
    "Refresh", // 53
    "Refresh Roms", // 54
    "Add Favorite", // 55
    "Upgrade", // 56
    "Update system?", // 57
    "SD space low\n%dM left", // 58
    "Mode", // 59
    "Serial", // 60
    "Version", // 61
    "CPU", // 62
    "%dMHz", // 63
    "Memory", // 64
    "%dM", // 65
    "SD Card", // 66
    "Resume Default Setting", // 67
    "Resume all system\nsettings.TF card content\nwill not be deleted", // 68
    "refresh roms on TF card", // 69
    "Reset", // 70
    "default", // 71
    "rotate90", // 72
    "map3", // 73
    "unknown", // 74
    "LOADING", // 75
    "Loading", // 76
    "Clear List", // 77
    "Clear recent game list,\nTF card content will\nnot be deleted", // 78
    "Clear Favorite", // 79
    "Clear all game from this\nlist, TF card content will\nnot be deleted", // 80
    "Remove Favorite", // 81
    "Remove game from this\nlist, TF card content will\nnot be deleted", // 82
    "Scan Games", // 83
    "GAMEINFO", // 84
    "Create Cache", // 85
    "Power off", // 86
    "Power off in %d seconds", // 87
    "OPEN", // 88
    "BACK", // 89
    "NAV", // 90
    "MENU", // 91
    "Continue", // 92
    "Save", // 93
    "Load", // 94
    "Volume", // 95
    "Exit Game", // 96
    "Record %d/%d", // 97
    "Empty", // 98
    "No Thumb", // 99
    "load fail", // 100
    "save fail", // 101
    "slot%d saved", // 102
    "Fullscreen", // 103
    "On", // 104
    "Off", // 105
    "Native Menu", // 106
    "App", // 107
    "Shut down your device?", // 108
    "IO Test", // 109
    "Key Setting", // 110
    "Exit", // 111
    "SAVE AND EXIT", // 112
    "%dm later", // 113
    "Never", // 114
    "Hibernate", // 115
    "Remove ROM", // 116
    "Remove ROM from this list?", // 117
    "Color", // 118
    "Lightness", // 119
    "Hue", // 120
    "Saturation", // 121
    "Contrast", // 122
    "Reboot to apply changes", // 123
    "Apply Changes", // 124
    "Themes", // 125
    "Font Size", // 126
    "Default", // 127
    "Delete", // 128
    "Audio Fix", // 129
    "Fixes", // 130
    "WIFI", // 131
    "ACTIVITY", // 132
    "Connect", // 133
    "Hotspot", // 134
    "SSID", // 135
    "Password", // 136
    "Bluetooth", // 137
    "Netplay", // 138
    "Reset Bluetooth Service", // 139
    "Scanning", // 140
    "No Device Found", // 141
    "Bluetooth Not Ready", // 142
    "Put roms into following directory", // 143
    "Bluetooth Reset", // 144
    "Hotspot is on", // 145
    "CPU Frequency", // 146
    "Connected", // 147
    "My Device", // 148
    "Other Device", // 149
    "Info", // 150
    "Input SSID", // 151
    "Input Password", // 152
    "Search", // 153
    "Fail to connect\nCheck your password\nOr reboot router", // 154
    "Website", // 155
};

// Loads <lang>.short (from the SD card's miyoo354/app/lang if present, else the firmware's
// /customer/app/lang) as a cJSON object mapping "88".."155" to text; NULL if unreadable.
cJSON *loadLanguageStrings(const char *lang)    // 0x14da50
{
    cJSON *strings = NULL;
    if (access("/mnt/SDCARD/miyoo354/app/lang", F_OK) == 0)
        snprintf(g_langPath, 128, "%s/%s.short", "/mnt/SDCARD/miyoo354/app/lang", lang);
    else
        snprintf(g_langPath, 128, "%s/%s.short", "/customer/app/lang", lang);
    FILE *f = fopen(g_langPath, "r");
    if (f) {
        char *json;
        if (readWholeFile(g_langPath, f, &json) >= 0) {
            strings = cJSON_Parse(json);
            free(json);
        }
        fclose(f);
    }
    return strings;
}

void freeLanguageStrings(cJSON *strings)    // 0x14db54
{
    if (strings)
        cJSON_Delete(strings);
}

// One string by id: from the loaded file if there is one (NULL if it lacks the id), else the
// English table. Out-of-range ids give "wrongbeef"; negative ids are not checked.
const char *getLanguageString(cJSON *strings, int id)   // 0x14db84
{
    if (id >= STR_COUNT)
        return "wrongbeef";
    if (strings) {
        snprintf(g_langPath, 128, "%d", id);
        cJSON *item = cJSON_GetObjectItem(strings, g_langPath);
        return cJSON_GetStringValue(item);
    }
    return g_defaultStrings[id];
}
