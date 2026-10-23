// tu_1484d4 (0x1484d4-0x14acfc): MultiDBCachedTextMenu, the global ROM search list.
//
// A ROM search opens this list (MenuWindow::onShow). Every emulator and RetroArch-app folder keeps a
// SQLite cache of its ROMs (<folder>/<name>_cache6.db, table <name>_roms); the menu opens each
// one that has games whose name matches the search (by pinyin, or by Chinese pinyin with the
// Chinese language), and shows all matches as one list, in folder order. Rows are not loaded up
// front: as the cursor moves, DBCacheWorker jobs (tu_015b20) fetch the rows around it in the
// background into the `items` cache, and getItem returns them once they are there.
// Tail order: the C headers come first and define no inline functions, so the tail starts with
// count() (defined in the class) and then follows first use in the bodies below; it matched as
// written. The unit is 0x1484d4-0x14acfc: body to 0x149b54, then 60 COMDAT functions.
#include "PlatformApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "MultiDBCachedTextMenu.h"

#include "SQLiteApi.h"

// One row of a cached list (0x74 bytes).
struct TextItem {
    ~TextItem();                // 0x25620
    char raw[0x74];
};

class DBCacheWorker
{           // a BGWorker, 0x284 bytes
public:
    DBCacheWorker(sqlite3 *db, const std::string &dbName, const std::string &table, int offset, int len,
                  int rowWindowId, LruCache *items, LruCache *data, EmuInfo *emu, std::string *dir,
                  std::string *search, bool previewSearchResults);   // 0x3a74c
    char raw[0x284];
};

const char *getLanguageFile();               // 0x14ba18
extern std::vector<EmuInfo> g_emuFolders;   // 0x181944
extern std::vector<EmuInfo> g_rappFolders;   // 0x181950

// 0x1484d4: free callback for `items`
static void deleteTextItem(SDL_Surface *value)
{
    TextItem *item = (TextItem *)value;
    delete item;
}

// 0x148518: free callback for `data`
static void freeData(SDL_Surface *value)
{
    void *d = value;
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

// Opens the caches with matches for `search` (see prepare); rows are 90 px high.
MultiDBCachedTextMenu::MultiDBCachedTextMenu(std::string search)
    : TextMenuDrawer(0, 1, 4, "skin/bg-list-l.png"), workers(10), items(deleteTextItem), data(freeData)
{
    name = "global_search";
    dir = ".";
    searchText = search;
    prepare();
    textRowHeight = 90;
}

// Closes the databases. Original bug, kept as-is: the loop index is never initialized.
MultiDBCachedTextMenu::~MultiDBCachedTextMenu()
{
    for (unsigned int i; i < dbs.size(); i++)   // sic: i is never initialized
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
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", base, searchText.c_str());
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", base, searchText.c_str());
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {   // SQLITE_ROW
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
    if (searchText.length() != 0) {
        char *buf = (char *)malloc(bufSize);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        t = tv.tv_sec * 1000000 + tv.tv_usec;
        for (i = 0; i < g_emuFolders.size(); i++) {
            count = openCache(g_emuFolders[i], buf, bufSize);
            if (count > 0)
                printf("%s have %d items\n", g_emuFolders[i].path.c_str(), count);
        }
        for (j = 0; j < g_rappFolders.size(); j++) {
            count = openCache(g_rappFolders[j], buf, bufSize);
            if (count > 0)
                printf("%s have %d items\n", g_rappFolders[j].path.c_str(), count);
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

// 0x148eac: queue the rows from the cursor down (five, or to the end)
void MultiDBCachedTextMenu::fetchForward()
{
    int index = cur;
    int len = 5;
    if (index + len > total)
        len = total - index;
    printf("fetchForward prepareBGWorkers index=%d, lenreq=%d\n", index, len);
    prepareBGWorkers(index, len);
}

// 0x148f30: queue the rows above the cursor (up to five, and the cursor row)
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

// Cursor movement (moveDown, moveUp, pageUp, pageDown): move as usual, then queue the rows that
// came into view.
int MultiDBCachedTextMenu::pageUp()
{
    int r = TextMenuDrawer::pageUp();
    fetchBackward();
    return r;
}

int MultiDBCachedTextMenu::pageDown()
{
    int r = TextMenuDrawer::pageDown();
    fetchForward();
    return r;
}

int MultiDBCachedTextMenu::moveDown()
{
    int r = TextMenuDrawer::moveDown();
    fetchForward();
    return r;
}

int MultiDBCachedTextMenu::moveUp()
{
    int r = TextMenuDrawer::moveUp();
    fetchBackward();
    return r;
}

// Returning to the list (e.g. after a game): restore the position and reload the visible page.
void MultiDBCachedTextMenu::restorePosition(int savedCur, int savedStart, int savedEnd)
{
    TextMenuDrawer::restorePosition(savedCur, savedStart, savedEnd);
    int len = pageEnd - pageStart + 1;
    int index = savedStart;   // load from the top of the visible page
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
                                           &items, &data, dbs[i].emu, &dir, &searchText, true);
                workers.addWorker((BGWorker *)worker);
                len = 0;
            } else {
                take = end - index;
                worker = new DBCacheWorker(dbs[i].db, dbs[i].name, table, index - start, take, 24,
                                           &items, &data, dbs[i].emu, &dir, &searchText, true);
                workers.addWorker((BGWorker *)worker);
                index += take;
                len -= take;
            }
        }
        start = end;
    }
}

// The row at a list position if it has been loaded, else NULL (not loaded yet).
// Rows are cached under "global_search_<db name>_<position in that db>".
// Original quirk, kept as-is: a position past the last match indexes past the end of dbs.
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

// The same lookup in the `data` cache: the row's extra data, if loaded.
void *MultiDBCachedTextMenu::getItemData(int index)
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
