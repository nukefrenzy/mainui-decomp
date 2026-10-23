// tu_14b860 (0x14b860-0x14cb24): system settings.
//
// /mnt/SDCARD/system.json holds the user's settings (volume, brightness, display colour, theme,
// language, button keymap, Wi-Fi switch, ...). loadSystemJson copies them into the memory that
// MainUI shares with the keymon daemon (see ShmVar.h), which applies them to the hardware;
// saveSystemJson writes the shared values back. The language, keymap and the Wi-Fi and hotspot
// switches are kept here in file-local variables with small get/set functions.
//
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include "PlatformApi.h"
#include "ShmVar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

// cJSON 1.7.10 (compiled into MainUI): the node layout and the type flags tested below.
typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
#define cJSON_Number 8
#define cJSON_String 16
cJSON *cJSON_Parse(const char *value);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
int cJSON_IsNumber(const cJSON *item);
void cJSON_Delete(cJSON *c);
cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
char *cJSON_Print(const cJSON *item);

// libshmvar; `shm` is MainUI's handle (g_keyShm), `key` a MonitorValue.
int SetKeyShm(void *shm, int key, int value);
int GetKeyShm(void *shm, int key);

static char g_language[64];         // 0x1825b8: language file name, "" = default
static char g_keymap[64];           // 0x1825f8: button keymap string, "" = default
static int g_wifiOn;                // 0x182638: Wi-Fi switch from system.json
static int g_hotspotOn;             // 0x18263c: hotspot (access-point mode) switch; not saved

// Size of an open file in bytes; leaves the position at the start.
long fileSize(FILE *f)          // 0x14b860
{
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    return size;
}

// Reads the whole file f into a new NUL-terminated buffer *out (the caller frees it); name is
// only used in error messages. Files must be under 128 KiB. Returns 0, or -1 on error.
int readWholeFile(const char *name, FILE *f, char **out)   // 0x14b8a4
{
    int i;
    int total = 0;
    int size = fileSize(f);
    if (size <= 0 || size >= 0x20000) {
        printf("%s invalid size %d\n", name, size);
        return -1;
    }
    *out = (char *)malloc(size + 1);
    // fread may return short counts; give it up to ten tries.
    for (i = 0; i < 10 && total < size; i++)
        total += fread(*out + total, 1, size - total, f);
    if (total != size) {
        printf("read %s (%d vs %d)error : %d:%s\n", name, total, size, errno, strerror(errno));
        return -1;
    }
    (*out)[total] = 0;
    return 0;
}

// The language file to load (system.json "language"), default English.
const char *getLanguageFile()   // 0x14ba18
{
    return g_language[0] == 0 ? "en.lang" : g_language;
}

// Sets the language file name; NULL selects the default.
void setLanguageFile(const char *lang)  // 0x14ba58
{
    if (lang) {
        printf("set language %s\n", lang);
        strncpy(g_language, lang, 64);
    } else {
        g_language[0] = 0;
    }
}

// The button keymap (system.json "keymap"): eight comma-separated button names that tell the
// kernel's button driver which physical button reports as which. KeySettingWindow edits it.
const char *getKeymap()         // 0x14bab8
{
    return g_keymap[0] == 0 ? "L2,L,R2,R,X,A,B,Y" : g_keymap;
}

// Sets the keymap string; NULL selects the default. Takes effect through setupKeymap.
void setKeymap(const char *keymap)  // 0x14baf8
{
    if (keymap) {
        printf("set keymap to %s\n", keymap);
        strncpy(g_keymap, keymap, 64);
    } else {
        puts("clear keymap");
        g_keymap[0] = 0;
    }
}

// Applies a keymap: hands the string (with its NUL) to the gpio_keys_polled kernel driver.
void setupKeymap(const char *keymap)    // 0x14bb64
{
    FILE *f = fopen("/sys/module/gpio_keys_polled/parameters/button_config", "w");
    if (f) {
        printf("setup_keymap %s\n", keymap);
        fwrite(keymap, 1, strlen(keymap) + 1, f);
        fclose(f);
    }
}

int getWifiOn()                 // 0x14bbdc
{
    return g_wifiOn;
}

// Wi-Fi (client) and hotspot exclude each other: turning Wi-Fi on turns the hotspot off.
void setWifiOn(int on)          // 0x14bc00
{
    g_wifiOn = on;
    if (g_wifiOn)
        g_hotspotOn = 0;
}

// The hotspot switch (HotspotMenu, tu_138c50.cpp).
int getHotspotOn()              // 0x14bc54
{
    return g_hotspotOn;
}

void setHotspotOn(int on)       // 0x14bc78
{
    g_hotspotOn = on;
}

// Loads /mnt/SDCARD/system.json into the shared settings. A missing or invalid key gets its
// default; display colour values must be 0-20. Also returns the theme path (a strdup, or NULL if
// unset or the folder no longer exists) and font size when theme/fontSize are not NULL. If the
// file cannot be read or parsed, every setting gets its default and the keymap is cleared.
void loadSystemJson(void *shm, char **theme, int *fontSize)    // 0x14bca8
{
    int failed = 1;
    FILE *f = fopen("/mnt/SDCARD/system.json", "r");
    if (f) {
        char *json = NULL;
        if (readWholeFile("/mnt/SDCARD/system.json", f, &json) >= 0) {
            cJSON *root = cJSON_Parse(json);
            if (root) {
                cJSON *vol = cJSON_GetObjectItem(root, "vol");
                if (vol && vol->type == cJSON_Number)
                    SetKeyShm(shm, MONITOR_VOLUME, vol->valueint);
                else
                    SetKeyShm(shm, MONITOR_VOLUME, 20);
                cJSON *keymap = cJSON_GetObjectItem(root, "keymap");
                if (keymap && keymap->type == cJSON_String)
                    setKeymap(keymap->valuestring);
                // Original bug, kept as-is: the mute flag is taken from "vol", not "mute" (and
                // a file with "mute" but no "vol" would dereference NULL).
                cJSON *mute = cJSON_GetObjectItem(root, "mute");
                if (mute && mute->type == cJSON_Number)
                    SetKeyShm(shm, MONITOR_MUTE, vol->valueint != 0);
                else
                    SetKeyShm(shm, MONITOR_MUTE, 0);
                cJSON *bgmvol = cJSON_GetObjectItem(root, "bgmvol");
                if (bgmvol && bgmvol->type == cJSON_Number)
                    SetKeyShm(shm, MONITOR_BGM_VOLUME, bgmvol->valueint);
                else
                    SetKeyShm(shm, MONITOR_BGM_VOLUME, 8);
                cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
                if (brightness && brightness->type == cJSON_Number)
                    SetKeyShm(shm, MONITOR_BRIGHTNESS, brightness->valueint);
                else
                    SetKeyShm(shm, MONITOR_BRIGHTNESS, 7);
                cJSON *hibernate = cJSON_GetObjectItem(root, "hibernate");
                if (hibernate && hibernate->type == cJSON_Number) {
                    SetKeyShm(shm, MONITOR_HIBERNATE_DELAY, hibernate->valueint);
                    printf("set hibernate to %d\n", hibernate->valueint);
                } else {
                    SetKeyShm(shm, MONITOR_HIBERNATE_DELAY, 0);
                    printf("set hibernate to default %d\n", 0);
                }
                cJSON *language = cJSON_GetObjectItem(root, "language");
                if (language && language->type == cJSON_String)
                    setLanguageFile(language->valuestring);
                cJSON *lumination = cJSON_GetObjectItem(root, "lumination");
                if (lumination && lumination->type == cJSON_Number && lumination->valueint >= 0 && lumination->valueint <= 20) {
                    printf("set lumination to %d\n", lumination->valueint);
                    SetKeyShm(shm, MONITOR_LUMINATION, lumination->valueint);
                } else {
                    SetKeyShm(shm, MONITOR_LUMINATION, 7);
                }
                cJSON *hue = cJSON_GetObjectItem(root, "hue");
                if (hue && hue->type == cJSON_Number && hue->valueint >= 0 && hue->valueint <= 20)
                    SetKeyShm(shm, MONITOR_HUE, hue->valueint);
                else
                    SetKeyShm(shm, MONITOR_HUE, 10);
                cJSON *saturation = cJSON_GetObjectItem(root, "saturation");
                if (saturation && saturation->type == cJSON_Number && saturation->valueint >= 0 && saturation->valueint <= 20)
                    SetKeyShm(shm, MONITOR_SATURATION, saturation->valueint);
                else
                    SetKeyShm(shm, MONITOR_SATURATION, 10);
                cJSON *contrast = cJSON_GetObjectItem(root, "contrast");
                if (contrast && contrast->type == cJSON_Number && contrast->valueint >= 0 && contrast->valueint <= 20)
                    SetKeyShm(shm, MONITOR_CONTRAST, contrast->valueint);
                else
                    SetKeyShm(shm, MONITOR_CONTRAST, 10);
                cJSON *themeItem = cJSON_GetObjectItem(root, "theme");
                if (theme) {
                    if (themeItem && themeItem->type == cJSON_String) {
                        if (access(themeItem->valuestring, F_OK) == 0)
                            *theme = strdup(themeItem->valuestring);
                        else
                            *theme = NULL;
                    } else {
                        *theme = NULL;
                    }
                }
                cJSON *fontsize = cJSON_GetObjectItem(root, "fontsize");
                if (fontSize && fontsize && fontsize->type == cJSON_Number)
                    *fontSize = fontsize->valueint;
                cJSON *audiofix = cJSON_GetObjectItem(root, "audiofix");
                if (audiofix && audiofix->type == cJSON_Number)
                    SetKeyShm(shm, MONITOR_AUDIOFIX, audiofix->valueint);
                else
                    SetKeyShm(shm, MONITOR_AUDIOFIX, 1);
                cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
                if (wifi && wifi->type == cJSON_Number)
                    g_wifiOn = wifi->valueint;
                cJSON_Delete(root);
                failed = 0;
            }
        }
        if (json)
            free(json);
        fclose(f);
    }
    if (failed) {
        // Defaults (the font size default, 24, is only applied here).
        SetKeyShm(shm, MONITOR_VOLUME, 20);
        SetKeyShm(shm, MONITOR_MUTE, 0);
        SetKeyShm(shm, MONITOR_BGM_VOLUME, 8);
        SetKeyShm(shm, MONITOR_BRIGHTNESS, 7);
        SetKeyShm(shm, MONITOR_HIBERNATE_DELAY, 0);
        SetKeyShm(shm, MONITOR_LUMINATION, 7);
        SetKeyShm(shm, MONITOR_HUE, 10);
        SetKeyShm(shm, MONITOR_SATURATION, 10);
        SetKeyShm(shm, MONITOR_CONTRAST, 10);
        SetKeyShm(shm, MONITOR_AUDIOFIX, 1);
        if (fontSize)
            *fontSize = 24;
        printf("set hibernate to default %d\n", 0);
        setKeymap(NULL);
    }
}

// Reads only the theme path and font size from /mnt/SDCARD/system.json, with the same rules as
// loadSystemJson; leaves both untouched if the file cannot be read.
void loadThemeAndFontSize(char **theme, int *fontSize)     // 0x14c460
{
    FILE *f = fopen("/mnt/SDCARD/system.json", "r");
    if (f) {
        char *json = NULL;
        if (readWholeFile("/mnt/SDCARD/system.json", f, &json) >= 0) {
            cJSON *root = cJSON_Parse(json);
            if (root) {
                cJSON *themeItem = cJSON_GetObjectItem(root, "theme");
                if (theme) {
                    if (themeItem && themeItem->type == cJSON_String) {
                        if (access(themeItem->valuestring, F_OK) == 0)
                            *theme = strdup(themeItem->valuestring);
                        else
                            *theme = NULL;
                    } else {
                        *theme = NULL;
                    }
                }
                cJSON *fontsize = cJSON_GetObjectItem(root, "fontsize");
                if (fontSize && fontsize && fontsize->type == cJSON_Number)
                    *fontSize = fontsize->valueint;
                cJSON_Delete(root);
            }
        }
        if (json)
            free(json);
        fclose(f);
    }
}

// Writes the shared settings, keymap, language, theme, font size and Wi-Fi switch to
// /mnt/SDCARD/system.json, then syncs the SD card. Nothing is written if any of the main
// values could not be created.
// Original behaviour, kept as-is: the file is opened without O_TRUNC, so if the new text is
// shorter than the old file, the old file's tail remains after it.
void saveSystemJson(void *shm, const char *theme, int fontSize)    // 0x14c5fc
{
    cJSON *themeItem;
    int fd = open("/mnt/SDCARD/system.json", O_RDWR | O_CREAT, 0755);
    if (fd >= 0) {
        cJSON *root = cJSON_CreateObject();
        if (root) {
            cJSON *vol = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_VOLUME));
            cJSON *keymap = cJSON_CreateString(getKeymap());
            cJSON *mute = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_MUTE) ? 1 : 0);
            cJSON *bgmvol = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_BGM_VOLUME));
            cJSON *brightness = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_BRIGHTNESS));
            cJSON *hibernate = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_HIBERNATE_DELAY));
            cJSON *language = cJSON_CreateString(getLanguageFile());
            cJSON *lumination = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_LUMINATION));
            cJSON *hue = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_HUE));
            cJSON *saturation = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_SATURATION));
            cJSON *contrast = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_CONTRAST));
            cJSON *audiofix = cJSON_CreateNumber(GetKeyShm(shm, MONITOR_AUDIOFIX));
            themeItem = NULL;
            cJSON *fontsize = cJSON_CreateNumber(fontSize);
            cJSON *wifi = cJSON_CreateNumber(getWifiOn());
            if (theme)
                themeItem = cJSON_CreateString(theme);
            if (vol && keymap && mute && bgmvol && brightness && language && lumination && hue && saturation && contrast) {
                // root takes ownership of every item; a NULL theme is simply skipped by cJSON.
                cJSON_AddItemToObject(root, "vol", vol);
                cJSON_AddItemToObject(root, "keymap", keymap);
                cJSON_AddItemToObject(root, "mute", mute);
                cJSON_AddItemToObject(root, "bgmvol", bgmvol);
                cJSON_AddItemToObject(root, "brightness", brightness);
                cJSON_AddItemToObject(root, "language", language);
                cJSON_AddItemToObject(root, "hibernate", hibernate);
                cJSON_AddItemToObject(root, "lumination", lumination);
                cJSON_AddItemToObject(root, "hue", hue);
                cJSON_AddItemToObject(root, "saturation", saturation);
                cJSON_AddItemToObject(root, "contrast", contrast);
                cJSON_AddItemToObject(root, "theme", themeItem);
                cJSON_AddItemToObject(root, "fontsize", fontsize);
                cJSON_AddItemToObject(root, "audiofix", audiofix);
                cJSON_AddItemToObject(root, "wifi", wifi);
                char *text = cJSON_Print(root);
                write(fd, text, strlen(text));
                free(text);
            } else {
                // Free whatever was created (the original misses "hibernate" here).
                if (vol)
                    cJSON_Delete(vol);
                if (keymap)
                    cJSON_Delete(keymap);
                if (mute)
                    cJSON_Delete(mute);
                if (bgmvol)
                    cJSON_Delete(bgmvol);
                if (brightness)
                    cJSON_Delete(brightness);
                if (language)
                    cJSON_Delete(language);
                if (lumination)
                    cJSON_Delete(lumination);
                if (hue)
                    cJSON_Delete(hue);
                if (saturation)
                    cJSON_Delete(saturation);
                if (contrast)
                    cJSON_Delete(contrast);
                if (themeItem)
                    cJSON_Delete(themeItem);
                if (fontsize)
                    cJSON_Delete(fontsize);
                if (audiofix)
                    cJSON_Delete(audiofix);
                if (wifi)
                    cJSON_Delete(wifi);
            }
            cJSON_Delete(root);
        }
        close(fd);
        sync();
    }
}
