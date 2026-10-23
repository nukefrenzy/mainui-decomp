// tu_015b20 part p08 (0x2b788-0x2e424): color, language, theme and file-browser menus.
extern bool g_customDisplayTiming; // 0x17e6ec
extern Theme g_skin; // 0x181974
Theme *findTheme(std::vector<Theme> *, const char *);
unsigned sdCardFreeMB();
extern "C" {
struct ZipFile;
struct ZipEntry;
ZipFile *zipOpen(const char *); // 0x14d8b0
ZipEntry *zipOpenEntry(ZipFile *, const char *, unsigned); // 0x14d418
void zipCloseEntry(ZipEntry *); // 0x14d858
void zipClose(ZipFile *); // 0x14da1c
}

// 0x2b788: the display color menu (WIN_COLOR): lightness, hue, saturation and contrast, each 0-20.
ColorMenu::ColorMenu() : TextMenu(false, true, 6, NULL)
{
    loadItems();
    leftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    colorRightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
}

// 0x2b8bc (D0 0x2b96c)
ColorMenu::~ColorMenu()
{
    freeListItem();
    if (leftArrow) SDL_FreeSurface(leftArrow);
    if (colorRightArrow) SDL_FreeSurface(colorRightArrow);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2b9a0: draws the four rows with their values between left/right arrows.
void ColorMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0};
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (row < 5 && g_listStripe) {
            dest.x = 0;
            dest.y = (row + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        }
        if (rowBackground) {
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->iconSurface) {
            area.y = (60 - item->iconSurface->h) / 2 + y;
            SDL_UpperBlit(item->iconSurface, NULL, dst, &area);
            area.x = area.x + item->iconSurface->w + 5;
        }
        area.y = y;
        area.w = rect.w - (leftArrow ? leftArrow->w : 0) - (colorRightArrow ? colorRightArrow->w : 0) - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, shadowSelected ? i == cur : false);
        area.x = rect.w + x - (colorRightArrow ? colorRightArrow->w : 0) - 240;
        area.w = 200;
        int type = item->handler;
        int level;
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 3)
            snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, MONITOR_SATURATION), 20);
        else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            snprintf(text, 32, "%02d/%02d", level, 20);
        }
        area.y += 2;
        drawText(dst, g_imeFont, area, text, color, 1, false);
        if (leftArrow) {
            area.y = (60 - leftArrow->h) / 2 + y;
            area.x = rect.w + x - (colorRightArrow ? colorRightArrow->w : 0) - leftArrow->w - 240;
            SDL_UpperBlit(leftArrow, NULL, dst, &area);
        }
        if (colorRightArrow) {
            area.y = (60 - colorRightArrow->h) / 2 + y;
            area.x = rect.w + x - colorRightArrow->w - 40;
            SDL_UpperBlit(colorRightArrow, NULL, dst, &area);
        }
        i++;
        row++;
    }
}

// 0x2c274: left/right change the selected value and apply all four to the panel (the first change
// also reprograms the panel timing); closing shows a "reboot to apply changes" note.
int ColorMenu::handleKey()
{
    int level;
    int key = ListMenu::handleKey();
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return 0;
    int type = item->handler;
    if (key == LIST_LEFT) {
        if (g_customDisplayTiming) {
            applyPanelTiming();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_LUMINATION, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_HUE, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, MONITOR_SATURATION);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_SATURATION, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_CONTRAST, level);
        }
        setLcdColor(GetKeyShm(g_keyShm, MONITOR_LUMINATION), GetKeyShm(g_keyShm, MONITOR_HUE), GetKeyShm(g_keyShm, MONITOR_SATURATION), GetKeyShm(g_keyShm, MONITOR_CONTRAST));
        return 0;
    } else if (key == LIST_RIGHT) {
        if (g_customDisplayTiming) {
            applyPanelTiming();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_LUMINATION, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_HUE, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, MONITOR_SATURATION);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_SATURATION, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_CONTRAST, level);
        }
        setLcdColor(GetKeyShm(g_keyShm, MONITOR_LUMINATION), GetKeyShm(g_keyShm, MONITOR_HUE), GetKeyShm(g_keyShm, MONITOR_SATURATION), GetKeyShm(g_keyShm, MONITOR_CONTRAST));
        return 0;
    } else if (key == LIST_CLOSE) {
        NotifyWindow *window = new NotifyWindow(languageString(STR_REBOOT_TO_APPLY_CHANGES), languageString(STR_OK), "skin/bg-button-f.png", NULL);
        pushWindow(window);
        return key;
    }
    return key;
}

// 0x2c73c: A does nothing here.
int ColorMenu::onSelect()
{
    return 0;
}

// 0x2c760: deletes every row.
void ColorMenu::freeListItem()
{
    puts("ColorMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        delete item;
    }
    items.clear();
}

// 0x2c824: the four rows (their handler numbers 1-4 are local to this menu).
void ColorMenu::loadItems()
{
    addItem((TextItem *)new SettingMenuItem(STR_LIGHTNESS, NULL, 1));
    addItem((TextItem *)new SettingMenuItem(STR_HUE, NULL, 2));
    addItem((TextItem *)new SettingMenuItem(STR_SATURATION, NULL, 3));
    addItem((TextItem *)new SettingMenuItem(STR_CONTRAST, NULL, 4));
}

// 0x2c950: one row per language offered (languageChoices).
LanguageMenu::LanguageMenu(std::vector<LanguageChoice> choices) : TextMenu(false, true, 6, NULL)
{
    languages = choices;
    for (int i = 0; i < languages.size(); i++) addItem(new TextItem(languages[i].name.c_str(), NULL, NULL, true));
}

// 0x2caa8 (D0 0x2caf4)
LanguageMenu::~LanguageMenu()
{
}

// 0x2cb28: switch to the selected language and reopen the theme's fonts for it.
int LanguageMenu::onSelect()
{
    if ((unsigned)cur < languages.size()) {
        setLanguageFile(languages[cur].file.c_str());
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (theme)
            changeLanguageFonts(theme, &g_fontSize);
        else
            changeLanguageFonts(&g_skin, &g_fontSize);
        return 1;
    }
    return -1;
}

// 0x2cc14: the built-in theme ("Default") and one row per installed theme.
ThemeMenu::ThemeMenu() : TextMenu(false, true, 6, NULL)
{
    addItem(new TextItem(languageString(STR_DEFAULT), NULL, NULL, true));
    for (int i = 0; i < g_themeList.size(); i++) addItem(new TextItem(g_themeList[i].name.c_str(), NULL, NULL, true));
}

// 0x2cd80 (D0 0x2cdbc)
ThemeMenu::~ThemeMenu()
{
}

// 0x2cdf0: switch to the selected theme: reload fonts, images and sounds and restart the music.
int ThemeMenu::onSelect()
{
    if ((unsigned)cur < g_themeList.size() + 1) {
        if (g_themePath) free(g_themePath);
        int unused = 0; // Retained original stack local.
        if (cur > 0) {
            g_themePath = strdup(g_themeList[cur - 1].root.c_str());
            changeLanguageFonts(&g_themeList[cur - 1], &g_fontSize);
            freeTheme();
            loadTheme(&g_themeList[cur - 1], &g_fontSize);
            Mix_PlayMusic(g_themeMusic, -1);
        } else {
            g_themePath = strdup(g_skinSubdir);
            changeLanguageFonts(&g_skin, &g_fontSize);
            freeTheme();
            loadTheme(&g_skin, &g_fontSize);
            Mix_PlayMusic(g_themeMusic, -1);
        }
        return 1;
    }
    return -1;
}

// Legacy suffix table retained in the target even though fileType reads configured extensions.
// Original writable pointer table: 0x17e6f0-0x17e764.
const char *g_legacyExtensions[] = {
    ".smc", ".fig", ".sfc", ".gd3", ".gd7", ".dx2", ".bsx", ".swc",
    ".gen", ".md", ".smd", ".32x", ".sms", ".rom", ".gg", ".sg",
    ".nes", ".fds", ".unf", ".unif", ".gba", ".agb", ".gbz", ".pbp",
    ".iso", ".cue", ".gb", ".gbc", ".pce"};

// 0x2cfc0: index of the emulator (Emu folder) whose extension list contains this file's extension,
// or -1.
int findEmuForFile(const char *name)
{
    const char *suffix = strrchr(name, '.');
    if (suffix == NULL) return -1;
    {
        for (int i = 0; i < g_emuFolders.size(); i++) {
            for (int j = 0; j < g_emuFolders[i].extensions.size(); j++) {
                if (strncasecmp(suffix + 1, g_emuFolders[i].extensions[j].c_str(), g_emuFolders[i].extensions[j].size()) == 0) return i;
            }
        }
    }
    return -1;
}

// 0x2d158: confirmed system update: write the zip's path to /tmp/.try_upgrade_file and quit MainUI
// (result 3), so the shell script can run the update.
static int requestUpgrade(void *arg)
{
    const char *path = (const char *)arg;
    FILE *file = fopen("/tmp/.try_upgrade_file", "w+");
    if (file) {
        fputs(path, file);
        fclose(file);
    }
    return 3;
}

// 0x2d1bc: A on a file-browser row: a TrimuiUpdate*.zip asks to update the system (needs 500 MB
// free), another file starts in the emulator that handles its extension; a folder posts
// EVENT_OPEN_DIRECTORY.
int FileAction::execute(void *param)
{
    TextItem *item = (TextItem *)param;
    if (kind == FILE_KIND_FILE) {
        const char *suffix = strrchr(path.c_str(), '.');
        if (suffix == NULL) {
            printf("openfile_action no suffix %s\n", path.c_str());
            return -1;
        }
        if (strncasecmp(item->label.c_str(), "TrimuiUpdate", 12) == 0 && strncasecmp(suffix, ".zip", 4) == 0) {
            int freeMB = sdCardFreeMB();
            if (freeMB >= 500) {
                strcpy(g_launchCmd, path.c_str());
                DialogWindow *window = new DialogWindow(STR_UPGRADE, STR_UPDATE_SYSTEM, false, new GeneralAction(-1, requestUpgrade, g_launchCmd));
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            } else {
                snprintf(g_launchCmd, 0x800, languageString(STR_SD_SPACE_LOW), freeMB);
                NotifyWindow *window = new NotifyWindow(g_launchCmd, languageString(STR_OK), "skin/bg-button-f.png", NULL);
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            }
        } else {
            int type = findEmuForFile(path.c_str());
            if (type >= 0 && (unsigned)type < g_emuFolders.size()) return launchRom(item->label.c_str(), &g_emuFolders[type], path.c_str(), NULL, WIN_ROM_LIST);
        }
    } else if (kind == FILE_KIND_DIRECTORY) {
        SDL_UserEvent event;
        event.type = SDL_USEREVENT;
        event.code = EVENT_OPEN_DIRECTORY;
        event.data1 = (void *)path.c_str();
        if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    }
    return 0;
}

// 0x2d574: which files the browser lists: ROMs of a known extension, update zips that contain an
// "updater" entry, and license files.
static bool fileBrowserFilter(const char *name, const char *path)
{
    if (findEmuForFile(name) >= 0) return true;
    const char *suffix = strrchr(name, '.');
    if (strncasecmp(name, "TrimuiUpdate", 12) == 0 && suffix && strncasecmp(suffix, ".zip", 4) == 0) {
        ZipFile *zip = zipOpen(path);
        if (zip) {
            ZipEntry *entry;
            if ((entry = zipOpenEntry(zip, "updater", 0)) == NULL) {
                zipClose(zip);
                printf("Can't found %s in %s\n", "updater", name);
                return false;
            }
            zipCloseEntry(entry);
            zipClose(zip);
            return true;
        }
    }
    if (strncmp(name, "COPYING", 7) == 0) return true;
    if (strncasecmp(name, "LICENSE", 7) == 0) return true;
    return false;
}

// 0x2d700: the file browser (WIN_FILES), starting at path.
FileBrowserMenu::FileBrowserMenu(const char *path) : ListMenu(false), cache(fileBrowserFilter)
{
    fileIcon = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false);
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false));
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-folder.png", g_backBuffer, false));
    selectionBar = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    dirItems = cache.open(path);
    highlightSelected = true;
}

// 0x2d908 (D0 0x2da58)
FileBrowserMenu::~FileBrowserMenu()
{
    // Original iterates an empty local vector, leaving the icon entries unfreed.
    std::vector<SDL_Surface *> surfaces;
    for (int i = 0; i < surfaces.size(); i++)
        if (surfaces[i]) SDL_FreeSurface(surfaces[i]);
    if (fileIcon) SDL_FreeSurface(fileIcon);
    if (selectionBar) SDL_FreeSurface(selectionBar);
}

// 0x2da8c: rows in the current folder.
int FileBrowserMenu::count()
{
    if (dirItems) return dirItems->size();
    return 0;
}

// 0x2dad4: A on a row: run its FileAction.
int FileBrowserMenu::onSelect()
{
    TextItem *item = (TextItem *)getItem(cur);
    if (item && item->action) {
        int result;
        if ((result = item->action->execute(item)) >= 0) return result;
    }
    return -1;
}

// 0x2db8c: the row at index.
void *FileBrowserMenu::getItem(int index)
{
    if (index >= 0 && (unsigned)index < dirItems->size()) return (*dirItems)[index];
    return NULL;
}

// 0x2dc14: the icon for a row: file or folder.
SDL_Surface *FileBrowserMenu::itemIcon(TextItem *item)
{
    FileAction *action = (FileAction *)item->action;
    int index = action->kind - FILE_KIND_FILE;
    if (index >= 0 && (unsigned)index < icons.size()) return icons[index];
    return fileIcon;
}

// 0x2dcbc: draws the visible rows: highlight, icon and name.
void FileBrowserMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= dirItems->size() - 1) pageEnd = dirItems->size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        TextItem *item = (TextItem *)getItem(i);
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (i == cur) {
            if (selectionBar) {
                dest.x = x;
                dest.y = (60 - selectionBar->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectionBar, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        SDL_Surface *icon = itemIcon(item);
        if (icon) {
            area.y = (60 - icon->h) / 2 + y;
            SDL_UpperBlit(icon, NULL, dst, &area);
            area.x = area.x + icon->w + 5;
        }
        area.y = (60 - height) / 2 + y;
        area.w = rect.w - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, item->label.c_str(), color, 0, highlightSelected ? i == cur : false);
        i++;
        row++;
    }
}

// 0x2e0f0: one button: up/down move, A/Y open, B/X close, MENU passes through; EVENT_OPEN_DIRECTORY
// enters the folder (or goes up for "..").
int FileBrowserMenu::handleKey()
{
    const char *path;
    SDL_Event event;
    int key = handleGlobalInput(&event);
    switch (key) {
        case BUTTON_UP:
            moveUp();
            break;
        case BUTTON_DOWN:
            moveDown();
            break;
        case BUTTON_A:
        case BUTTON_Y:
            puts("a");
            return onSelect();
        case BUTTON_B:
        case BUTTON_X:
            puts("b or c");
            return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        case EVENT_OPEN_DIRECTORY:
            path = (const char *)event.user.data1;
            printf("change dir %s\n", path);
            if (strncmp(path + strlen(path) - 2, "..", 2) == 0 || strncmp(path + strlen(path) - 3, "../", 3) == 0)
                dirItems = cache.parent();
            else
                dirItems = cache.child(path);
            restorePosition(0, 0, 5);
            break;
        default: break;
    }
    return -1;
}

// 0x2e318: free callback for caches of surfaces.
void freeSurfaceCallback(SDL_Surface *surface)
{
    SDL_Surface *value = surface;
    SDL_FreeSurface(value);
}

// 0x2e344: background job: load a ROM thumbnail scaled to fit 250x360 into the preview cache.
void LoadSurfaceWorker::run()
{
    SDL_Surface *surface = loadThumbnail(path.c_str(), 250, 360);
    if (surface) cache->put(path, surface);
}

// 0x2e3ec: the image is in: ask for a redraw.
void LoadSurfaceWorker::finished()
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = EVENT_REDRAW;
    SDL_PushEvent(&event);
}

// Inline definitions follow the record destructors first used by p03-p07.
inline LoadSurfaceWorker::LoadSurfaceWorker(std::string path, LruCache *cache)
{
    this->cache = cache;
    this->path = path;
}
inline LoadSurfaceWorker::~LoadSurfaceWorker() {}
