// p06 0x23548-0x2849c: cached ROM rows, icon menus, settings items.
extern "C" SDL_Thread *SDL_CreateThread(int (*)(void *), void *);
extern "C" void SDL_WaitThread(SDL_Thread *, int *);
int removeRecent(std::list<RecentItem>*, const char*);
int sqlite3_column_type(sqlite3_stmt *, int);
const unsigned char *sqlite3_column_text(sqlite3_stmt *, int);
extern SDL_Surface *g_gameIcon; // 0x1821f8
extern SDL_Surface *g_folderIcon; // 0x1821f4

// 0x23548
void DBCacheWorker::run() {
    if (db != NULL) {
        std::string key;
        int i = 0;
        for (i = 0; i < len; i++) {
            snprintf(raw04, 512, "%d", offset + i);
            key = table + std::string("_") + raw04;
            void *cached = items->get(key);
            if (cached == NULL) break;
        }
        if (i < len) {
            FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "r");
            TextItem *item = NULL;
            if (search.length() == 0) {
                snprintf(raw04, 512, "SELECT * FROM %s_roms WHERE ppath='%s' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), dir.c_str(), offset + i, len-i);
            } else {
                if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
                    snprintf(raw04, 512, "SELECT * FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len-i);
                else
                    snprintf(raw04, 512, "SELECT * FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len-i);
                printf("mDBStrBuf = %s\n", raw04);
            }
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, raw04, -1, &stmt, NULL) == 0) {
                for (; i<len; i++) {
                    int rc;
                    if ((rc = sqlite3_step(stmt)) == 100) {
                        const char *disp = (const char*)sqlite3_column_text(stmt, 1);
                        const char *path = (const char*)sqlite3_column_text(stmt, 2);
                        const char *image = (const char*)sqlite3_column_text(stmt, 3);
                        int type = sqlite3_column_int(stmt, 4);
                        const char *parent = (const char*)sqlite3_column_text(stmt, 5); // Retain the unused column read.
                        const char *pinyin = (const char*)sqlite3_column_text(stmt, 6);
                        const char *cpinyin = (const char*)sqlite3_column_text(stmt, 7);
                        const char *label = NULL;
                        if (emu->hasCpinyin) label = GetGameName(getLanguageFile(), disp);
                        if (label == NULL) label = disp;
                        if (sqlite3_column_type(stmt, 1) == 3 && sqlite3_column_type(stmt, 2) == 3 && sqlite3_column_type(stmt, 4) == 1 && sqlite3_column_type(stmt, 5) == 3) {
                            if (type == 0) {
                                bool preview = search.length() != 0 && flag;
                                item = new TextItem(label, preview?NULL:g_gameIcon, new EmuAction(emu, label, path, image, x, true, type), true);
                                item->str28 = emu->label;
                                item->favorite = isFavourite(NULL, f, path);
                                item->flag24 = preview;
                            } else {
                                EmuAction *action = new EmuAction(emu, label, path, image, x, false, type);
                                action->folderArg = disp;
                                item = new TextItem(label, g_folderIcon, action, true);
                            }
                            if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0) item->str40 = cpinyin;
                            else item->str40 = pinyin;
                            snprintf(raw04, 512, "%d", offset + i);
                            key = table + std::string("_") + raw04;
                            items->put(key, item);
                            printf("put %s to cache , type=%d, path =%s, py=%s\n", key.c_str(), type, path, item->str40.c_str());
                        }
                        if (sqlite3_column_type(stmt, 3) == 3 && image[0] != 0) data->put(key, strdup(image));
                    } else {
                        printf("get item %d, db step return %d\n", offset + i, rc);
                        break;
                    }
                }
                sqlite3_finalize(stmt);
            } else printf("get item %d, exec fail : %s\n", offset + i, sqlite3_errmsg(db));
            if (f) fclose(f);
        }
    }
}
// 0x2407c
void DBCacheWorker::finished() {
    SDL_Event event;
    event.type = 24;
    event.user.code = 24;
    SDL_PushEvent(&event);
}

// 0x240b4
int DBCachedTextMenu::vfunc16() {
    int result = ListMenu::vfunc16();
    prefetchPrevious();
    return result;
}

// 0x240ec
int DBCachedTextMenu::vfunc17() {
    int result = ListMenu::vfunc17();
    prefetchNext();
    return result;
}

// 0x24128
int DBCachedTextMenu::vfunc9() {
    int result = ListMenu::vfunc9();
    prefetchNext();
    return result;
}

// 0x24164
int DBCachedTextMenu::vfunc10() {
    int result = ListMenu::vfunc10();
    prefetchPrevious();
    return result;
}

// 0x241a0
void DBCachedTextMenu::prefetchNext() {
    if (field140) {
        int offset = cur;
        int len = 6;
        if (offset + len>total)len = total-offset;
        DBCacheWorker *worker = new DBCacheWorker(field140, str15c, dbName, offset, len, field174, &items, &data, field414, &dir, &str18, false);
        workers.addWorker(worker);
    }
}

// 0x242dc
void DBCachedTextMenu::prefetchPrevious() {
    if (field140) {
        int offset = cur-5;
        int len = 6;
        if (offset<0) {
            offset = 0;
            len = cur + 1;
        }
        DBCacheWorker *worker = new DBCacheWorker(field140, str15c, dbName, offset, len, field174, &items, &data, field414, &dir, &str18, false);
        workers.addWorker(worker);
    }
}

// 0x24410
void *DBCachedTextMenu::getItem(int index) {
    snprintf(raw194, 640, "%d", index);
    std::string key = dbName + std::string("_") + raw194;
    void *item = items.get(key);
    if (item)return item;
    return NULL;
}

// 0x24584
void *DBCachedTextMenu::vfunc6(int index) {
    snprintf(raw194, 640, "%d", index);
    std::string key = dbName + std::string("_") + raw194;
    void *item = data.get(key);
    if (item)return item;
    return NULL;
}

// 0x246f8
void DBCachedTextMenu::restorePosition(int a, int b, int c) {
    ListMenu::restorePosition(a, b, c);
    int len = pageEnd-pageStart + 1;
    if (pageEnd == 0)len = 6;
    DBCacheWorker *worker = new DBCacheWorker(field140, str15c, dbName, pageStart, len, field174, &items, &data, field414, &dir, &str18, false);
    workers.addWorker(worker);
}

// 0x24834
int DBCachedTextMenu::open(const char *path, const char *table, const char *folder) {
    if (field140) {
        sqlite3_close(field140);
        field140 = NULL;
    }
    int rc = sqlite3_open(path, &field140);
    name = path;
    str15c = table;
    dir = folder;
    if (rc != 0)printf("open db %s fail, db is %p\n", path, field140);
    else {
        int len = pageEnd-pageStart + 1;
        if (pageEnd == 0)len = 5;
        DBCacheWorker *worker = new DBCacheWorker(field140, str15c, dbName, 0, 6, field174, &items, &data, field414, &dir, &str18, false);
        workers.addWorker(worker);
    }
    return rc;
}

// 0x24a14
void DBCachedTextMenu::vfunc12(int index) {
    TextItem *item = (TextItem*)getItem(index);
    if (item && item->action) {
        EmuAction *action = (EmuAction*)item->action;
        if (action->rompath.length() != 0) {
            unlink(action->rompath.c_str());
            snprintf(raw194, 640, "DELETE FROM %s_roms WHERE path='%s'", str15c.c_str(), action->rompath.c_str());
            sqlite3_stmt *stmt;
            char *err;
            if (sqlite3_exec(field140, raw194, NULL, NULL, &err) == 0) {
                launch_label_1a75c(item->label.c_str(), "/mnt/SDCARD/Roms/favourite.json");
                removeRecent(&g_recentGames, action->rompath.c_str());
                snprintf(raw194, 640, "SELECT COUNT(*) FROM %s_roms WHERE ppath='%s'", str15c.c_str(), dir.length() == 0?".":dir.c_str());
                if (sqlite3_prepare_v2(field140, raw194, -1, &stmt, NULL) == 0) {
                    if (sqlite3_step(stmt) == 100) {
                        int n = sqlite3_column_int(stmt, 0);
                        setTotal(n);
                        items.clear();
                        data.clear();
                        restorePosition(cur, pageStart, pageEnd);
                    }
                }
                sqlite3_finalize(stmt);
            } else {
                printf("db exec at %d fail: %s\n", 0x112b, err);
                sqlite3_free(err);
            }
        }
    }
}

// 0x24d28
static void deleteCachedTextItem(SDL_Surface *p) {
    TextItem *item = (TextItem*)p;
    delete item;
}

// 0x24d6c
static void freeCachedData(SDL_Surface *p) {
    void *data = p;
    free(data);
}

// 0x24d98
void DBCachedTextMenu::vfunc14(const char *search) {
    printf("search %s\n", search);
    str18 = search;
    if (str18.length() != 0) {
        const int size = 512;
        char *buf = (char*)malloc(size);
        sqlite3_stmt *stmt;
        if (field414->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", str15c.c_str(), search);
        else snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", str15c.c_str(), search);
        if (sqlite3_prepare_v2(field140, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                int n = sqlite3_column_int(stmt, 0);
                if (n>0) {
                    printf("search %s found %d items\n", search, n);
                    if (dir.length() != 0 && dir.compare(".") != 0)setRomListPosition(&g_romListPositions, field414->path + "/" + dir, cur, pageStart, pageEnd);
                    else setRomListPosition(&g_romListPositions, std::string(field414->path), cur, pageStart, pageEnd);
                    setTotal(n);
                    items.clear();
                    data.clear();
                    cur = 0;
                    pageStart = 0;
                    pageEnd = 0;
                    DBCacheWorker *worker = new DBCacheWorker(field140, str15c, dbName, 0, 5, field174, &items, &data, field414, &dir, &str18, false);
                    workers.addWorker(worker);
                }
            }
        } else printf("db exec at %d fail: %s\n", 0x1160, sqlite3_errmsg(field140));
        sqlite3_finalize(stmt);
        free(buf);
    }
}

// 0x25240
DBCachedTextMenu::DBCachedTextMenu(const char *label, EmuInfo *emu, int arg, std::string search)
    :TextMenuDrawer(false, true, 6, NULL), workers(3), items(deleteCachedTextItem), data(freeCachedData) {
    field140 = NULL;
    total = 0;
    dbName = label;
    field174 = arg;
    field414 = emu;
    str18 = search;
}

// 0x2540c
DBCachedTextMenu::~DBCachedTextMenu() {
    if (field140)sqlite3_close(field140);
}

// 0x25510
TextItem::TextItem(const char *text, SDL_Surface *surface, Action *a, bool flag) {
    label = text;
    value = surface;
    action = a;
    flag1c = flag;
    favorite = false;
    removed = false;
    flag24 = false;
    field70 = NULL;
}

// 0x25620
TextItem::~TextItem() {
    if (action && flag1c)delete action;
    if (field70)SDL_FreeSurface(field70);
}
// 0x256f0
IconMenuItem::IconMenuItem(int id, const char *normal, const char *selected, int bx, int by, int menu, void *arg, bool own) {
    labelId = id;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    if (!iconSurface)printf("load icon %s fail\n", normal);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    if (!iconSelectedSurface)printf("load icon %s fail\n", selected);
    icon = normal;
    iconSelected = selected;
    x = bx;
    y = by;
    param = arg;
    flag = own;
    menuId = menu;
}

// 0x258d4
IconMenuItem::IconMenuItem(const char *text, const char *normal, const char *selected, int bx, int by, int menu, void *arg, bool own) {
    label = text;
    labelId = -1;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    icon = normal;
    iconSelected = selected;
    x = bx;
    y = by;
    param = arg;
    flag = own;
    menuId = menu;
}

// 0x25a8c
IconMenuItem::~IconMenuItem() {
    if (flag && param)free((void*)param);
    if (iconSurface)SDL_FreeSurface(iconSurface);
    if (iconSelectedSurface)SDL_FreeSurface(iconSelectedSurface);
}

// 0x25b5c
void IconMenuItem::reload() {
    if (iconSurface)SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
    if (iconSelectedSurface)SDL_FreeSurface(iconSelectedSurface);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, iconSelected.c_str(), g_backBuffer, false);
}

// 0x25c68
static TTF_Font *defaultIconFont() {
    return g_iconFont;
}

Theme *findTheme(std::vector<Theme>*, const char*);
// 0x25c8c
IconMenu::IconMenu(int rows, int columns, bool stripes, bool dots, int marginX, int marginY, bool themeOffsets):ListMenu(true) {
    field54 = columns;
    field58 = rows;
    flag5c = stripes;
    flag5d = dots;
    field68 = 0;
    field6c = 0;
    fontFn = defaultIconFont;
    field74 = 0;
    field60 = marginX;
    field64 = marginY;
    Theme *theme = findTheme(&g_themeList, g_themePath);
    flag9c = themeOffsets;
    if (themeOffsets && theme) {
        field90 = theme->field144;
        field94 = theme->field140;
        field98 = theme->field148;
    } else {
        field90 = 0;
        field94 = 0;
        field98 = 0;
    }
}

// 0x25e48
IconMenu::~IconMenu() {
    puts("~IconMenu");
    freeItems();
    items.clear();
}

extern SDL_Surface *g_vertLine; // 0x1821cc
extern SDL_Surface *g_dotActive; // 0x1821a0
extern SDL_Surface *g_dotNormal; // 0x1821a4
extern SDL_Color g_textColor; // 0x182298
// 0x25efc
void IconMenu::vfunc7(SDL_Surface *dst) {
    SDL_Surface *surface, *background;
    int x, y, width, height;
    int i;
    unsigned j;
    TTF_Font *font;
    int index, row, column;
    IconMenuItem *item;
    SDL_Rect rect, src;
    SDL_Color color = g_textColor;
    if (field60>0) {
        width = (640-field60-field60)/field54;
        x = field60;
    } else {
        width = 640/field54;
        x = 0;
    }
    if (field64>0) {
        height = (360-field64-field64)/field58;
        y = field64 + 65;
    } else {
        height = 360/field58;
        y = 60;
    }
    if (items.size() == 0)return;
    {
        font = fontFn();
        if (pageStart == 0 && pageEnd == 0) {
            pageEnd = field58*field54-1;
            cur = pageStart;
        }
        if (cur<pageStart) {
            if (field58>1)pageStart = pageStart-field58*field54;
            else pageStart = pageStart-field58;
            if (pageStart<0)pageStart = 0;
            pageEnd = pageStart + field58*field54-1;
        }
        if (cur>pageEnd) {
            if (field58>1)pageStart = pageEnd + 1;
            else {
                if (pageStart + field58 + field58*field54-1<items.size())pageStart = pageStart + field58;
            }
            pageEnd = pageStart + field58*field54-1;
        }
        if ((unsigned)pageStart >= items.size())pageStart = items.size()-1;
        if ((unsigned)pageEnd >= items.size())pageEnd = items.size()-1;
        if (flag5c && g_vertLine) {
            rect.y = 60;
            for (i = 0;i<field54-1;i++) {
                rect.x = (i + 1)*width;
                SDL_UpperBlit(g_vertLine, NULL, dst, &rect);
            }
            if (g_listStripe) {
                rect.x = 0;
                for (i = 0;i<field58-1;i++) {
                    rect.y = (i + 1)*height + 60;
                    SDL_UpperBlit(g_listStripe, NULL, dst, &rect);
                }
            }
        }
        for (i = pageStart;i <= pageEnd;i++) {
            index = i-pageStart;
            row = index/field54;
            column = index-row*field54;
            item = (IconMenuItem *)items.at(i);
            if (i == cur) {
                surface = item->iconSurface;
                background = (SDL_Surface*)item->x;
                color = g_textColor;
            } else {
                surface = item->iconSelectedSurface;
                background = (SDL_Surface*)item->y;
                color = g_selectedColor;
            }
            if (background) {
                rect.x = column*width + x + (width-background->w)/2 + 2;
                rect.y = row*height + y + (height-background->h)/2 + 2;
                SDL_UpperBlit(background, NULL, dst, &rect);
            }
            if (surface) {
                if (field68>0 && field6c>0) {
                    rect.x = column*width + x + (width-field68)/2;
                    rect.y = row*height + y + 15;
                    src.x = (surface->w-field68)/2;
                    src.y = (surface->h-field6c)/2;
                    src.w = field68;
                    src.h = field6c;
                    if (flag9c) {
                        if (field90)rect.x = field90 + column*width + (width-field68)/2;
                        if (field94)rect.y = field94 + row*height;
                    }
                    SDL_UpperBlit(surface, &src, dst, &rect);
                    rect.x = column*width + x;
                    rect.y = row*height + y + field6c + 21;
                } else {
                    rect.x = column*width + x + (width-surface->w)/2;
                    rect.y = row*height + y + (height-surface->h)/2-10;
                    if (flag9c) {
                        if (field90)rect.x = field90 + column*width + (width-surface->w)/2;
                        if (field94)rect.y = field94 + row*height;
                    }
                    SDL_UpperBlit(surface, NULL, dst, &rect);
                    rect.x = column*width + x;
                    if (field74)rect.y = row*height + y + height-field74;
                    else rect.y = row*height + y + height*3/5;
                }
            } else {
                rect.x = column*width + x;
                if (field74)rect.y = row*height + y + height-field74;
                else rect.y = row*height + y + height*3/5;
            }
            rect.w = width;
            rect.h = 18;
            if (flag9c && field98)rect.y = field98;
            if (item->label.size() != 0)drawText(dst, font, rect, item->label.c_str(), color, 1, i == cur);
            else drawText(dst, font, rect, languageString(item->labelId), color, 1, i == cur);
        }
        if (flag5d && g_dotActive && g_dotNormal) {
            y = 390;
            width = items.size()*(g_dotActive->w + 5);
            x = (640-width)/2;
            rect.y = y;
            rect.w = g_dotActive->w;
            rect.h = g_dotActive->h;
            for (j = 0;j<items.size();j++) {
                rect.x = (g_dotActive->w + 5)*j + x;
                if (j == cur)SDL_UpperBlit(g_dotActive, NULL, dst, &rect);
                else SDL_UpperBlit(g_dotNormal, NULL, dst, &rect);
            }
        }
        if (field58>1)drawPageDots_311f4(dst, cur/(field58*field54) + 1, (items.size() + field58*field54-1)/(field58*field54));
    }
}

// 0x26fa0
void *IconMenu::getItem(int index) {
    if (items.size() != 0)return items.at(index);
    return NULL;
}

// 0x27014
static int prepareGlobalRomCache() {
    int total = 0;
    // The original leaves this buffer allocated; retain that behavior.
    const int size = 512;
    char *buf = (char*)malloc(size);
    char *path = NULL;
    char *base = NULL;
    int count;
    int n;
    std::vector<int> countsB, countsA;
    for (unsigned i = 0;i<g_romSourcesB.size();i++) {
        path = strdup(g_romSourcesB[i].path.c_str());
        base = database_strlen_20b1c(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_romSourcesB[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)n = countDirectoryItems(g_romSourcesB[i].path, isRomFile);
        else n = 0;
        countsB.push_back(n);
        total += n;
    }
    for (unsigned i = 0;i<g_romSourcesA.size();i++) {
        path = strdup(g_romSourcesA[i].path.c_str());
        base = database_strlen_20b1c(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_romSourcesA[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)n = countDirectoryItems(g_romSourcesA[i].path, isRomFile);
        else n = 0;
        countsA.push_back(n);
        total += n;
    }
    g_progressTotal = total;
    int unused = 0;
    for (unsigned i = 0;i<g_romSourcesB.size();i++) {
        n = countsB[i];
        if (n != 0) {
            count = database_lang_22d90(&g_romSourcesB[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_romSourcesB[i].path.c_str(), count);
        }
    }
    for (unsigned i = 0;i<g_romSourcesA.size();i++) {
        n = countsA[i];
        if (n != 0) {
            count = database_lang_22d90(&g_romSourcesA[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_romSourcesA[i].path.c_str(), count);
        }
    }
    g_progressTotal = 0;
    return total;
}

// 0x27724
int IconMenu::vfunc4() {
    MenuWindow *window;
    IconMenuItem *item = (IconMenuItem*)getItem(cur);
    if (item != NULL) {
        if (item->menuId == 24) {
            bool unused = false;
            int count = 0;
            g_busy = 1;
            BusyData busy;
            busy.type = 24;
            SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &busy);
            count = prepareGlobalRomCache();
            TrimusWindow *ime = new ImeWindow(153, new GlobalSearchAction, 153);
            g_busy = 0;
            SDL_WaitThread(thread, NULL);
            g_windowStack->push_back((TrimusWindow*)ime);
        } else {
            window = NULL;
            if (item->labelId >= 0)window = new MenuWindow(item->labelId, item->menuId, (void*)item->param);
            else window = new MenuWindow(item->label.c_str(), item->menuId, (void*)item->param);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
        }
        return 0;
    } else return -1;
}

// 0x2797c
void IconMenu::freeItems() {
    puts("IconMenu::freeListItem");
    for (int i = 0;i<items.size();i++) {
        IconMenuItem *item = (IconMenuItem *)items.at(i);
        delete item;
    }
}

// 0x27a30
void IconMenu::addItem(void *item) {
    items.push_back(item);
}

// 0x27a68
void IconMenu::vfunc11() {
    if (str78.compare(g_themePath) != 0) {
        for (int i = 0;i<items.size();i++) {
            IconMenuItem *item = (IconMenuItem *)items.at(i);
            item->reload();
        }
        str78 = g_themePath;
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (flag9c && theme) {
            field90 = theme->field144;
            field94 = theme->field140;
            field98 = theme->field148;
        }
    }
}

// 0x27bc4
int IconMenu::vfunc3() {
    int key = handleGlobalInput(0);
    switch (key) {
        case 5: {
            int index = cur-pageStart;
            if (index >= field54)cur = pageStart + index-field54;
            else cur = pageStart + (field58-1)*field54 + index%field54;
            if (cur<pageStart)cur = pageStart;
            if (cur>pageEnd)cur = pageEnd;
            break;
        }
        case 6: {
            int index = cur-pageStart;
            if (index<(field58-1)*field54)cur = pageStart + index + field54;
            else cur = pageStart + index%field54;
            if (cur<pageStart)cur = pageStart;
            if (cur>pageEnd)cur = pageEnd;
            break;
        }
        case 7: {
            int previous = cur;
            if (cur%field54 != 0)previous--;
            else previous -= (field54*(field58-1) + 1);
            if (previous >= 0)cur = previous;
            else {
                int n = count();
                if (field58 == 1) {
                    cur = n-1;
                    pageEnd = cur;
                    pageStart = cur-field54 + 1;
                } else {
                    cur = n-1;
                    pageEnd = cur;
                    pageStart = ((cur + field58*field54-1)/(field58*field54))*(field58*field54);
                }
            }
            break;
        }
        case 8: {
            int next = cur;
            if (cur%field54 == field54-1 && field58>1)next += field54*(field58-1) + 1;
            else next++;
            int n = count();
            if (next<n)cur = next;
            else if (field58 == 1) {
                cur = 0;
                pageStart = 0;
                pageEnd = field58*field54-1;
            } else {
                int start = ((cur + field54*field58-1)/(field54*field58))*(field54*field58);
                int row = (cur%(field58*field54) + (field54-1))/field54;
                int total = count();
                if (total>start) {
                    int remaining = total-start;
                    int rows = (field54-1 + remaining)/field54;
                    if (rows<row) {
                        cur = (rows-1)*field54 + start;
                        pageStart = start;
                        pageEnd = total-1;
                    }
                } else {
                    cur = (row-1)*field54;
                    pageStart = 0;
                    pageEnd = field58*field54-1;
                }
            }
            break;
        }
        case 1:case 4:return vfunc4();
        case 2:case 3:return 1;
        case 11:return 11;
        case 12:break;
        case 13:break;
        default:break;
    }
    return 0;
}

// 0x282d0
SettingMenuItem::SettingMenuItem(int id, const char *path, int arg) {
    labelId = id;
    if (path != NULL)iconSurface = loadSkinImage(g_themePath, g_skinSubdir, path, g_backBuffer, false);
    else iconSurface = NULL;
    handler = arg;
    if (path)icon = path;
}

// 0x283b8
SettingMenuItem::~SettingMenuItem() {
    if (iconSurface)SDL_FreeSurface(iconSurface);
}

// 0x28408
void SettingMenuItem::reload() {
    if (iconSurface)SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
}
