// tu_14da50: UI language strings (0x14da50-0x14dc18): <lang>.short files as cJSON, falling back
// to the built-in English table. Its .rodata starts with that table's strings (0x163e5c).
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static char g_langPath[128];    // 0x182680
extern const char *g_defaultStrings[];  // 0x17fc9c, built-in English strings

// Loads the language file <lang>.short as a cJSON object of id -> string.
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

const char *getLanguageString(cJSON *strings, int id)   // 0x14db84
{
    if (id >= 156)
        return "wrongbeef";
    if (strings) {
        snprintf(g_langPath, 128, "%d", id);
        cJSON *item = cJSON_GetObjectItem(strings, g_langPath);
        return cJSON_GetStringValue(item);
    }
    return g_defaultStrings[id];
}
