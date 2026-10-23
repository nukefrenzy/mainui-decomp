// p09 0x2e424-0x3225c: MenuWindow and FloatWindow.
// 0x2e424: a window that shows a list (built in onShow by createMenu, or the ROM list for an
// emulator in entry) under a title from the language strings.
MenuWindow::MenuWindow(int titleLabelId, int id, void *entry)
    : TrimusWindow(titleLabelId, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = NULL;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = entry;
    showFullPath = false;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2e6d4: the same with a list that is already built (Wi-Fi, hotspot, the root menu).
MenuWindow::MenuWindow(int titleLabelId, int id, ListMenu *menu)
    : TrimusWindow(titleLabelId, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = menu;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = 0;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2e978: the same with a literal title (a folder name).
MenuWindow::MenuWindow(const char *folder, int id, void *entry)
    : TrimusWindow(-1, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = NULL;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = entry;
    showFullPath = false;
    titleText = folder;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2ec3c (D0 0x2ed78)
MenuWindow::~MenuWindow()
{
    if (cachedScreen != 0) {
        SDL_FreeSurface(cachedScreen);
    }
    if (iconTF != 0) {
        SDL_FreeSurface(iconTF);
    }
    if (ownsHeaderIcon && headerIcon != 0) {
        SDL_FreeSurface(headerIcon);
    }
}

int findEmuByLabelPrefix(std::vector<EmuInfo> *, const char *);
// 0x121074
extern SDL_Surface *g_previewBg;
// 0x1821fc
extern SDL_Surface *g_emptyImage;
// 0x1821b4
extern SDL_Color g_listColor;
// 0x1822ac
extern SDL_Color g_hintColor;
// 0x1822a8
extern char g_countBuffer[0x100];
// 0x180754
// 0x2edac: what is saved in /tmp/state.json for this window: position, emulator or RetroArch-app
// name, folder and search string.
void MenuWindow::saveState(cJSON *obj)
{
    cJSON *position = cJSON_CreateNumber(menu->getCur());
    cJSON_AddItemToObject(obj, "currpos", position);
    cJSON *start = cJSON_CreateNumber(menu->getPageStart());
    cJSON_AddItemToObject(obj, "pagestart", start);
    cJSON *end = cJSON_CreateNumber(menu->getPageEnd());
    cJSON_AddItemToObject(obj, "pageend", end);
    if (emuEntry) {
        EmuInfo *emu = (EmuInfo *)emuEntry;
        cJSON *name = cJSON_CreateString(emu->label.c_str());
        if (id == WIN_ROM_LIST)
            cJSON_AddItemToObject(obj, "emuname", name);
        else
            cJSON_AddItemToObject(obj, "raname", name);
    }
    if (parentPath.length() != 0) cJSON_AddStringToObject(obj, "ppath", parentPath.c_str());
    std::string search = menu->getSearch();
    if (search.length() != 0) cJSON_AddStringToObject(obj, "searchstr", search.c_str());
}
// 0x2efcc: reads that back (onShow then rebuilds the list at the saved position).
int MenuWindow::restoreState(cJSON *obj)
{
    cJSONItem *position = (cJSONItem *)cJSON_GetObjectItem(obj, "currpos");
    cJSONItem *start = (cJSONItem *)cJSON_GetObjectItem(obj, "pagestart");
    cJSONItem *end = (cJSONItem *)cJSON_GetObjectItem(obj, "pageend");
    if (position && position->type == 8 && start && start->type == 8 && end && end->type == 8) {
        savedCurPos = position->valueint;
        savedPageStart = start->valueint;
        savedPageEnd = end->valueint;
    }
    cJSON *name = cJSON_GetObjectItem(obj, "emuname");
    if (name) {
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_emuFolders, label);
        if (index >= 0) {
            emuEntry = &g_emuFolders[index];
            headerText = g_emuFolders[index].label;
        } else
            emuEntry = NULL;
    } else {
        name = cJSON_GetObjectItem(obj, "raname");
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_rappFolders, label);
        if (index >= 0)
            emuEntry = &g_rappFolders[index];
        else
            emuEntry = NULL;
    }
    cJSONItem *folder = (cJSONItem *)cJSON_GetObjectItem(obj, "ppath");
    if (folder && folder->type == 16) parentPath = folder->valuestring;
    cJSONItem *search = (cJSONItem *)cJSON_GetObjectItem(obj, "searchstr");
    if (search && search->type == 16) path = search->valuestring;
    return 0;
}
// 0x2f290: builds the list when first shown: the global search, a ROM list (at the position last
// used in that folder) or createMenu(id).
void MenuWindow::onShow()
{
    if (menu == NULL) {
        if (id == WIN_SEARCH) {
            menu = new MultiDBCachedTextMenu(path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else if (id != WIN_ROM_LIST && id != WIN_RETROARCH_LIST) {
            menu = createMenu(id, path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else {
            if (emuEntry) {
                EmuInfo *emu = (EmuInfo *)emuEntry;
                RomListPosition position;
                menu = createRomListMenu(emu, id, &parentPath, &path);
                std::string folder = emu->path;
                if (parentPath.length() != 0) folder = emu->path + "/" + parentPath;
                if (getRomListPosition(&g_romListPositions, folder, &position) >= 0)
                    menu->restorePosition(position.current, position.start, position.selected);
                else
                    menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
            }
        }
        hasPreview = isListMenuId(id) && id != WIN_FILES && id != WIN_APPS;
        if (menu) isListMenuId(id);
    }
}
// 0x2f6c4: keeps the preview image for the selected row: an app's icon, or the ROM's image from the
// database or the Imgs folder; loads it in the background if it is not cached.
void MenuWindow::updateThumbnail()
{
    if (hasPreview) {
        TextItem *item = (TextItem *)menu->getItem(menu->getCur());
        if (item) {
            std::string unused;
            std::string image;
            if (item->action->result == WIN_APPS) {
                AppAction *action = (AppAction *)item->action;
                AppInfo *app = action->app;
                image = app->iconSelected;
            } else {
                const char *explicitPath = (const char *)menu->getItemData(menu->getCur());
                if (explicitPath) {
                    image = explicitPath;
                    struct stat st;
                    if (stat(image.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) image = "";
                } else {
                    EmuAction *action = (EmuAction *)item->action;
                    EmuInfo *emu = action->emu;
                    std::string game = action->rompath;
                    image = makeThumbPath(&game, emu->imagePath.c_str());
                }
            }
            if (image.size() != 0 && (image.compare(shownImageKey) != 0 || previewImage == NULL)) {
                shownImageKey = image;
                previewImage = (SDL_Surface *)thumbnailCache.get(image);
                if (previewImage == NULL) {
                    LoadSurfaceWorker *worker = new LoadSurfaceWorker(image, &thumbnailCache);
                    thumbnailLoader.addWorker(worker);
                    pendingImageKey = image;
                } else
                    pendingImageKey = "";
            } else {
                if (image.size() == 0) {
                    shownImageKey = "";
                    previewImage = NULL;
                }
            }
        }
    }
}
// 0x2fb14: each frame: drop removed favourites, follow the selection with the preview, and reload
// images after a theme change.
void MenuWindow::update()
{
    if (menu) {
        bool changed = removeDeletedFavorites(menu, id);
        if (menuChanged != true) menuChanged = changed;
        updateThumbnail();
        menu->reloadTheme();
    }
    if (themePath.compare(g_themePath) != 0) {
        if (iconTF) SDL_FreeSurface(iconTF);
        iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}
// 0x2fc74: on close: save the favourites if any were removed, and remember the position in a ROM
// folder.
void MenuWindow::onHide()
{
    if (menu) {
        if (id == WIN_FAVORITES && menuChanged) {
            TextMenu *text = (TextMenu *)menu;
            saveFavorites("/mnt/SDCARD/Roms/favourite.json", text->getItems());
        }
        if (emuEntry && (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST)) {
            EmuInfo *emu = (EmuInfo *)emuEntry;
            int position = menu->getCur();
            int start = menu->getPageStart();
            int end = menu->getPageEnd();
            std::string search = menu->getSearch();
            if (search.length() == 0) {
                if (parentPath.length() != 0)
                    setRomListPosition(&g_romListPositions, emu->path + "/" + parentPath, position, start, end);
                else
                    setRomListPosition(&g_romListPositions, std::string(emu->path), position, start, end);
            }
        }
        delete menu;
    }
}
// 0x2ff04: one button: the list handles it first. MENU opens the options pop-up for the row (open,
// add to favourites, delete the ROM, search; or refresh the ROMs on the main and games menus;
// remove or clear in favourites; clear Recent). RIGHT opens the game's detail window; START toggles
// the full ROM path in the header.
int MenuWindow::handleInput()
{
    int rows;
    PopUpWindow *popup;
    int key;
    if (menu) {
        key = menu->handleKey();
        if (key == BUTTON_MENU) {
            if (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST || id == WIN_APPS || id == WIN_SEARCH) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    unsigned char canOperate = item->action->finished();
                    if (canOperate) {
                        rows = 2;
                        TextMenu *options = new TextMenu(false, false, 6, NULL);
                        if (id == WIN_APPS)
                            options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                        else
                            options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                        options->addItem(new TextItem(languageString(STR_ADD_FAVORITE), NULL, new AddFavoriteAction(item, id), true));
                        if (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST) {
                            DeleteRomAction *remove = new DeleteRomAction(menu, id);
                            remove->itemIndex = menu->getCur();
                            Action *confirm = new PopupDialogAction(STR_REMOVE_ROM, STR_REMOVE_ROM_TEXT, true, remove);
                            options->addItem(new TextItem(languageString(STR_DELETE), NULL, confirm, true));
                            rows++;
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(STR_SEARCH), NULL, searchAction, true));
                                rows++;
                            }
                        }
                        options->setUnusedPopupFlag(false);
                        popup = NULL;
                        if (rows == 2)
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-2.png");
                        else if (rows == 3)
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                        else
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-4.png");
                        popup->onShow();
                        g_windowStack->push_back((TrimusWindow *)popup);
                    } else {
                        if (id != WIN_APPS) {
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                TextMenu *options = new TextMenu(false, true, 6, NULL);
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(STR_SEARCH), NULL, searchAction, true));
                                options->setUnusedPopupFlag(false);
                                PopUpWindow *window = NULL;
                                window = new PopUpWindow(1, options, "skin/bg-pop-menu-1.png");
                                window->onShow();
                                g_windowStack->push_back((TrimusWindow *)window);
                            }
                        }
                    }
                }
            } else if (id == WIN_MAIN || id == WIN_GAMES || id == WIN_RETROARCH) {
                TextMenu *options = new TextMenu(false, false, 6, NULL);
                Action *reset = new GeneralAction(-1, resetEmulatorSettings, NULL);
                Action *progress = new PopupProgressAction(STR_REFRESH, reset);
                options->addItem(new TextItem(languageString(STR_REFRESH_ROMS_2), NULL, progress, true));
                options->setUnusedPopupFlag(false);
                PopUpWindow *window = new PopUpWindow(53, options, "skin/bg-pop-menu-1.png");
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            } else if (id == WIN_FAVORITES) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (item->action->result == WIN_APPS)
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    else
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    RemoveFavoriteAction *remove = new RemoveFavoriteAction(item, id);
                    Action *confirm = new PopupDialogAction(STR_REMOVE_FAVORITE, STR_REMOVE_FAVORITE_TEXT, true, remove);
                    printf("item->theaction->mWinType=%d, mitem = %p, menuitem = %p\n", item->action->result, remove, item);
                    options->addItem(new TextItem(languageString(STR_REMOVE), NULL, confirm, true));
                    ClearFavoriteAction *clear = new ClearFavoriteAction(menu, id);
                    Action *clearConfirm = new PopupDialogAction(STR_CLEAR_FAVORITE, STR_CLEAR_FAVORITE_TEXT, true, clear);
                    options->addItem(new TextItem(languageString(STR_CLEAR), NULL, clearConfirm, true));
                    options->setUnusedPopupFlag(false);
                    PopUpWindow *window = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow *)window);
                }
            } else if (id == WIN_RECENT) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (id == WIN_APPS)
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    else
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    ClearRecentAction *clear = new ClearRecentAction(menu, id);
                    Action *confirm = new PopupDialogAction(STR_CLEAR_LIST, STR_CLEAR_RECENT_TEXT, true, clear);
                    options->addItem(new TextItem(languageString(STR_CLEAR), NULL, confirm, true));
                    options->setUnusedPopupFlag(false);
                    PopUpWindow *window = new PopUpWindow(49, options, "skin/bg-pop-menu-2.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow *)window);
                }
            }
        } else if (key == LIST_RIGHT) {
            if (menu && menu->count() > 0 && id != WIN_DEVICE_INFO) {
                if (rightOpensDetail) {
                    TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                    void *data = menu->getItemData(menu->getCur());
                    if (item && item->action) return item->action->progress((int *)item, menu->getCur());
                } else
                    rightOpensDetail = true;
            }
        } else if (key == LIST_LEFT) {
        } else if (key == BUTTON_START)
            showFullPath = !showFullPath;
        return key;
    } else {
        int result = handleGlobalInput(NULL);
        if (result == BUTTON_B)
            return 1;
        else
            return 0;
    }
}
// 0x310ac: draws the window into an off-screen copy (with the status icons).
void MenuWindow::cacheBackground()
{
    if (cachedScreen) {
        SDL_FreeSurface(cachedScreen);
        cachedScreen = NULL;
    }
    cachedScreen = createSurface(g_backBuffer->w, g_backBuffer->h);
    if (cachedScreen) {
        draw(cachedScreen);
        int x = 0;
        int next = 0;
        x = drawBatteryIndicator(cachedScreen, true);
        next = drawWifiIndicator(g_backBuffer, x, true);
        if (next >= 0) x = next;
        if (g_volumeIndicatorVisible) drawKeyStateIndicator(g_backBuffer, x, true);
    }
}
// 0x311f4: the "current/total" count at the bottom right.
int drawPageCount(SDL_Surface *dst, int current, int total)
{
    int y = 420;
    int height = TTF_FontHeight(g_labelFont);
    SDL_Rect area;
    area.x = 320;
    area.y = (60 - height) / 2 + y;
    if (total < 100)
        area.w = 300;
    else
        area.w = 280;
    area.h = 54;
    snprintf(g_countBuffer, 256, "%d", total);
    int width = drawText(dst, g_labelFont, area, g_countBuffer, g_emptyColor, 2, false);
    area.x = area.x - width;
    snprintf(g_countBuffer, 256, "%d/", current);
    drawText(dst, g_labelFont, area, g_countBuffer, g_listColor, 2, false);
    return 0;
}
// 0x31398: title frame, the list, the preview image on the right (with its background) and, for
// lists, the count or the "empty" image and the header (search string, emulator name or full path).
int MenuWindow::draw(SDL_Surface *dst)
{
    char *canonical;
    char *relative;
    int unused = 0;
    if (titleLabelId != 157)
        drawScreenFrame /* 157: the root menu, titled "MIYOO" */ (dst, titleLabelId >= 0 ? languageString(titleLabelId) : titleText.c_str(), id, !isListMenuId(id));
    else
        drawScreenFrame(dst, "MIYOO", id, !isListMenuId(id));
    bool hasImage = previewImage || (pendingImageKey.size() != 0 && pendingImageKey.compare(shownImageKey) == 0);
    if (menu && menu->count() > 0) {
        menu->drawList(dst);
        if (hasPreview && hasImage)
            menu->previewWidth = 250;
        else
            menu->previewWidth = 0;
    }
    if (hasPreview) {
        if (menu && menu->count() > 0 && hasImage) {
            SDL_Rect bounds;
            SDL_Rect dest;
            SDL_Rect source;
            if (g_previewBg) {
                source.x = 0;
                source.y = 0;
                source.w = g_previewBg->w;
                source.h = g_previewBg->h;
                dest.x = 640 - g_previewBg->w;
                dest.y = 60;
                SDL_UpperBlit(g_previewBg, &source, dst, &dest);
                bounds.x = dest.x;
                bounds.y = dest.y;
                bounds.w = g_previewBg->w;
                bounds.h = g_previewBg->h;
            } else {
                bounds.x = 390;
                bounds.y = 60;
                bounds.w = 250;
                bounds.h = 360;
            }
            if (previewImage) {
                dest.x = bounds.x + (250 - previewImage->w) / 2;
                dest.y = bounds.y + (360 - previewImage->h) / 2;
                SDL_UpperBlit(previewImage, NULL, dst, &dest);
            }
        }
    }
    if (isListMenuId(id)) {
        SDL_Rect area;
        if (menu && menu->count() > 0)
            drawPageCount(dst, menu->getCur() + 1, menu->count());
        else if (g_emptyImage) {
            area.x = (640 - g_emptyImage->w) / 2;
            area.y = (480 - g_emptyImage->h) / 2;
            SDL_UpperBlit(g_emptyImage, NULL, dst, &area);
        }
        area.x = area.y = 0;
        area.w = 640;
        area.h = 60;
        if (id != WIN_ROM_LIST && id != WIN_RETROARCH_LIST) {
            if (path.length() != 0) {
                std::string label(languageString(STR_SEARCH));
                label += std::string(":") + path;
                drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
            } else
                drawText(dst, g_titleFont, area, windowTitle(id), g_wifiEmptyColor, 1, true);
        } else {
            EmuInfo *emu = (EmuInfo *)emuEntry;
            if (emu) {
                int now = milliseconds();
                if (showFullPath) {
                    if (iconTF) {
                        SDL_Rect icon;
                        icon.x = 20;
                        icon.y = (area.h - iconTF->h) / 2;
                        SDL_UpperBlit(iconTF, NULL, dst, &icon);
                        area.x = area.x + iconTF->w + 25;
                        area.x += drawText(dst, g_imeFont, area, ":", g_hintColor, 3, true);
                    }
                    canonical = canonicalize_file_name(emu->path.c_str());
                    relative = NULL;
                    if (canonical == NULL) canonical = strdup(emu->path.c_str());
                    if (canonical) {
                        relative = strstr(canonical, "/mnt/SDCARD");
                        relative += 11;
                    }
                    if (relative) drawText(dst, g_imeFont, area, relative, g_hintColor, 3, true);
                    free(canonical);
                } else {
                    if (path.length() != 0) {
                        std::string label = emu->label + std::string(":") + path;
                        drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
                    } else
                        drawText(dst, g_titleFont, area, emu->label.c_str(), g_wifiEmptyColor, 1, true);
                }
            }
        }
    }
    return 0;
}
// 0x31ed4: an icon (optionally scaled to 128x48) and text for the header.
void MenuWindow::setHeaderIcon(SDL_Surface *icon, const char *text, bool scale, bool owns)
{
    if (icon) {
        if (scale) {
            headerIcon = SDL_CreateRGBSurface(0, 128, 48, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
            if (headerIcon) {
                scalePixels(0, 0, icon->w, icon->h, 128, 48, icon->w, 0, (int *)icon->pixels, (int *)headerIcon->pixels);
                if (owns) SDL_FreeSurface(icon);
                ownsHeaderIcon = true;
            } else {
                ownsHeaderIcon = owns;
                headerIcon = icon;
            }
        } else {
            headerIcon = icon;
            ownsHeaderIcon = owns;
        }
    }
    headerText = text;
}
// 0x32040: base of the pop-ups and dialogs: drawn over a dimmed copy of the screen behind them.
FloatWindow::FloatWindow(int title) : TrimusWindow(title, -1)
{
    backdrop = NULL;
}
// 0x32094 (D0 0x320f0)
FloatWindow::~FloatWindow()
{
    if (backdrop) SDL_FreeSurface(backdrop);
}
// 0x32124: draw the dimmed backdrop.
int FloatWindow::draw(SDL_Surface *dst)
{
    SDL_UpperBlit(backdrop, NULL, dst, NULL);
    return 0;
}
// 0x32160: when shown: copy the screen and dim it (except the bottom bar).
void FloatWindow::onShow()
{
    if (backdrop == NULL) {
        backdrop = SDL_CreateRGBSurface(0, 640, 480, 32, 0, 0, 0, 0);
        SDL_UpperBlit(g_backBuffer, NULL, backdrop, NULL);
        SDL_Surface *shade = createSurface(640, 480);
        if (shade) {
            SDL_Rect source = {
                0, 0, 640, 420};
            SDL_Rect dest = {
                0, 0, 0, 0};
            SDL_FillRect(shade, NULL, 0xaa000000);
            SDL_UpperBlit(shade, &source, backdrop, &dest);
            SDL_FreeSurface(shade);
        }
    }
}
