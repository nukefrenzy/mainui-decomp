// tu_143720: pinyin table for search (0x143720-0x143ea4, then its template tail).
// Loads ./py.dat ("<hanzi> <pinyin>" per line) into a map from codepoint to pinyin, and uses it
// to match ASCII search keys against UTF-8 names. Strings are tiny-utf8 4.4.x
// (scripts/fetch-tinyutf8.sh); every template function in the tail comes from its header.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "tinyutf8/tinyutf8.h"

std::map<char32_t, std::string> g_pinyinTable;   // 0x18249c

// 0x143720: split one py.dat line into its codepoint and pinyin; -1 if it isn't a CJK entry
int parsePinyinLine(tiny_utf8::string line, char32_t *codepoint, std::string &pinyin)
{
    if (line.length() > 2) {
        *codepoint = line.at(0);
        if (*codepoint > 0x7f) {
            tiny_utf8::string::size_type first = line.find(" ", 1);
            tiny_utf8::string::size_type second = line.find(" ", 2);
            if (second == tiny_utf8::string::npos)
                pinyin = line.substr(first + 1, -1).c_str();
            else
                pinyin = line.substr(first + 1, second - 2).c_str();
            pinyin.erase(std::remove(pinyin.begin(), pinyin.end(), '\n'), pinyin.end());
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

// 0x143920: called from main
void loadPinyinTable()
{
    g_pinyinTable.clear();
    char *line = NULL;
    size_t len = 0;
    tiny_utf8::string str;
    char32_t codepoint;
    std::string pinyin;
    FILE *fp = fopen("./py.dat", "r");
    if (fp != NULL) {
        while (getline(&line, &len, fp) != -1) {
            str = tiny_utf8::string(line);
            if (parsePinyinLine(str, &codepoint, pinyin) >= 0)
                g_pinyinTable[codepoint] = pinyin;
        }
        fclose(fp);
    }
}

// 0x143ae4: one ASCII letter per codepoint (the pinyin initial for CJK); caller frees
char *pinyinInitials(const char *text)
{
    tiny_utf8::string str(text);
    char *out = (char *)malloc(str.length() + 1);
    memset(out, 0, str.length() + 1);
    char32_t cp;
    std::string pinyin;
    for (unsigned int i = 0; i < str.length(); i++) {
        cp = str.at(i);
        if (cp < 0x80) {
            out[i] = (char)cp;
        } else {
            pinyin = g_pinyinTable[cp];
            if (pinyin.length() != 0)
                out[i] = pinyin.at(0);
            else
                out[i] = ' ';
        }
    }
    return out;
}

// 0x143cf4: find key in the initials of text; return the match as a byte range of text
int pinyinFind(const char *text, const char *initials, const char *key, const char **start, int *bytes)
{
    const char *hit = strcasestr(initials, key);
    if (hit != NULL) {
        tiny_utf8::string str(text);
        tiny_utf8::string::size_type offset = 0;
        if (hit == initials) {
            offset = 0;
            *start = text;
        } else {
            offset = str.get_num_bytes_from_start(hit - initials);
            *start = text + offset;
        }
        *bytes = str.get_num_bytes(offset, strlen(key));
        return 0;
    }
    return -1;
}
