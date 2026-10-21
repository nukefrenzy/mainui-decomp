// tu_1484d4: MultiDBCachedTextMenu, the global ROM search list (0x1484d4-0x149b54, then its template tail).
// Opens every emulator's <name>_cache6.db that has rows matching the search string and pages the
// results in through DBCacheWorker background jobs.
// Tail order: the C headers come first and define no inline functions, so the tail starts with
// count() (defined in the class) and then follows first use in the bodies below; it matched as
// written. The unit is 0x1484d4-0x14acfc: body to 0x149b54, then 60 COMDAT functions.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "MultiDBCachedTextMenu.h"

extern "C" {
typedef struct sqlite3_stmt sqlite3_stmt;
int sqlite3_open(const char *filename, sqlite3 **db);
int sqlite3_close(sqlite3 *db);
int sqlite3_prepare_v2(sqlite3 *db, const char *sql, int nbyte, sqlite3_stmt **stmt, const char **tail);
int sqlite3_step(sqlite3_stmt *stmt);
int sqlite3_column_int(sqlite3_stmt *stmt, int col);
int sqlite3_finalize(sqlite3_stmt *stmt);
const char *sqlite3_errmsg(sqlite3 *db);
}

// One row of a cached list (0x74 bytes).
struct TextItem {
    ~TextItem();                // 0x25620
    char raw[0x74];
};

class DBCacheWorker {           // a BGWorker, 0x284 bytes
public:
    DBCacheWorker(sqlite3 *db, const std::string &dbName, const std::string &table, int offset, int len,
                  int x, MenuHelper80 *items, MenuHelper80 *data, EmuInfo *emu, std::string *dir,
                  std::string *search, bool flag);   // 0x3a74c
    char raw[0x284];
};

const char *getLanguageFile();               // 0x14ba18
extern std::vector<EmuInfo> g_romSourcesA;   // 0x181944
extern std::vector<EmuInfo> g_romSourcesB;   // 0x181950

// 0x1484d4: free callback for `items`
static void deleteTextItem(SDL_Surface *p)
{
    TextItem *item = (TextItem *)p;
    delete item;
}

// 0x148518: free callback for `data`
static void freeData(SDL_Surface *p)
{
    void *d = p;
    free(d);
}

// 0x148544: strip trailing slashes and return the last path component
char *lastPathComponent(char *path)
{
    int len = strlen(path);
    if (len > 0) {
        for (int i = len - 1; i > 0; i--) {
            if (path[i] == '/') {
                if (i < len - 1)
                    return path + i + 1;
                path[i] = '\0';
            }
        }
    }
    return path;
}

MultiDBCachedTextMenu::MultiDBCachedTextMenu(std::string search)
    : TextMenuDrawer(0, 1, 4, "skin/bg-list-l.png"), workers(10), items(deleteTextItem), data(freeData)
{
    name = "global_search";
    dir = ".";
    str18 = search;
    prepare();
    field58 = 90;
}

MultiDBCachedTextMenu::~MultiDBCachedTextMenu()
{
    for (unsigned int i; i < dbs.size(); i++)   // sic: i is never initialised
        sqlite3_close(dbs[i].db);
}

// 0x1488d0: open emu's cache database and count its matches; keeps it in dbs if there are any
int MultiDBCachedTextMenu::openCache(EmuInfo &emu, char *buf, size_t size)
{
    bool ok = false;
    char *base = NULL;
    char *path = NULL;
    DBEntry entry;
    int rc = -1;
    int count = 0;
    path = strdup(emu.path.c_str());
    base = lastPathComponent(path);
    snprintf(buf, size, "%s/%s_cache6.db", emu.path.c_str(), base);
    sqlite3 *db;
    rc = sqlite3_open(buf, &db);
    if (rc == 0) {
        if (emu.hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", base, str18.c_str());
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", base, str18.c_str());
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                count = sqlite3_column_int(stmt, 0);
                if (count > 0) {
                    entry.db = db;
                    entry.count = count;
                    entry.name = base;
                    entry.emu = &emu;
                    dbs.push_back(entry);
                    printf("%s %d items\n", base, count);
                    ok = true;
                }
            }
        } else {
            printf("db exec at %d fail: %s\n", 90, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
        if (!ok)
            sqlite3_close(db);
    }
    return count;
}

// 0x148bc4: open the cache of every ROM source and add up the matches
void MultiDBCachedTextMenu::prepare()
{
    int count;
    unsigned int i;
    unsigned int j;
    unsigned int k;
    const int bufSize = 512;
    int t = 0;
    if (str18.length() != 0) {
        char *buf = (char *)malloc(bufSize);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        t = tv.tv_sec * 1000000 + tv.tv_usec;
        for (i = 0; i < g_romSourcesA.size(); i++) {
            count = openCache(g_romSourcesA[i], buf, bufSize);
            if (count > 0)
                printf("%s have %d items\n", g_romSourcesA[i].path.c_str(), count);
        }
        for (j = 0; j < g_romSourcesB.size(); j++) {
            count = openCache(g_romSourcesB[j], buf, bufSize);
            if (count > 0)
                printf("%s have %d items\n", g_romSourcesB[j].path.c_str(), count);
        }
        free(buf);
        gettimeofday(&tv, NULL);
        t = tv.tv_sec * 1000000 + tv.tv_usec - t;
        printf("prepare cost %dms\n", t / 1000);
    }
    count = 0;
    for (k = 0; k < dbs.size(); k++)
        count += dbs[k].count;
    total = count;
}

// 0x148eac
void MultiDBCachedTextMenu::fetchForward()
{
    int index = cur;
    int len = 5;
    if (index + len > total)
        len = total - index;
    printf("fetchForward prepareBGWorkers index=%d, lenreq=%d\n", index, len);
    prepareBGWorkers(index, len);
}

// 0x148f30
void MultiDBCachedTextMenu::fetchBackward()
{
    int index = cur - 5;
    int len = 6;
    if (index < 0) {
        index = 0;
        len = cur + 1;
    }
    printf("fetchBackward prepareBGWorkers index=%d, lenreq=%d\n", index, len);
    prepareBGWorkers(index, len);
}

int MultiDBCachedTextMenu::vfunc16()
{
    int r = TextMenuDrawer::vfunc16();
    fetchBackward();
    return r;
}

int MultiDBCachedTextMenu::vfunc17()
{
    int r = TextMenuDrawer::vfunc17();
    fetchForward();
    return r;
}

int MultiDBCachedTextMenu::vfunc9()
{
    int r = TextMenuDrawer::vfunc9();
    fetchForward();
    return r;
}

int MultiDBCachedTextMenu::vfunc10()
{
    int r = TextMenuDrawer::vfunc10();
    fetchBackward();
    return r;
}

void MultiDBCachedTextMenu::restorePosition(int a, int cur, int c)
{
    TextMenuDrawer::restorePosition(a, cur, c);
    int len = pageEnd - pageStart + 1;
    int index = cur;
    if (pageEnd == 0)
        len = 5;
    printf("restorePos prepareBGWorkers index=%d, lenreq=%d\n", index, len);
    prepareBGWorkers(index, len);
}

// 0x149138: queue workers that load rows [index, index+len) across the databases
void MultiDBCachedTextMenu::prepareBGWorkers(int index, int len)
{
    int take = 0;
    int end = 0;
    int start = 0;
    DBCacheWorker *worker = NULL;
    std::string table;
    for (unsigned int i = 0; i < dbs.size() && len > 0; i++) {
        end += dbs[i].count;
        if (index < end) {
            table = name + "_" + dbs[i].name;
            if (len <= end - index) {
                worker = new DBCacheWorker(dbs[i].db, dbs[i].name, table, index - start, len, 24,
                                           &items, &data, dbs[i].emu, &dir, &str18, true);
                workers.addWorker((BGWorker *)worker);
                len = 0;
            } else {
                take = end - index;
                worker = new DBCacheWorker(dbs[i].db, dbs[i].name, table, index - start, take, 24,
                                           &items, &data, dbs[i].emu, &dir, &str18, true);
                workers.addWorker((BGWorker *)worker);
                index += take;
                len -= take;
            }
        }
        start = end;
    }
}

void *MultiDBCachedTextMenu::getItem(int index)
{
    if (dbs.size() != 0) {
        int end = 0;
        int start = 0;
        unsigned int i = 0;
        for (i = 0; i < dbs.size(); i++) {
            end += dbs[i].count;
            if (index < end)
                break;
            start = end;
        }
        snprintf(key, sizeof(key), "%d", index - start);
        std::string k = name + std::string("_") + dbs[i].name + std::string("_") + key;
        void *item = items.get(k);
        return item;
    }
    return NULL;
}

void *MultiDBCachedTextMenu::vfunc6(int index)
{
    if (dbs.size() != 0) {
        int end = 0;
        int start = 0;
        unsigned int i = 0;
        for (i = 0; i < dbs.size(); i++) {
            end += dbs[i].count;
            if (index < end)
                break;
            start = end;
        }
        snprintf(key, sizeof(key), "%d", index - start);
        std::string k = name + std::string("_") + dbs[i].name + std::string("_") + key;
        void *d = data.get(k);
        if (d != NULL)
            return d;
    }
    return NULL;
}
