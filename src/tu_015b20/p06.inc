// tu_015b20 part p06 (0x23548-0x2849c): ROM cache rows, icon menus and settings rows.


int removeRecent(std::list<RecentItem> *, const char *);
extern SDL_Surface *g_gameIcon; // 0x1821f8
extern SDL_Surface *g_folderIcon; // 0x1821f4

// 0x23548: background job of the ROM lists: loads rows [offset, offset+len) of a folder (or of a
// search, by pinyin or Chinese pinyin) from the cache database into the items cache, skipping rows
// already cached; each row is a game (type 0) or a subfolder, with its favorite mark, and its
// image path goes to the data cache.
void DBCacheWorker::run()
{
    if (db != NULL) {
        std::string key;
        int i = 0;
        for (i = 0; i < len; i++) {
            snprintf(sqlBuffer, 512, "%d", offset + i);
            key = table + std::string("_") + sqlBuffer;
            void *cached = items->get(key);
            if (cached == NULL) break;
        }
        if (i < len) {
            FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "r");
            TextItem *item = NULL;
            if (search.length() == 0) {
                snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE ppath='%s' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), dir.c_str(), offset + i, len - i);
            } else {
                if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
                    snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len - i);
                else
                    snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len - i);
                printf("mDBStrBuf = %s\n", sqlBuffer);
            }
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sqlBuffer, -1, &stmt, NULL) == 0) {
                for (; i < len; i++) {
                    int rc;
                    if ((rc = sqlite3_step(stmt)) == 100) {
                        const char *disp = (const char *)sqlite3_column_text(stmt, 1);
                        const char *path = (const char *)sqlite3_column_text(stmt, 2);
                        const char *image = (const char *)sqlite3_column_text(stmt, 3);
                        int type = sqlite3_column_int(stmt, 4);
                        const char *parent = (const char *)sqlite3_column_text(stmt, 5); // Retain the unused column read.
                        const char *pinyin = (const char *)sqlite3_column_text(stmt, 6);
                        const char *cpinyin = (const char *)sqlite3_column_text(stmt, 7);
                        const char *label = NULL;
                        if (emu->hasCpinyin) label = GetGameName(getLanguageFile(), disp);
                        if (label == NULL) label = disp;
                        if (sqlite3_column_type(stmt, 1) == 3 && sqlite3_column_type(stmt, 2) == 3 && sqlite3_column_type(stmt, 4) == 1 && sqlite3_column_type(stmt, 5) == 3) {
                            if (type == 0) {
                                bool preview = search.length() != 0 && previewSearchResults;
                                item = new TextItem(label, preview ? NULL : g_gameIcon, new EmuAction(emu, label, path, image, rowWindowId, true, type), true);
                                item->subtitle = emu->label;
                                item->favorite = isFavorite(NULL, f, path);
                                item->showThumbnail = preview;
                            } else {
                                EmuAction *action = new EmuAction(emu, label, path, image, rowWindowId, false, type);
                                action->folderArg = disp;
                                item = new TextItem(label, g_folderIcon, action, true);
                            }
                            if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
                                item->displayLabel = cpinyin;
                            else
                                item->displayLabel = pinyin;
                            snprintf(sqlBuffer, 512, "%d", offset + i);
                            key = table + std::string("_") + sqlBuffer;
                            items->put(key, item);
                            printf("put %s to cache , type=%d, path =%s, py=%s\n", key.c_str(), type, path, item->displayLabel.c_str());
                        }
                        if (sqlite3_column_type(stmt, 3) == 3 && image[0] != 0) data->put(key, strdup(image));
                    } else {
                        printf("get item %d, db step return %d\n", offset + i, rc);
                        break;
                    }
                }
                sqlite3_finalize(stmt);
            } else
                printf("get item %d, exec fail : %s\n", offset + i, sqlite3_errmsg(db));
            if (f) fclose(f);
        }
    }
}
// 0x2407c: the rows are in: ask for a redraw.
void DBCacheWorker::finished()
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = EVENT_REDRAW;
    SDL_PushEvent(&event);
}

// 0x240b4: page up: move as usual, then queue the rows that came into view.
int DBCachedTextMenu::pageUp()
{
    int result = ListMenu::pageUp();
    prefetchPrevious();
    return result;
}

// 0x240ec: the same for page down.
int DBCachedTextMenu::pageDown()
{
    int result = ListMenu::pageDown();
    prefetchNext();
    return result;
}

// 0x24128: the same for down.
int DBCachedTextMenu::moveDown()
{
    int result = ListMenu::moveDown();
    prefetchNext();
    return result;
}

// 0x24164: the same for up.
int DBCachedTextMenu::moveUp()
{
    int result = ListMenu::moveUp();
    prefetchPrevious();
    return result;
}

// 0x241a0: queue the six rows from the cursor down.
void DBCachedTextMenu::prefetchNext()
{
    if (cacheDb) {
        int offset = cur;
        int len = 6;
        if (offset + len > total) len = total - offset;
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, offset, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
}

// 0x242dc: queue the rows above the cursor (up to five, and the cursor row).
void DBCachedTextMenu::prefetchPrevious()
{
    if (cacheDb) {
        int offset = cur - 5;
        int len = 6;
        if (offset < 0) {
            offset = 0;
            len = cur + 1;
        }
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, offset, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
}

// 0x24410: the row at index if it has been loaded, else NULL (not loaded yet).
void *DBCachedTextMenu::getItem(int index)
{
    snprintf(keyBuffer, 640, "%d", index);
    std::string key = dbName + std::string("_") + keyBuffer;
    void *item = items.get(key);
    if (item) return item;
    return NULL;
}

// 0x24584: a row's image path if loaded, else NULL.
void *DBCachedTextMenu::getItemData(int index)
{
    snprintf(keyBuffer, 640, "%d", index);
    std::string key = dbName + std::string("_") + keyBuffer;
    void *item = data.get(key);
    if (item) return item;
    return NULL;
}

// 0x246f8: back to the list: restore the position and load the visible page.
void DBCachedTextMenu::restorePosition(int savedCur, int savedStart, int savedEnd)
{
    ListMenu::restorePosition(savedCur, savedStart, savedEnd);
    int len = pageEnd - pageStart + 1;
    if (pageEnd == 0) len = 6;
    DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, pageStart, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
    workers.addWorker(worker);
}

// 0x24834: opens the cache database at path for the table prefix and folder, and loads the first
// rows.
int DBCachedTextMenu::open(const char *path, const char *table, const char *folder)
{
    if (cacheDb) {
        sqlite3_close(cacheDb);
        cacheDb = NULL;
    }
    int rc = sqlite3_open(path, &cacheDb);
    name = path;
    tablePrefix = table;
    dir = folder;
    if (rc != 0)
        printf("open db %s fail, db is %p\n", path, cacheDb);
    else {
        int len = pageEnd - pageStart + 1;
        if (pageEnd == 0) len = 5;
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, 0, 6, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
    return rc;
}

// 0x24a14: deletes the ROM at index: the file itself, its database row, its favorite and its
// Recent entry; then recounts the folder and reloads the page.
void DBCachedTextMenu::removeItem(int index)
{
    TextItem *item = (TextItem *)getItem(index);
    if (item && item->action) {
        EmuAction *action = (EmuAction *)item->action;
        if (action->rompath.length() != 0) {
            unlink(action->rompath.c_str());
            snprintf(keyBuffer, 640, "DELETE FROM %s_roms WHERE path='%s'", tablePrefix.c_str(), action->rompath.c_str());
            sqlite3_stmt *stmt;
            char *err;
            if (sqlite3_exec(cacheDb, keyBuffer, NULL, NULL, &err) == 0) {
                removeFavorite(item->label.c_str(), "/mnt/SDCARD/Roms/favourite.json");
                removeRecent(&g_recentGames, action->rompath.c_str());
                snprintf(keyBuffer, 640, "SELECT COUNT(*) FROM %s_roms WHERE ppath='%s'", tablePrefix.c_str(), dir.length() == 0 ? "." : dir.c_str());
                if (sqlite3_prepare_v2(cacheDb, keyBuffer, -1, &stmt, NULL) == 0) {
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

// 0x24d28: free callbacks of the row cache (TextItem) and the data cache (strings).
static void deleteCachedTextItem(SDL_Surface *value)
{
    TextItem *item = (TextItem *)value;
    delete item;
}

// 0x24d6c
static void freeCachedData(SDL_Surface *value)
{
    void *data = value;
    free(data);
}

// 0x24d98: filters the list by a search string (pinyin initials) if anything matches: remembers the
// position in the folder, then shows the first matches.
void DBCachedTextMenu::setSearch(const char *search)
{
    printf("search %s\n", search);
    searchText = search;
    if (searchText.length() != 0) {
        const int size = 512;
        char *buf = (char *)malloc(size);
        sqlite3_stmt *stmt;
        if (emuInfo->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", tablePrefix.c_str(), search);
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", tablePrefix.c_str(), search);
        if (sqlite3_prepare_v2(cacheDb, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                int n = sqlite3_column_int(stmt, 0);
                if (n > 0) {
                    printf("search %s found %d items\n", search, n);
                    if (dir.length() != 0 && dir.compare(".") != 0)
                        setRomListPosition(&g_romListPositions, emuInfo->path + "/" + dir, cur, pageStart, pageEnd);
                    else
                        setRomListPosition(&g_romListPositions, std::string(emuInfo->path), cur, pageStart, pageEnd);
                    setTotal(n);
                    items.clear();
                    data.clear();
                    cur = 0;
                    pageStart = 0;
                    pageEnd = 0;
                    DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, 0, 5, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
                    workers.addWorker(worker);
                }
            }
        } else
            printf("db exec at %d fail: %s\n", 0x1160, sqlite3_errmsg(cacheDb));
        sqlite3_finalize(stmt);
        free(buf);
    }
}

// 0x25240: a ROM list for one emulator (label, emu), whose rows' actions carry the window id arg;
// three background loaders.
DBCachedTextMenu::DBCachedTextMenu(const char *label, EmuInfo *emu, int arg, std::string search)
    : TextMenuDrawer(false, true, 6, NULL), workers(3), items(deleteCachedTextItem), data(freeCachedData)
{
    cacheDb = NULL;
    total = 0;
    dbName = label;
    listWindowId = arg;
    emuInfo = emu;
    searchText = search;
}

// 0x2540c
DBCachedTextMenu::~DBCachedTextMenu()
{
    if (cacheDb) sqlite3_close(cacheDb);
}

// 0x25510: one text row: label, icon (value), action (deleted with the row when ownsAction).
TextItem::TextItem(const char *text, SDL_Surface *surface, Action *act, bool owns)
{
    label = text;
    value = surface;
    action = act;
    ownsAction = owns;
    favorite = false;
    removed = false;
    showThumbnail = false;
    thumbnailSurface = NULL;
}

// 0x25620
TextItem::~TextItem()
{
    if (action && ownsAction) delete action;
    if (thumbnailSurface) SDL_FreeSurface(thumbnailSurface);
}
// 0x256f0: a main-menu style icon with a language-string label; menu is the window id it opens and
// arg its parameter (an EmuInfo for emulators). Note: callers pass the focused image as `normal`
// and the normal one as `selected`; drawList uses them that way round.
IconMenuItem::IconMenuItem(int id, const char *normal, const char *selected, SDL_Surface *bx, SDL_Surface *by, int menu, void *arg, bool own)
{
    labelId = id;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    if (!iconSurface) printf("load icon %s fail\n", normal);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    if (!iconSelectedSurface) printf("load icon %s fail\n", selected);
    icon = normal;
    iconSelected = selected;
    background = bx;
    backgroundSelected = by;
    param = arg;
    ownsParam = own;
    menuId = menu;
}

// 0x258d4: the same with a literal label (emulator and RetroArch icons).
IconMenuItem::IconMenuItem(const char *text, const char *normal, const char *selected, SDL_Surface *bx, SDL_Surface *by, int menu, void *arg, bool own)
{
    label = text;
    labelId = -1;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    icon = normal;
    iconSelected = selected;
    background = bx;
    backgroundSelected = by;
    param = arg;
    ownsParam = own;
    menuId = menu;
}

// 0x25a8c
IconMenuItem::~IconMenuItem()
{
    if (ownsParam && param) free((void *)param);
    if (iconSurface) SDL_FreeSurface(iconSurface);
    if (iconSelectedSurface) SDL_FreeSurface(iconSelectedSurface);
}

// 0x25b5c: reload both icons from the current theme.
void IconMenuItem::reload()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
    if (iconSelectedSurface) SDL_FreeSurface(iconSelectedSurface);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, iconSelected.c_str(), g_backBuffer, false);
}

// 0x25c68: the font of icon labels.
static TTF_Font *defaultIconFont()
{
    return g_iconFont;
}

Theme *findTheme(std::vector<Theme> *, const char *);
// 0x25c8c: a grid of icons: rows x columns per page, optional separator lines (stripes) and page
// dots, margins, and optionally the theme's grid offsets.
IconMenu::IconMenu(int rows, int columns, bool stripes, bool dots, int marginX, int marginY, bool themeOffsets) : ListMenu(true)
{
    gridColumns = columns;
    gridRows = rows;
    showStripes = stripes;
    showPageDots = dots;
    iconCropWidth = 0;
    iconCropHeight = 0;
    fontFn = defaultIconFont;
    labelOffsetY = 0;
    gridMarginX = marginX;
    gridMarginY = marginY;
    Theme *theme = findTheme(&g_themeList, g_themePath);
    useThemeOffsets = themeOffsets;
    if (themeOffsets && theme) {
        iconOffsetX = theme->gridIconOffsetX;
        iconOffsetY = theme->gridIconOffsetY;
        textOffsetY = theme->gridTextOffsetY;
    } else {
        iconOffsetX = 0;
        iconOffsetY = 0;
        textOffsetY = 0;
    }
}

// 0x25e48
IconMenu::~IconMenu()
{
    puts(__func__);
    freeItems();
    items.clear();
}

extern SDL_Surface *g_vertLine; // 0x1821cc
extern SDL_Surface *g_dotActive; // 0x1821a0
extern SDL_Surface *g_dotNormal; // 0x1821a4
extern SDL_Color g_textColor; // 0x182298
// 0x25efc: draws the visible page of the grid: separator lines, each icon with its background
// (crop, offsets), its label, the page dots and the "page/total" count.
void IconMenu::drawList(SDL_Surface *dst)
{
    SDL_Surface *surface, *background;
    int x, y, width, height;
    int i;
    unsigned j;
    TTF_Font *font;
    int index, row, column;
    IconMenuItem *item;
    SDL_Rect rect, src;
    SDL_Color color = g_textColor;
    if (gridMarginX > 0) {
        width = (640 - gridMarginX - gridMarginX) / gridColumns;
        x = gridMarginX;
    } else {
        width = 640 / gridColumns;
        x = 0;
    }
    if (gridMarginY > 0) {
        height = (360 - gridMarginY - gridMarginY) / gridRows;
        y = gridMarginY + 65;
    } else {
        height = 360 / gridRows;
        y = 60;
    }
    if (items.size() == 0) return;
    {
        font = fontFn();
        if (pageStart == 0 && pageEnd == 0) {
            pageEnd = gridRows * gridColumns - 1;
            cur = pageStart;
        }
        if (cur < pageStart) {
            if (gridRows > 1)
                pageStart = pageStart - gridRows * gridColumns;
            else
                pageStart = pageStart - gridRows;
            if (pageStart < 0) pageStart = 0;
            pageEnd = pageStart + gridRows * gridColumns - 1;
        }
        if (cur > pageEnd) {
            if (gridRows > 1)
                pageStart = pageEnd + 1;
            else {
                if (pageStart + gridRows + gridRows * gridColumns - 1 < items.size()) pageStart = pageStart + gridRows;
            }
            pageEnd = pageStart + gridRows * gridColumns - 1;
        }
        if ((unsigned)pageStart >= items.size()) pageStart = items.size() - 1;
        if ((unsigned)pageEnd >= items.size()) pageEnd = items.size() - 1;
        if (showStripes && g_vertLine) {
            rect.y = 60;
            for (i = 0; i < gridColumns - 1; i++) {
                rect.x = (i + 1) * width;
                SDL_UpperBlit(g_vertLine, NULL, dst, &rect);
            }
            if (g_listStripe) {
                rect.x = 0;
                for (i = 0; i < gridRows - 1; i++) {
                    rect.y = (i + 1) * height + 60;
                    SDL_UpperBlit(g_listStripe, NULL, dst, &rect);
                }
            }
        }
        for (i = pageStart; i <= pageEnd; i++) {
            index = i - pageStart;
            row = index / gridColumns;
            column = index - row * gridColumns;
            item = (IconMenuItem *)items.at(i);
            if (i == cur) {
                surface = item->iconSurface;
                background = item->background;
                color = g_textColor;
            } else {
                surface = item->iconSelectedSurface;
                background = item->backgroundSelected;
                color = g_selectedColor;
            }
            if (background) {
                rect.x = column * width + x + (width - background->w) / 2 + 2;
                rect.y = row * height + y + (height - background->h) / 2 + 2;
                SDL_UpperBlit(background, NULL, dst, &rect);
            }
            if (surface) {
                if (iconCropWidth > 0 && iconCropHeight > 0) {
                    rect.x = column * width + x + (width - iconCropWidth) / 2;
                    rect.y = row * height + y + 15;
                    src.x = (surface->w - iconCropWidth) / 2;
                    src.y = (surface->h - iconCropHeight) / 2;
                    src.w = iconCropWidth;
                    src.h = iconCropHeight;
                    if (useThemeOffsets) {
                        if (iconOffsetX) rect.x = iconOffsetX + column * width + (width - iconCropWidth) / 2;
                        if (iconOffsetY) rect.y = iconOffsetY + row * height;
                    }
                    SDL_UpperBlit(surface, &src, dst, &rect);
                    rect.x = column * width + x;
                    rect.y = row * height + y + iconCropHeight + 21;
                } else {
                    rect.x = column * width + x + (width - surface->w) / 2;
                    rect.y = row * height + y + (height - surface->h) / 2 - 10;
                    if (useThemeOffsets) {
                        if (iconOffsetX) rect.x = iconOffsetX + column * width + (width - surface->w) / 2;
                        if (iconOffsetY) rect.y = iconOffsetY + row * height;
                    }
                    SDL_UpperBlit(surface, NULL, dst, &rect);
                    rect.x = column * width + x;
                    if (labelOffsetY)
                        rect.y = row * height + y + height - labelOffsetY;
                    else
                        rect.y = row * height + y + height * 3 / 5;
                }
            } else {
                rect.x = column * width + x;
                if (labelOffsetY)
                    rect.y = row * height + y + height - labelOffsetY;
                else
                    rect.y = row * height + y + height * 3 / 5;
            }
            rect.w = width;
            rect.h = 18;
            if (useThemeOffsets && textOffsetY) rect.y = textOffsetY;
            if (item->label.size() != 0)
                drawText(dst, font, rect, item->label.c_str(), color, 1, i == cur);
            else
                drawText(dst, font, rect, languageString(item->labelId), color, 1, i == cur);
        }
        if (showPageDots && g_dotActive && g_dotNormal) {
            y = 390;
            width = items.size() * (g_dotActive->w + 5);
            x = (640 - width) / 2;
            rect.y = y;
            rect.w = g_dotActive->w;
            rect.h = g_dotActive->h;
            for (j = 0; j < items.size(); j++) {
                rect.x = (g_dotActive->w + 5) * j + x;
                if (j == cur)
                    SDL_UpperBlit(g_dotActive, NULL, dst, &rect);
                else
                    SDL_UpperBlit(g_dotNormal, NULL, dst, &rect);
            }
        }
        if (gridRows > 1) drawPageCount(dst, cur / (gridRows * gridColumns) + 1, (items.size() + gridRows * gridColumns - 1) / (gridRows * gridColumns));
    }
}

// 0x26fa0: the item at index.
void *IconMenu::getItem(int index)
{
    if (items.size() != 0) return items.at(index);
    return NULL;
}

// 0x27014: before a global search: counts the ROMs of every emulator and RetroArch app that has no
// cache yet (for the progress count) and builds those caches.
static int prepareGlobalRomCache()
{
    int total = 0;
    // The original leaves this buffer allocated; retain that behavior.
    const int size = 512;
    char *buf = (char *)malloc(size);
    char *path = NULL;
    char *base = NULL;
    int count;
    int n;
    std::vector<int> countsB, countsA;
    for (unsigned i = 0; i < g_rappFolders.size(); i++) {
        path = strdup(g_rappFolders[i].path.c_str());
        base = romFolderName(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_rappFolders[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)
            n = countDirectoryItems(g_rappFolders[i].path, isRomFile);
        else
            n = 0;
        countsB.push_back(n);
        total += n;
    }
    for (unsigned i = 0; i < g_emuFolders.size(); i++) {
        path = strdup(g_emuFolders[i].path.c_str());
        base = romFolderName(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_emuFolders[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)
            n = countDirectoryItems(g_emuFolders[i].path, isRomFile);
        else
            n = 0;
        countsA.push_back(n);
        total += n;
    }
    g_progressTotal = total;
    int unused = 0;
    for (unsigned i = 0; i < g_rappFolders.size(); i++) {
        n = countsB[i];
        if (n != 0) {
            count = prepareRomCache(&g_rappFolders[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_rappFolders[i].path.c_str(), count);
        }
    }
    for (unsigned i = 0; i < g_emuFolders.size(); i++) {
        n = countsA[i];
        if (n != 0) {
            count = prepareRomCache(&g_emuFolders[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_emuFolders[i].path.c_str(), count);
        }
    }
    g_progressTotal = 0;
    return total;
}

// 0x27724: opens what the selected icon leads to: its menu window, or for the search icon
// (WIN_SEARCH) builds the missing caches behind the busy screen and opens the keyboard.
int IconMenu::onSelect()
{
    MenuWindow *window;
    IconMenuItem *item = (IconMenuItem *)getItem(cur);
    if (item != NULL) {
        if (item->menuId == WIN_SEARCH) {
            bool unused = false;
            int count = 0;
            g_busy = 1;
            BusyData busy;
            busy.type = WIN_SEARCH;
            SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &busy);
            count = prepareGlobalRomCache();
            TrimusWindow *ime = new ImeWindow(STR_SEARCH, new GlobalSearchAction, STR_SEARCH);
            g_busy = 0;
            SDL_WaitThread(thread, NULL);
            g_windowStack->push_back((TrimusWindow *)ime);
        } else {
            window = NULL;
            if (item->labelId >= 0)
                window = new MenuWindow(item->labelId, item->menuId, (void *)item->param);
            else
                window = new MenuWindow(item->label.c_str(), item->menuId, (void *)item->param);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
        }
        return 0;
    } else
        return -1;
}

// 0x2797c: deletes every item.
void IconMenu::freeItems()
{
    puts("IconMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        IconMenuItem *item = (IconMenuItem *)items.at(i);
        delete item;
    }
}

// 0x27a30
void IconMenu::addItem(void *item)
{
    items.push_back(item);
}

// 0x27a68: after a theme change: reload the icons and the theme's grid offsets.
void IconMenu::reloadTheme()
{
    if (loadedThemePath.compare(g_themePath) != 0) {
        for (int i = 0; i < items.size(); i++) {
            IconMenuItem *item = (IconMenuItem *)items.at(i);
            item->reload();
        }
        loadedThemePath = g_themePath;
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (useThemeOffsets && theme) {
            iconOffsetX = theme->gridIconOffsetX;
            iconOffsetY = theme->gridIconOffsetY;
            textOffsetY = theme->gridTextOffsetY;
        }
    }
}

// 0x27bc4: grid navigation: up/down within the column, left/right to the previous/next item across
// rows and pages (wrapping); A/Y open, B/X close, MENU passes through.
int IconMenu::handleKey()
{
    int key = handleGlobalInput(0);
    switch (key) {
        case BUTTON_UP: {
            int index = cur - pageStart;
            if (index >= gridColumns)
                cur = pageStart + index - gridColumns;
            else
                cur = pageStart + (gridRows - 1) * gridColumns + index % gridColumns;
            if (cur < pageStart) cur = pageStart;
            if (cur > pageEnd) cur = pageEnd;
            break;
        }
        case BUTTON_DOWN: {
            int index = cur - pageStart;
            if (index < (gridRows - 1) * gridColumns)
                cur = pageStart + index + gridColumns;
            else
                cur = pageStart + index % gridColumns;
            if (cur < pageStart) cur = pageStart;
            if (cur > pageEnd) cur = pageEnd;
            break;
        }
        case BUTTON_LEFT: {
            int previous = cur;
            if (cur % gridColumns != 0)
                previous--;
            else
                previous -= (gridColumns * (gridRows - 1) + 1);
            if (previous >= 0)
                cur = previous;
            else {
                int n = count();
                if (gridRows == 1) {
                    cur = n - 1;
                    pageEnd = cur;
                    pageStart = cur - gridColumns + 1;
                } else {
                    cur = n - 1;
                    pageEnd = cur;
                    pageStart = ((cur + gridRows * gridColumns - 1) / (gridRows * gridColumns)) * (gridRows * gridColumns);
                }
            }
            break;
        }
        case BUTTON_RIGHT: {
            int next = cur;
            if (cur % gridColumns == gridColumns - 1 && gridRows > 1)
                next += gridColumns * (gridRows - 1) + 1;
            else
                next++;
            int n = count();
            if (next < n)
                cur = next;
            else if (gridRows == 1) {
                cur = 0;
                pageStart = 0;
                pageEnd = gridRows * gridColumns - 1;
            } else {
                int start = ((cur + gridColumns * gridRows - 1) / (gridColumns * gridRows)) * (gridColumns * gridRows);
                int row = (cur % (gridRows * gridColumns) + (gridColumns - 1)) / gridColumns;
                int total = count();
                if (total > start) {
                    int remaining = total - start;
                    int rows = (gridColumns - 1 + remaining) / gridColumns;
                    if (rows < row) {
                        cur = (rows - 1) * gridColumns + start;
                        pageStart = start;
                        pageEnd = total - 1;
                    }
                } else {
                    cur = (row - 1) * gridColumns;
                    pageStart = 0;
                    pageEnd = gridRows * gridColumns - 1;
                }
            }
            break;
        }
        case BUTTON_A:
        case BUTTON_Y: return onSelect();
        case BUTTON_B:
        case BUTTON_X: return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        case BUTTON_HOTKEY1: break;
        case BUTTON_HOTKEY2: break;
        default: break;
    }
    return 0;
}

// 0x282d0: one settings row: label, icon and the number of its handler (what A or left/right do;
// see SettingMenu).
SettingMenuItem::SettingMenuItem(int id, const char *path, int arg)
{
    labelId = id;
    if (path != NULL)
        iconSurface = loadSkinImage(g_themePath, g_skinSubdir, path, g_backBuffer, false);
    else
        iconSurface = NULL;
    handler = arg;
    if (path) icon = path;
}

// 0x283b8
SettingMenuItem::~SettingMenuItem()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
}

// 0x28408: reload the icon from the current theme.
void SettingMenuItem::reload()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
}
