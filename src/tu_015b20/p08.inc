// p08 0x2b788-0x2e424: colour/language/theme menus and file navigation.
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

// 0x2b788
ColorMenu::ColorMenu() : TextMenu(false, true, 6, NULL) {
    loadItems();
    surface104 = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    surface108 = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
}

// 0x2b8bc (D0 0x2b96c)
ColorMenu::~ColorMenu() {
    vfunc18();
    if (surface104) SDL_FreeSurface(surface104);
    if (surface108) SDL_FreeSurface(surface108);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2b9a0
void ColorMenu::vfunc7(SDL_Surface *dst) {
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0
    };
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd-5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned) pageEnd >= items.size()-1) pageEnd = items.size()-1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem*)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row*60;
        if (row < 5 && g_listStripe) {
            dest.x = 0;
            dest.y = (row + 2)*60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        }
        if (surface48) {
            dest.x = x;
            dest.y = (60-surface48->h)/2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(surface48, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60-selectedBackground->h)/2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->iconSurface) {
            area.y = (60-item->iconSurface->h)/2 + y;
            SDL_UpperBlit(item->iconSurface, NULL, dst, &area);
            area.x = area.x + item->iconSurface->w + 5;
        }
        area.y = y;
        area.w = rect.w-(surface104?surface104->w:0)-(surface108?surface108->w:0)-20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, flagC0?i == cur:false);
        area.x = rect.w + x-(surface108?surface108->w:0)-240;
        area.w = 200;
        int type = item->handler;
        int level;
        if (type == 1) {
            level = GetKeyShm(g_keyShm, 8);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, 9);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 3) snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, 10), 20);
        else if (type == 4) {
            level = GetKeyShm(g_keyShm, 11);
            snprintf(text, 32, "%02d/%02d", level, 20);
        }
        area.y += 2;
        drawText(dst, g_imeFont, area, text, color, 1, false);
        if (surface104) {
            area.y = (60-surface104->h)/2 + y;
            area.x = rect.w + x-(surface108?surface108->w:0)-surface104->w-240;
            SDL_UpperBlit(surface104, NULL, dst, &area);
        }
        if (surface108) {
            area.y = (60-surface108->h)/2 + y;
            area.x = rect.w + x-surface108->w-40;
            SDL_UpperBlit(surface108, NULL, dst, &area);
        }
        i++;
        row++;
    }
}

// 0x2c274
int ColorMenu::vfunc3() {
    int level;
    int key = ListMenu::vfunc3();
    SettingMenuItem *item = (SettingMenuItem*)items[cur];
    if (item == NULL) return 0;
    int type = item->handler;
    if (key == 5) {
        if (g_customDisplayTiming) {
            display_memset_2b5bc();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, 8);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, 8, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, 9);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, 9, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, 10);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, 10, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, 11);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, 11, level);
        }
        display_getlcd_1bea4(GetKeyShm(g_keyShm, 8), GetKeyShm(g_keyShm, 9), GetKeyShm(g_keyShm, 10), GetKeyShm(g_keyShm, 11));
        return 0;
    } else if (key == 6) {
        if (g_customDisplayTiming) {
            display_memset_2b5bc();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, 8);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, 8, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, 9);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, 9, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, 10);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, 10, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, 11);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, 11, level);
        }
        display_getlcd_1bea4(GetKeyShm(g_keyShm, 8), GetKeyShm(g_keyShm, 9), GetKeyShm(g_keyShm, 10), GetKeyShm(g_keyShm, 11));
        return 0;
    } else if (key == 1) {
        NotifyWindow *window = new NotifyWindow(languageString(STR_REBOOT_TO_APPLY_CHANGES), languageString(STR_OK), "skin/bg-button-f.png", NULL);
        pushWindow(window);
        return key;
    }
    return key;
}

// 0x2c73c
int ColorMenu::vfunc4() {
    return 0;
}

// 0x2c760
void ColorMenu::vfunc18() {
    puts("ColorMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        SettingMenuItem *item = (SettingMenuItem*)items[i];
        delete item;
    }
    items.clear();
}

// 0x2c824
void ColorMenu::loadItems() {
    addItem((TextItem*)new SettingMenuItem(119, NULL, 1));
    addItem((TextItem*)new SettingMenuItem(120, NULL, 2));
    addItem((TextItem*)new SettingMenuItem(121, NULL, 3));
    addItem((TextItem*)new SettingMenuItem(122, NULL, 4));
}

// 0x2c950
LanguageMenu::LanguageMenu(std::vector<LanguageChoice> choices) : TextMenu(false, true, 6, NULL) {
    languages = choices;
    for (int i = 0; i < languages.size(); i++) addItem(new TextItem(languages[i].name.c_str(), NULL, NULL, true));
}

// 0x2caa8 (D0 0x2caf4)
LanguageMenu::~LanguageMenu() {
}

// 0x2cb28
int LanguageMenu::vfunc4() {
    if ((unsigned)cur < languages.size()) {
        setLanguageFile(languages[cur].file.c_str());
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (theme) changeLanguageFonts(theme, &g_fontSize);
        else changeLanguageFonts(&g_skin, &g_fontSize);
        return 1;
    }
    return -1;
}

// 0x2cc14
ThemeMenu::ThemeMenu() : TextMenu(false, true, 6, NULL) {
    addItem(new TextItem(languageString(STR_DEFAULT), NULL, NULL, true));
    for (int i = 0; i < g_themeList.size(); i++) addItem(new TextItem(g_themeList[i].name.c_str(), NULL, NULL, true));
}

// 0x2cd80 (D0 0x2cdbc)
ThemeMenu::~ThemeMenu() {
}

// 0x2cdf0
int ThemeMenu::vfunc4() {
    if ((unsigned)cur < g_themeList.size() + 1) {
        if (g_themePath) free(g_themePath);
        int unused = 0; // Retained original stack local.
        if (cur > 0) {
            g_themePath = strdup(g_themeList[cur-1].root.c_str());
            changeLanguageFonts(&g_themeList[cur-1], &g_fontSize);
            freeTheme();
            loadTheme(&g_themeList[cur-1], &g_fontSize);
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
    ".iso", ".cue", ".gb", ".gbc", ".pce"
};

// 0x2cfc0
int fileType_2cfc0(const char *name) {
    const char *suffix = strrchr(name, '.');
    if (suffix == NULL) return -1;
    {
        for (int i = 0; i < g_romSourcesA.size(); i++) {
            for (int j = 0; j < g_romSourcesA[i].extensions.size(); j++) {
                if (strncasecmp(suffix + 1, g_romSourcesA[i].extensions[j].c_str(), g_romSourcesA[i].extensions[j].size()) == 0) return i;
            }
        }
    }
    return -1;
}

// 0x2d158
static int requestUpgrade(void *arg) {
    const char *path = (const char*)arg;
    FILE *file = fopen("/tmp/.try_upgrade_file", "w+");
    if (file) {
        fputs(path, file);
        fclose(file);
    }
    return 3;
}

// 0x2d1bc
int FileAction::execute(void *param) {
    TextItem *item = (TextItem*)param;
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
                DialogWindow *window = new DialogWindow(56, 57, false, new GeneralAction(-1, requestUpgrade, g_launchCmd));
                window->onShow();
                g_windowStack->push_back((TrimusWindow*)window);
            } else {
                snprintf(g_launchCmd, 0x800, languageString(STR_SD_SPACE_LOW), freeMB);
                NotifyWindow *window = new NotifyWindow(g_launchCmd, languageString(STR_OK), "skin/bg-button-f.png", NULL);
                window->onShow();
                g_windowStack->push_back((TrimusWindow*)window);
            }
        } else {
            int type = fileType_2cfc0(path.c_str());
            if (type >= 0 && (unsigned)type < g_romSourcesA.size()) return launchRom(item->label.c_str(), &g_romSourcesA[type], path.c_str(), NULL, 5);
        }
    } else if (kind == FILE_KIND_DIRECTORY) {
        struct FileEvent {
            unsigned char type;
            int code;
            void *data1, *data2;
        } event;
        event.type = SDL_USEREVENT;
        event.code = EVENT_OPEN_DIRECTORY;
        event.data1 = (void*)path.c_str();
        if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    }
    return 0;
}

// 0x2d574
static bool fileBrowserFilter(const char *name, const char *path) {
    if (fileType_2cfc0(name) >= 0) return true;
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

// 0x2d700
FileBrowserMenu::FileBrowserMenu(const char *path) : ListMenu(false), cache(fileBrowserFilter) {
    surface74 = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false);
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false));
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-folder.png", g_backBuffer, false));
    surface78 = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    dirItems = cache.open(path);
    flag7c = true;
}

// 0x2d908 (D0 0x2da58)
FileBrowserMenu::~FileBrowserMenu() {
    // Original iterates an empty local vector, leaving the icon entries unfreed.
    std::vector<SDL_Surface *> surfaces;
    for (int i = 0; i < surfaces.size(); i++) if (surfaces[i]) SDL_FreeSurface(surfaces[i]);
    if (surface74) SDL_FreeSurface(surface74);
    if (surface78) SDL_FreeSurface(surface78);
}

// 0x2da8c
int FileBrowserMenu::count() {
    if (dirItems) return dirItems->size();
    return 0;
}

// 0x2dad4
int FileBrowserMenu::vfunc4() {
    TextItem *item = (TextItem*)getItem(cur);
    if (item && item->action) {
        int result;
        if ((result = item->action->execute(item)) >= 0) return result;
    }
    return -1;
}

// 0x2db8c
void *FileBrowserMenu::getItem(int index) {
    if (index >= 0 && (unsigned)index < dirItems->size()) return (*dirItems)[index];
    return NULL;
}

// 0x2dc14
SDL_Surface *FileBrowserMenu::itemIcon(TextItem *item) {
    FileAction *action = (FileAction*)item->action;
    int index = action->kind - 101;
    if (index >= 0 && (unsigned)index < icons.size()) return icons[index];
    return surface74;
}

// 0x2dcbc
void FileBrowserMenu::vfunc7(SDL_Surface *dst) {
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
        pageStart = pageEnd-5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned) pageEnd >= dirItems->size()-1) pageEnd = dirItems->size()-1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        TextItem *item = (TextItem*)getItem(i);
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row*60;
        if (i == cur) {
            if (surface78) {
                dest.x = x;
                dest.y = (60-surface78->h)/2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(surface78, &area, dst, &dest);
            } else color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        SDL_Surface *icon = itemIcon(item);
        if (icon) {
            area.y = (60-icon->h)/2 + y;
            SDL_UpperBlit(icon, NULL, dst, &area);
            area.x = area.x + icon->w + 5;
        }
        area.y = (60-height)/2 + y;
        area.w = rect.w-20;
        area.h = 60;
        drawText(dst, g_imeFont, area, item->label.c_str(), color, 0, flag7c?i == cur:false);
        i++;
        row++;
    }
}

// 0x2e0f0
int FileBrowserMenu::vfunc3() {
    const char *path;
    SDL_Event event;
    int key = handleGlobalInput(&event);
    switch (key) {
        case 5: vfunc10();
        break;
        case 6: vfunc9();
        break;
        case 1: case 4: puts("a");
        return vfunc4();
        case 2: case 3: puts("b or c");
        return 1;
        case 11: return 11;
        case 25:
        path = (const char*)event.user.data1;
        printf("change dir %s\n", path);
        if (strncmp(path + strlen(path)-2, "..", 2) == 0 || strncmp(path + strlen(path)-3, "../", 3) == 0) dirItems = cache.parent();
        else dirItems = cache.child(path);
        restorePosition(0, 0, 5);
        break;
        default: break;
    }
    return -1;
}

// 0x2e318
void freeSurfaceCallback(SDL_Surface *surface) {
    SDL_Surface *value = surface;
    SDL_FreeSurface(value);
}

// 0x2e344
void LoadSurfaceWorker::run() {
    SDL_Surface *surface = loadThumbnail(path.c_str(), 250, 360);
    if (surface) cache->put(path, surface);
}

// 0x2e3ec
void LoadSurfaceWorker::finished() {
    MainEvent event;
    event.type = 24;
    event.user.code = 24;
    SDL_PushEvent(&event);
}

// Inline definitions follow the record destructors first used by p03-p07.
inline LoadSurfaceWorker::LoadSurfaceWorker(std::string path, LruCache *cache) {
    this->cache = cache;
    this->path = path;
}
inline LoadSurfaceWorker::~LoadSurfaceWorker() {}
