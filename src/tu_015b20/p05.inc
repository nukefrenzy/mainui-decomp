// tu_015b20 part p05 (0x20500-0x23548): TextMenu and the ROM cache database (scan, gamelist.xml import).
#include <sys/time.h>
#include <fstream>
#include <iterator>
#include "rapidxml.hpp"   // scripts/fetch-rapidxml.sh (deps "rapidxml" in src/matches.tsv)

#include "../SQLiteApi.h"

const char *getLanguageFile();                                  // 0x14ba18 (tu_14b860)
char *pinyinInitials(const char *s);                            // 0x143ae4 (tu_143720)
int countDirectoryItems(std::string dir, bool (*filter)(const char *, const char *)); // 0x12d400

// 0x20500: a TextMenuDrawer that owns its rows (TextItem*).
TextMenu::TextMenu(bool noSelectedBg, bool stripes, int rows, const char *background)
    : TextMenuDrawer(noSelectedBg, stripes, rows, background)
{
}

// 0x20570
TextMenu::~TextMenu()
{
    freeListItem();
}

// 0x205f8: free every item (slot 18).
void TextMenu::freeListItem()
{
    for (int i = 0; i < items.size(); i++) {
        TextItem *item = (TextItem *)items[i];
        printf("TextMenu::freeListItem %d %s\n", i, item->label.c_str());
        delete item;
    }
    puts("TextMenu::freeListItem end");
    items.clear();
}

// 0x206e0: reload (slot 13).
void TextMenu::clearItems()
{
    freeListItem();
}

// 0x20714: the row at index, or NULL.
void *TextMenu::getItem(int index)
{
    if (index >= 0 && index < items.size())
        return items[index];
    else
        return 0;
}

// 0x2079c: remove the item at index (slot 12).
void TextMenu::removeItem(int index)
{
    if (index >= 0 && index < items.size()) {
        TextItem *item = (TextItem *)items[index];
        items.erase(items.begin() + index);
        delete item;
    }
}

// 0x20898
void TextMenu::addItem(void *item)
{
    items.push_back(item);
}

// 0x208d0: insert before the first item cmp() does not rank after.
void TextMenu::insertSorted(void *item, int (*cmp)(TextItem *, TextItem *))
{
    int i = 0;
    while (i < items.size()) {
        if (cmp((TextItem *)item, (TextItem *)items[i]) <= 0)
            break;
        i++;
    }
    if (i < items.size())
        items.insert(items.begin() + i, item);
    else
        items.push_back(item);
}

// 0x20a3c: append unless an item with the same label is listed; true if added.
bool TextMenu::addUnique(TextItem *item)
{
    for (int i = 0; i < items.size(); i++) {
        if (item->label.compare(((TextItem *)items[i])->label) == 0)
            return false;
    }
    items.push_back((void *)item);
    return true;
}

// 0x20b1c: strip trailing slashes and return the last path component (same as tu_1484d4's).
char *romFolderName(char *path)
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

// 0x20bd4: rebuild <table>_roms from a gamelist.xml (<gameList><game><path/><name/><image/>...);
// returns the rows added.
int importGameList(const char *xml, int arg, const char *table, sqlite3 *db)
{
    char *err;
    std::string name;
    std::string path;
    std::string image;
    int count = 0;
    int rc = -1;
    char *sql = NULL;
    int cost;
    rapidxml::xml_node<> *game;
    const int bufSize = 512;
    char *dir = strdup(xml);
    char *slash = strrchr(dir, '/');
    if (slash)
        *slash = '\0';
    cost = 0;
    int t1 = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int t0 = tv.tv_sec * 1000000 + tv.tv_usec;
    if (db) {
        sql = (char *)malloc(bufSize);
        snprintf(sql, bufSize, "DROP TABLE %s_roms;", table);
        if (sqlite3_exec(db, sql, NULL, NULL, &err) != 0) {
            fprintf(stderr, "db exec at %d error: %s\n", 3581, err);
            sqlite3_free(err);
        }
        snprintf(sql, bufSize,
                 "CREATE TABLE %s_roms (id INTEGER PRIMARY KEY AUTOINCREMENT,disp TEXT NOT NULL,path TEXT NOT NULL,imgpath TEXT NOT NULL,type INTEGER DEFAULT 0,ppath TEXT NOT NULL,pinyin TEXT NOT NULL,cpinyin TEXT NOT NULL);",
                 table);
        rc = sqlite3_exec(db, sql, NULL, NULL, &err);
        if (rc != 0) {
            fprintf(stderr, "db exec at %d error: %s\n", 3596, err);
            sqlite3_free(err);
        }
    }
    printf("read xml from %s\n", xml);
    if (access(xml, 0) == 0) {
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        rapidxml::xml_document<> doc;
        std::ifstream file(xml);
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
        rapidxml::xml_node<> *root = doc.first_node("gameList");
        if (root) {
            for (game = root->first_node("game"); game; game = game->next_sibling()) {
                rapidxml::xml_node<> *node = game->first_node("path");
                if (!node) {
                    puts("no 'path' in xml");
                    break;
                }
                if (node->value()) {
                    path = dir;
                    path += "/";
                    path += node->value();
                }
                const char *ext = strrchr(path.c_str(), '.');
                if (strncasecmp(ext, ".zip", 4) != 0) {
                    if (findEmuForFile(path.c_str()) < 0)
                        continue;
                }
                node = game->first_node("name");
                if (!node) {
                    puts("no 'name' in xml");
                    break;
                }
                if (node->value())
                    name = node->value();
                node = game->first_node("image");
                if (!node) {
                    puts("no 'image' in xml");
                    break;
                }
                if (node->value()) {
                    image = dir;
                    image += "/";
                    image += node->value();
                }
                if (name.length() && path.length() && image.length()) {
                    if (access(path.c_str(), 0) == 0) {
                        if (rc == 0) {
                            gettimeofday(&tv, NULL);
                            t1 = tv.tv_sec * 1000000 + tv.tv_usec;
                            std::string pinyin;
                            char *py = pinyinInitials(name.c_str());
                            if (py)
                                pinyin = py;
                            snprintf(sql, bufSize,
                                     "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,0,'.',?,?);",
                                     table);
                            sqlite3_stmt *stmt;
                            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                                sqlite3_bind_text(stmt, 1, name.c_str(), name.length(), NULL);
                                sqlite3_bind_text(stmt, 2, path.c_str(), path.length(), NULL);
                                sqlite3_bind_text(stmt, 3, image.c_str(), image.length(), NULL);
                                sqlite3_bind_text(stmt, 4, pinyin.c_str(), pinyin.length(), NULL);
                                sqlite3_bind_text(stmt, 5, pinyin.c_str(), pinyin.length(), NULL);
                                int r = sqlite3_step(stmt);
                                if (r == 101 || r == 100) {
                                    count++;
                                } else {
                                    const char *e = sqlite3_errmsg(db);
                                    fprintf(stderr, "db exec at %d error : %s\n", 3700, sqlite3_errmsg(db));
                                }
                                sqlite3_finalize(stmt);
                            } else {
                                fprintf(stderr, "db exec at %d error : %s\n", 3704, sqlite3_errmsg(db));
                            }
                            if (py)
                                free(py);
                            gettimeofday(&tv, NULL);
                            cost += tv.tv_sec * 1000000 + tv.tv_usec - t1;
                        }
                        g_progressCur++;
                    }
                }
            }
        }
        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        printf("%d items added cost %dms\n", count, cost / 1000);
    }
    sync();
    free(dir);
    if (sql)
        free(sql);
    gettimeofday(&tv, NULL);
    t0 = tv.tv_sec * 1000000 + tv.tv_usec - t0;
    printf("total cost %dms\n", t0 / 1000);
    return count;
}

// 0x21818: true if a directory entry is not a ROM of this emulator (hidden files, BIOS, licenses,
// saves, or an extension outside the emulator's list; a .bin with a matching .cue).
bool isHiddenRomFile(EmuInfo *emu, const char *path, const char *name, int hideBios)
{
    if (hideBios && strncmp(name, "[BIOS]", 6) == 0)
        return true;
    if (name[0] == '.')
        return true;
    if (strncmp(name, "neogeo.zip", 10) == 0)
        return true;
    if (strncmp(name, "gngeo_data.zip", 14) == 0)
        return true;
    if (strncmp(name, "COPYING", 7) == 0)
        return true;
    if (strncasecmp(name, "LICENSE", 7) == 0)
        return true;
    const char *ext = strrchr(name, '.');
    if (ext == NULL || emu == NULL)
        return true;
    if (emu->extensions.size() != 0) {
        for (int i = 0; i < emu->extensions.size(); i++) {
            if (strncasecmp(ext + 1, emu->extensions[i].c_str(), emu->extensions[i].size()) == 0)
                return false;
        }
    } else {
        if (ext && strncmp(ext, ".db", 3) == 0)
            return true;
        if (ext && strncmp(ext, ".sav", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".svs", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".gz", 3) == 0)
            return true;
        if (ext && strncmp(ext, ".gqs", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".png", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".bin", 4) == 0) {
            std::string bin(path);
            size_t dot = bin.rfind(".");
            std::string cue = bin.substr(0, dot) + ".cue";
            if (access(cue.c_str(), 0) == 0) {
                printf("%s exists, hide %s\n", cue.c_str(), bin.c_str());
                return true;
            }
        }
        return false;
    }
    return true;
}

// 0x21d50: add the ROMs under dir to <table>_roms (recursing once into subfolders, which get a
// type 1 row); returns the rows added here.
int scanRomDir(std::string dir, std::string imgDir, EmuInfo *emu, const char *table, bool sub, sqlite3 *db,
               char *path, char *name, int nameSize, char *sql, int sqlSize, unsigned long *cost, int *added)
{
    std::string disp;
    std::string romPath;
    std::string imgPath;
    size_t slash = dir.rfind("/");
    std::string ppath(dir);
    int count = 0;
    if (slash != -1)
        ppath = dir.substr(slash + 1);
    puts("handleDir start");
    DIR *d;
    if ((d = opendir(dir.c_str())) != NULL) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            sprintf(path, "%s/%s", dir.c_str(), ent->d_name);
            if (ent->d_type == DT_REG) {
                if (isHiddenRomFile(emu, path, ent->d_name, emu->hideBios))
                    continue;
                strncpy(name, ent->d_name, nameSize - 1);
                char *dot = strrchr(name, '.');
                if (dot)
                    *dot = '\0';
                unsigned long t = milliseconds();
                disp = name;
                romPath = path;
                sprintf(path, "%s/%s.png", imgDir.c_str(), name);
                imgPath = path;
                std::string en;
                std::string ch;
                char *py = NULL;
                char *pyEn = NULL;
                char *pyCh = NULL;
                if (emu->hasCpinyin) {
                    const char *enName = NULL;
                    const char *chName = NULL;
                    enName = GetGameName("en.lang", name);
                    chName = GetGameName("ch.lang", name);
                    if (enName)
                        pyEn = pinyinInitials(enName);
                    if (chName)
                        pyCh = pinyinInitials(chName);
                    printf("dispname=%s,endisp=%s,chdisp=%s,pyen=%s,pych=%s\n", name, enName, chName, pyEn,
                           pyCh);
                    if (pyEn == NULL || pyCh == NULL) {
                        py = pinyinInitials(disp.c_str());
                        if (pyEn == NULL)
                            pyEn = py;
                        if (pyCh == NULL)
                            pyCh = py;
                    }
                } else {
                    py = pinyinInitials(disp.c_str());
                    pyCh = py;
                    pyEn = pyCh;
                }
                en = pyEn;
                ch = pyCh;
                printf("inset to db shortname %d, name %s, enstr %s, chstr %s\n", emu->hasCpinyin, disp.c_str(),
                       en.c_str(), ch.c_str());
                snprintf(sql, sqlSize,
                         "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,?,?,?,?);",
                         table);
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                    sqlite3_bind_text(stmt, 1, disp.c_str(), disp.length(), NULL);
                    sqlite3_bind_text(stmt, 2, romPath.c_str(), romPath.length(), NULL);
                    sqlite3_bind_text(stmt, 3, imgPath.c_str(), imgPath.length(), NULL);
                    sqlite3_bind_int(stmt, 4, 0);
                    if (!sub)
                        sqlite3_bind_text(stmt, 5, ".", 1, NULL);
                    else
                        sqlite3_bind_text(stmt, 5, ppath.c_str(), ppath.length(), NULL);
                    sqlite3_bind_text(stmt, 6, en.c_str(), en.length(), NULL);
                    sqlite3_bind_text(stmt, 7, ch.c_str(), ch.length(), NULL);
                    int rc = sqlite3_step(stmt);
                    if (rc == 101 || rc == 100) {
                        (*added)++;
                        count++;
                    } else {
                        const char *err = sqlite3_errmsg(db);
                        fprintf(stderr, "db exec at %d error : %s\n", 3908, sqlite3_errmsg(db));
                    }
                    sqlite3_finalize(stmt);
                } else {
                    fprintf(stderr, "db exec at %d error : %s\n", 3912, sqlite3_errmsg(db));
                }
                if (py)
                    free(py);
                *cost += milliseconds() - t;
                g_progressCur++;
            } else if (ent->d_type == DT_DIR && !sub && strncmp(ent->d_name, ".", 1) != 0) {
                std::string subdir = dir + "/" + ent->d_name;
                if (scanRomDir(subdir, imgDir, emu, table, true, db, path, name, nameSize, sql, sqlSize, cost, added) > 0) {
                    sprintf(path, "%s/%s", dir.c_str(), ent->d_name);
                    snprintf(sql, sqlSize,
                             "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,1,'.','','');",
                             table);
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                        sqlite3_bind_text(stmt, 1, ent->d_name, strlen(ent->d_name), NULL);
                        sqlite3_bind_text(stmt, 2, path, strlen(path), NULL);
                        sqlite3_bind_text(stmt, 3, path, strlen(path), NULL);
                        int rc = sqlite3_step(stmt);
                        if (rc == 101 || rc == 100) {
                            (*added)++;
                            count++;
                        } else {
                            const char *err = sqlite3_errmsg(db);
                            fprintf(stderr, "db exec at %d error : %s\n", 3957, sqlite3_errmsg(db));
                        }
                        sqlite3_finalize(stmt);
                    } else {
                        fprintf(stderr, "db exec at %d error : %s\n", 3961, sqlite3_errmsg(db));
                    }
                }
            }
        }
        closedir(d);
        printf("closedir %s %p\n", dir.c_str(), d);
    }
    printf("handleDir end newly add %d roms, total %d roms now\n", count, *added);
    return count;
}

// 0x2291c: rebuild <table>_roms from the emulator's ROM folder.
int rebuildRomCache(EmuInfo *emu, int arg, const char *table, sqlite3 *db)
{
    if (db == NULL) {
        puts("readFromFS nil db");
        return 0;
    }
    char name[128];
    char *err;
    int added = 0;
    int rc = -1;
    int ret = 0;
    char *sql = NULL;
    const int bufSize = 512;
    char *path = (char *)malloc(emu->path.size() + 256);
    unsigned long cost = 0;
    int unused = 0;
    unsigned long t = milliseconds();
    sql = (char *)malloc(bufSize);
    snprintf(sql, bufSize, "DROP TABLE %s_roms;", table);
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != 0) {
        fprintf(stderr, "db exec at %d error: %s\n", 4003, err);
        sqlite3_free(err);
    }
    snprintf(sql, bufSize,
             "CREATE TABLE %s_roms (id INTEGER PRIMARY KEY AUTOINCREMENT,disp TEXT NOT NULL,path TEXT NOT NULL,imgpath TEXT NOT NULL,type INTEGER DEFAULT 0,ppath TEXT NOT NULL,pinyin TEXT NOT NULL,cpinyin TEXT NOT NULL);",
             table);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != 0) {
        fprintf(stderr, "db exec at %d error: %s\n", 4018, err);
        sqlite3_free(err);
    }
    printf("read from %s\n", emu->path.c_str());
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    ret = scanRomDir(emu->path, emu->imagePath, emu, table, false, db, path, name, 128, sql, bufSize, &cost, &added);
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    printf("%d items added cost %ldms\n", added, cost / 1000);
    sync();
    free(path);
    if (sql)
        free(sql);
    t = milliseconds() - t;
    printf("total cost %ldms\n", t / 1000);
    return ret;
}

// 0x22c88: directory filter for countDirectoryItems: a known ROM type or a .zip, no BIOS files.
bool isRomFile(const char *name, const char *dir)
{
    if (strncmp(name, "[BIOS]", 6) == 0)
        return false;
    if (findEmuForFile(name) >= 0)
        return true;
    if (strncmp(name, "neogeo.zip", 10) == 0)
        return false;
    if (strncmp(name, "gngeo_data.zip", 14) == 0)
        return false;
    const char *ext = strrchr(name, '.');
    if (ext && strncasecmp(ext, ".zip", 4) == 0)
        return true;
    return false;
}

// 0x22d90: open the emulator's ROM cache (<path>/<name>_cache6.db) and count the rows for a folder or
// a search; an empty cache is rebuilt from gamelist.xml, else from the file system.
int prepareRomCache(EmuInfo *emu, int arg, std::string dir, std::string search, char *buf, int size)
{
    int count = 0;
    int rc = -1;
    int t = 0;
    char *base = NULL;
    char *path = strdup(emu->path.c_str());
    base = romFolderName(path);
    sqlite3 *db;
    sqlite3_stmt *stmt;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec;
    snprintf(buf, size, "%s/%s_cache6.db", emu->path.c_str(), base);
    rc = sqlite3_open(buf, &db);
    if (rc == 0) {
        if (search.length() == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE ppath='%s'", base,
                     dir.length() ? dir.c_str() : ".");
        else if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", base,
                     search.c_str());
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", base,
                     search.c_str());
        if (sqlite3_prepare_v2(db, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                count = sqlite3_column_int(stmt, 0);
                if (count > 0)
                    goto done;
            }
        } else {
            printf("db exec at %d fail: %s\n", 4100, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    } else {
        printf("open db %s fail: %s\n", buf, sqlite3_errmsg(db));
        snprintf(buf, size, "%s/%s_cache6.db", emu->path.c_str(), base);
        unlink(buf);
        rc = sqlite3_open(buf, &db);
        printf("open again %s : %s\n", buf, sqlite3_errmsg(db));
    }
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec - t;
    printf("count %d item from db cost %d ms ppath %s\n", count, t, dir.c_str());
    if (count == 0 && search.length() == 0) {
        count = importGameList(emu->gameList.c_str(), arg, base, rc == 0 ? db : NULL);
        if (count > 0)
            goto done;
        if (g_progressTotal == 0)
            g_progressTotal = countDirectoryItems(emu->path, isRomFile);
        count = rebuildRomCache(emu, arg, base, rc == 0 ? db : NULL);
    }
done:
    if (rc == 0) {
        printf("rc is %d , close db %p\n", rc, db);
        sqlite3_close(db);
    }
    free(path);
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec - t;
    printf("prepare db in phase2 cost %d ms\n", t);
    return count;
}

// 0x23318: the ROM list of one emulator folder (or a search in it), backed by its cache database.
DBCachedTextMenu *buildRomList(EmuInfo *emu, int arg, std::string *dir, std::string *search)
{
    DBCachedTextMenu *menu = new DBCachedTextMenu(emu->label.c_str(), emu, arg, *search);
    printf("create_rom_browser_menu %p, search %s\n", menu, search->c_str());
    int count = 0;
    const int bufSize = 512;
    char *buf = (char *)malloc(bufSize);
    char *path = strdup(emu->path.c_str());
    char *base = romFolderName(path);
    count = prepareRomCache(emu, arg, *dir, *search, buf, bufSize);
    menu->setTotal(count);
    snprintf(buf, bufSize, "%s/%s_cache6.db", emu->path.c_str(), base);
    menu->open(buf, base, dir->length() ? dir->c_str() : ".");
    free(buf);
    free(path);
    return menu;
}
