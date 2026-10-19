// tu_14b860: system settings (0x14b860-0x14cb24): whole-file reads, language and keymap names,
// the Wi-Fi switch, and /mnt/SDCARD/system.json load/save through the shared settings memory.
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

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
int cJSON_IsNumber(const cJSON *item);
void cJSON_Delete(cJSON *c);
cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
char *cJSON_Print(const cJSON *item);

int SetKeyShm(void *shm, int key, int value);
int GetKeyShm(void *shm, int key);

static char g_language[64];         // 0x1825b8
static char g_keymap[64];           // 0x1825f8
static int g_wifiOn;                // 0x182638
static int g_wifiConnected;         // 0x18263c

long fileSize(FILE *f)          // 0x14b860
{
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    return size;
}

// Reads the whole file f (named name, for messages) into a new NUL-terminated buffer *out.
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
    for (i = 0; i < 10 && total < size; i++)
        total += fread(*out + total, 1, size - total, f);
    if (total != size) {
        printf("read %s (%d vs %d)error : %d:%s\n", name, total, size, errno, strerror(errno));
        return -1;
    }
    (*out)[total] = 0;
    return 0;
}

const char *getLanguageFile()   // 0x14ba18
{
    return g_language[0] == 0 ? "en.lang" : g_language;
}

void setLanguageFile(const char *lang)  // 0x14ba58
{
    if (lang) {
        printf("set language %s\n", lang);
        strncpy(g_language, lang, 64);
    } else {
        g_language[0] = 0;
    }
}

const char *getKeymap()         // 0x14bab8
{
    return g_keymap[0] == 0 ? "L2,L,R2,R,X,A,B,Y" : g_keymap;
}

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

void setWifiOn(int on)          // 0x14bc00
{
    g_wifiOn = on;
    if (g_wifiOn)
        g_wifiConnected = 0;
}

int getWifiConnected()          // 0x14bc54
{
    return g_wifiConnected;
}

void setWifiConnected(int connected)    // 0x14bc78
{
    g_wifiConnected = connected;
}

// Loads /mnt/SDCARD/system.json into the shared settings, with defaults for missing keys.
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
                if (vol && vol->type == 8)
                    SetKeyShm(shm, 0, vol->valueint);
                else
                    SetKeyShm(shm, 0, 20);
                cJSON *keymap = cJSON_GetObjectItem(root, "keymap");
                if (keymap && keymap->type == 16)
                    setKeymap(keymap->valuestring);
                cJSON *mute = cJSON_GetObjectItem(root, "mute");
                if (mute && mute->type == 8)
                    SetKeyShm(shm, 3, vol->valueint != 0);
                else
                    SetKeyShm(shm, 3, 0);
                cJSON *bgmvol = cJSON_GetObjectItem(root, "bgmvol");
                if (bgmvol && bgmvol->type == 8)
                    SetKeyShm(shm, 5, bgmvol->valueint);
                else
                    SetKeyShm(shm, 5, 8);
                cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
                if (brightness && brightness->type == 8)
                    SetKeyShm(shm, 1, brightness->valueint);
                else
                    SetKeyShm(shm, 1, 7);
                cJSON *hibernate = cJSON_GetObjectItem(root, "hibernate");
                if (hibernate && hibernate->type == 8) {
                    SetKeyShm(shm, 6, hibernate->valueint);
                    printf("set hibernate to %d\n", hibernate->valueint);
                } else {
                    SetKeyShm(shm, 6, 0);
                    printf("set hibernate to default %d\n", 0);
                }
                cJSON *language = cJSON_GetObjectItem(root, "language");
                if (language && language->type == 16)
                    setLanguageFile(language->valuestring);
                cJSON *lumination = cJSON_GetObjectItem(root, "lumination");
                if (lumination && lumination->type == 8 && lumination->valueint >= 0 && lumination->valueint <= 20) {
                    printf("set lumination to %d\n", lumination->valueint);
                    SetKeyShm(shm, 8, lumination->valueint);
                } else {
                    SetKeyShm(shm, 8, 7);
                }
                cJSON *hue = cJSON_GetObjectItem(root, "hue");
                if (hue && hue->type == 8 && hue->valueint >= 0 && hue->valueint <= 20)
                    SetKeyShm(shm, 9, hue->valueint);
                else
                    SetKeyShm(shm, 9, 10);
                cJSON *saturation = cJSON_GetObjectItem(root, "saturation");
                if (saturation && saturation->type == 8 && saturation->valueint >= 0 && saturation->valueint <= 20)
                    SetKeyShm(shm, 10, saturation->valueint);
                else
                    SetKeyShm(shm, 10, 10);
                cJSON *contrast = cJSON_GetObjectItem(root, "contrast");
                if (contrast && contrast->type == 8 && contrast->valueint >= 0 && contrast->valueint <= 20)
                    SetKeyShm(shm, 11, contrast->valueint);
                else
                    SetKeyShm(shm, 11, 10);
                cJSON *themeItem = cJSON_GetObjectItem(root, "theme");
                if (theme) {
                    if (themeItem && themeItem->type == 16) {
                        if (access(themeItem->valuestring, F_OK) == 0)
                            *theme = strdup(themeItem->valuestring);
                        else
                            *theme = NULL;
                    } else {
                        *theme = NULL;
                    }
                }
                cJSON *fontsize = cJSON_GetObjectItem(root, "fontsize");
                if (fontSize && fontsize && fontsize->type == 8)
                    *fontSize = fontsize->valueint;
                cJSON *audiofix = cJSON_GetObjectItem(root, "audiofix");
                if (audiofix && audiofix->type == 8)
                    SetKeyShm(shm, 13, audiofix->valueint);
                else
                    SetKeyShm(shm, 13, 1);
                cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
                if (wifi && wifi->type == 8)
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
        SetKeyShm(shm, 0, 20);
        SetKeyShm(shm, 3, 0);
        SetKeyShm(shm, 5, 8);
        SetKeyShm(shm, 1, 7);
        SetKeyShm(shm, 6, 0);
        SetKeyShm(shm, 8, 7);
        SetKeyShm(shm, 9, 10);
        SetKeyShm(shm, 10, 10);
        SetKeyShm(shm, 11, 10);
        SetKeyShm(shm, 13, 1);
        if (fontSize)
            *fontSize = 24;
        printf("set hibernate to default %d\n", 0);
        setKeymap(NULL);
    }
}

// Reads theme and font size from /mnt/SDCARD/system.json.
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
                    if (themeItem && themeItem->type == 16) {
                        if (access(themeItem->valuestring, F_OK) == 0)
                            *theme = strdup(themeItem->valuestring);
                        else
                            *theme = NULL;
                    } else {
                        *theme = NULL;
                    }
                }
                cJSON *fontsize = cJSON_GetObjectItem(root, "fontsize");
                if (fontSize && fontsize && fontsize->type == 8)
                    *fontSize = fontsize->valueint;
                cJSON_Delete(root);
            }
        }
        if (json)
            free(json);
        fclose(f);
    }
}

// Writes the shared settings back to /mnt/SDCARD/system.json.
void saveSystemJson(void *shm, const char *theme, int fontSize)    // 0x14c5fc
{
    cJSON *themeItem;
    int fd = open("/mnt/SDCARD/system.json", O_RDWR | O_CREAT, 0755);
    if (fd >= 0) {
        cJSON *root = cJSON_CreateObject();
        if (root) {
            cJSON *vol = cJSON_CreateNumber(GetKeyShm(shm, 0));
            cJSON *keymap = cJSON_CreateString(getKeymap());
            cJSON *mute = cJSON_CreateNumber(GetKeyShm(shm, 3) ? 1 : 0);
            cJSON *bgmvol = cJSON_CreateNumber(GetKeyShm(shm, 5));
            cJSON *brightness = cJSON_CreateNumber(GetKeyShm(shm, 1));
            cJSON *hibernate = cJSON_CreateNumber(GetKeyShm(shm, 6));
            cJSON *language = cJSON_CreateString(getLanguageFile());
            cJSON *lumination = cJSON_CreateNumber(GetKeyShm(shm, 8));
            cJSON *hue = cJSON_CreateNumber(GetKeyShm(shm, 9));
            cJSON *saturation = cJSON_CreateNumber(GetKeyShm(shm, 10));
            cJSON *contrast = cJSON_CreateNumber(GetKeyShm(shm, 11));
            cJSON *audiofix = cJSON_CreateNumber(GetKeyShm(shm, 13));
            themeItem = NULL;
            cJSON *fontsize = cJSON_CreateNumber(fontSize);
            cJSON *wifi = cJSON_CreateNumber(getWifiOn());
            if (theme)
                themeItem = cJSON_CreateString(theme);
            if (vol && keymap && mute && bgmvol && brightness && language && lumination && hue && saturation && contrast) {
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
