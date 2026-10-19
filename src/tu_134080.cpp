// Language loading, built-in fallback and abbreviated language files.
// Proven 0x134080-0x1376fc: 4,768-byte body plus 110 generated functions.
// The draft start 0x133f60 was part of the preceding file's allocator tail.
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
std::vector<LanguageChoice> g_languageChoices; // 0x1823f4
std::vector<Language> g_languages; // 0x182400
static_assert(sizeof(Language) == 60 && sizeof(LanguageChoice) == 48, "language layouts");

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
                for (int i = 88; i < 156; i++) {
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
bool createEnglishLangFile()
{
    bool ok = false;
    if (access("/customer/app/lang", 0) != 0) mkdir("/customer/app/lang", 0777);
    if (access("/customer/app/lang/en.lang", 0) != 0) {
        cJSON *json = cJSON_CreateObject();
        char key[16] = {0};
        if (json) {
            cJSON_AddStringToObject(json, "lang", "English");
            for (int i = 0; i < 156; i++) {
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
                        for (int i = 0; i < 156; i++) {
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
void loadLanguages(const char *root)
{
    bool created = createEnglishLangFile();
    bool fallback = true;
    int index = 0;
    if (created != true) {
        std::string path(root);
        path += "./lang/en.lang";
        Language lang;
        if (readLanguageFile(path.c_str(), lang) && lang.strings.size() >= 156) {
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
        for (int i = 0; i < 156; i++) {
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
const char *languageString(int id)
{
    if (id < 0) return "";
    if (id >= 156) return "wrongbeef";
    {
        const char *lang = getLanguageFile();
        for (int i = 0; i < g_languages.size(); i++) {
            if (g_languages[i].file.compare(lang) == 0)
                return g_languages[i].strings[id].c_str();
        }
        return g_defaultStrings[id];
    }
}
