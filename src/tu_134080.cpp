// tu_134080 (0x134080-0x1376fc): user-interface languages.
//
// Every language is a JSON file <lang>.lang in ./lang/ with "lang" (its display name) and the 156
// strings "0".."155" (ids: LangStrings.h). loadLanguages reads English first (writing
// /customer/app/lang/en.lang from the built-in table if missing, or falling back to that table),
// then every other .lang file, sorted by file name. languageString() returns a string in the
// language chosen in system.json. For each language it also writes a reduced <lang>.short with
// ids 88-155 (see tu_14da50.c). Languages other than English need the CJK font on the SD card.
// Body 4,768 bytes plus 110 generated functions; the draft start 0x133f60 was part of the
// preceding file's allocator tail.
#include "PlatformApi.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "Language.h"
struct cJSON;
extern "C" {
cJSON *cJSON_CreateObject(); void cJSON_Delete(cJSON *);
cJSON *cJSON_AddStringToObject(cJSON *, const char *, const char *);
char *cJSON_Print(const cJSON *); void cJSON_free(void *);
cJSON *cJSON_Parse(const char *); const char *cJSON_GetErrorPtr();
cJSON *cJSON_GetObjectItem(const cJSON *, const char *);
char *cJSON_GetStringValue(const cJSON *);
int readWholeFile(const char *, FILE *, char **);
const char *getLanguageFile();
}
bool languageFontExists();
extern const char *g_defaultStrings[156]; // 0x17fc9c
std::vector<LanguageChoice> g_languageChoices; // 0x1823f4: what the language setting offers
std::vector<Language> g_languages; // 0x182400: English first, then the others by file name
static_assert(sizeof(Language) == 60 && sizeof(LanguageChoice) == 48, "language layouts");

// Writes /customer/app/lang/<file>.short with strings 88-155 of a loaded language, unless it
// exists already. Returns true if it was written.
bool genShortLangFile(const std::string &file)
{
    std::string path;
    bool ok = false;
    int index = -1;
    index = 0;
    while (index < g_languages.size()) {
        if (g_languages[index].file.compare(file.c_str()) == 0) break;
        index++;
    }
    printf("genShortLangFile %s index %d, size %d\n", file.c_str(), index, g_languages.size());
    if (index >= g_languages.size()) return ok;
    if (access("/customer/app/lang", 0) == 0) {
        path = "/customer/app/lang";
        path += "/"; path += file; path += ".short";
        if (access(path.c_str(), 0) != 0) {
            char key[16] = {0};
            cJSON *json = cJSON_CreateObject();
            if (json) {
                for (int i = 88; i < STR_COUNT; i++) {   // the in-game menu's strings
                    snprintf(key, 16, "%d", i);
                    cJSON_AddStringToObject(json, key, g_languages[index].strings[i].c_str());
                }
                char *text = cJSON_Print(json);
                if (text) {
                    FILE *out = fopen(path.c_str(), "w+");
                    if (out) {
                        fwrite(text, 1, strlen(text), out);
                        fclose(out);
                        ok = true;
                    }
                    cJSON_free(text);
                }
                cJSON_Delete(json);
            }
        }
    }
    return ok;
}
// Writes the built-in English table to /customer/app/lang/en.lang if that file is missing.
// Returns true if it was written.
bool createEnglishLangFile()
{
    bool ok = false;
    if (access("/customer/app/lang", 0) != 0) mkdir("/customer/app/lang", 0777);
    if (access("/customer/app/lang/en.lang", 0) != 0) {
        cJSON *json = cJSON_CreateObject();
        char key[16] = {0};
        if (json) {
            cJSON_AddStringToObject(json, "lang", "English");
            for (int i = 0; i < STR_COUNT; i++) {
                snprintf(key, 16, "%d", i);
                cJSON_AddStringToObject(json, key, g_defaultStrings[i]);
            }
            char *text = cJSON_Print(json);
            if (text) {
                FILE *out = fopen("/customer/app/lang/en.lang", "w+");
                if (out) {
                    fwrite(text, 1, strlen(text), out);
                    fclose(out);
                    ok = true;
                }
                cJSON_free(text);
            }
            cJSON_Delete(json);
        }
    }
    return ok;
}
// Reads one .lang file into language (file name, display name, 156 strings). Fails if the
// display name or any string is missing.
bool readLanguageFile(const char *path, Language &language)
{
    FILE *file = fopen(path, "r");
    bool ok = false;
    if (file) {
        char *content;
        if (readWholeFile(path, file, &content) >= 0) {
            const char *part = strrchr(path, '/');
            if (part) {
                language.file = part + 1;
                cJSON *json = cJSON_Parse(content);
                if (json) {
                    char key[16] = {0};
                    cJSON *item = cJSON_GetObjectItem(json, "lang");
                    part = cJSON_GetStringValue(item);
                    if (part) {
                        language.name = part;
                        ok = true;
                        for (int i = 0; i < STR_COUNT; i++) {
                            snprintf(key, 16, "%d", i);
                            item = cJSON_GetObjectItem(json, key);
                            part = cJSON_GetStringValue(item);
                            if (part) language.strings.push_back(std::string(part));
                            else { ok = false; break; }
                        }
                    } else printf("%s have no lang string\n", path);
                    cJSON_Delete(json);
                } else printf("parse %s content fail:%s\n", path, cJSON_GetErrorPtr());
            }
            free(content);
        } else printf("read %s content fail\n", path);
        fclose(file);
    }
    printf("read lang file %s ok=%d\n", path, ok);
    return ok;
}
// Builds g_languages from <root>/lang/ (see the file header).
// Original quirk, kept as-is: the paths are built as root + "./lang/...", e.g. "././lang/".
void loadLanguages(const char *root)
{
    bool created = createEnglishLangFile();
    bool fallback = true;
    int index = 0;
    if (created != true) {
        std::string path(root);
        path += "./lang/en.lang";
        Language lang;
        if (readLanguageFile(path.c_str(), lang) && lang.strings.size() >= STR_COUNT) {
            fallback = false;
            g_languages.push_back(lang);
            genShortLangFile("en.lang");
        }
    }
    if (fallback) {
        Language lang;
        lang.file = "en.lang";
        lang.name = "English";
        std::string text;
        for (int i = 0; i < STR_COUNT; i++) {
            text = g_defaultStrings[i];
            lang.strings.push_back(text);
        }
        g_languages.push_back(lang);
        genShortLangFile("en.lang");
    }
    std::string path(root);
    path += "./lang/";
    DIR *dir;
    if ((dir = opendir(path.c_str())) != NULL) {
        std::string fullPath;
        Language lang;
        dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                if (strcmp(entry->d_name, "en.lang") == 0) continue;
                if (strstr(entry->d_name, ".short") != NULL) continue;
                fullPath = path;
                fullPath += entry->d_name;
                lang.strings.clear();
                if (readLanguageFile(fullPath.c_str(), lang)) {
                    index = 1;
                    while (index < g_languages.size()) {
                        if (strncmp(g_languages[index].file.c_str(), lang.file.c_str(), lang.file.length()) > 0) break;
                        index++;
                    }
                    printf("add language %ld %s to %d. example %s\n", g_languages.size(), lang.file.c_str(), index, lang.strings[0].c_str());
                    if (index >= g_languages.size()) g_languages.push_back(lang);
                    else g_languages.insert(g_languages.begin() + index, lang);
                    genShortLangFile(entry->d_name);
                }
            }
        }
    }
}
// The languages the settings menu offers: all of them, or only English when the CJK font
// (/mnt/SDCARD/wqy-microhei.ttc) is missing.
std::vector<LanguageChoice> *languageChoices()
{
    g_languageChoices.clear();
    LanguageChoice item;
    if (languageFontExists()) {
        for (int i = 0; i < g_languages.size(); i++) {
            item.file = g_languages[i].file;
            item.name = g_languages[i].name;
            g_languageChoices.push_back(item);
        }
    } else {
        printf("LANG_FONT_FILE %s not found, support english only", "/mnt/SDCARD/wqy-microhei.ttc");
        for (int i = 0; i < g_languages.size(); i++) {
            item.file = g_languages[i].file;
            if (item.file.compare("en.lang") == 0) {
                item.name = g_languages[i].name;
                g_languageChoices.push_back(item);
                break;
            }
        }
    }
    return &g_languageChoices;
}
// A user-interface string (LangStrings.h) in the current language, else in English.
// Out-of-range ids give "" or "wrongbeef".
const char *languageString(int id)
{
    if (id < 0) return "";
    if (id >= STR_COUNT) return "wrongbeef";
    {
        const char *lang = getLanguageFile();
        for (int i = 0; i < g_languages.size(); i++) {
            if (g_languages[i].file.compare(lang) == 0)
                return g_languages[i].strings[id].c_str();
        }
        return g_defaultStrings[id];
    }
}
