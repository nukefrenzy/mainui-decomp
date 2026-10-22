// p09 0x2e424-0x3225c: MenuWindow and FloatWindow.
// 0x2e424
MenuWindow::MenuWindow(int titleLabelId, int id, void *entry)
    : TrimusWindow(titleLabelId, id), helper60(2), helper80(freeSurfaceCallback) {
    this->menu = NULL;
    flag1c = false;
    flag1d = false;
    field38 = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    flag1c = false;
    flag1d = false;
    field54 = true;
    surface58 = 0;
    emuEntry = entry;
    field94 = false;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    surfaceFC = 0;
    ownsSurfaceFC = false;
}
// 0x2e6d4
MenuWindow::MenuWindow(int titleLabelId, int id, ListMenu *menu)
    : TrimusWindow(titleLabelId, id), helper60(2), helper80(freeSurfaceCallback) {
    this->menu = menu;
    flag1c = false;
    flag1d = false;
    field38 = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    flag1c = false;
    flag1d = false;
    field54 = true;
    surface58 = 0;
    emuEntry = 0;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    surfaceFC = 0;
    ownsSurfaceFC = false;
}
// 0x2e978
MenuWindow::MenuWindow(const char *folder, int id, void *entry)
    : TrimusWindow(-1, id), helper60(2), helper80(freeSurfaceCallback) {
    this->menu = NULL;
    flag1c = false;
    flag1d = false;
    field38 = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    flag1c = false;
    flag1d = false;
    field54 = true;
    surface58 = 0;
    emuEntry = entry;
    field94 = false;
    str98 = folder;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    surfaceFC = 0;
    ownsSurfaceFC = false;
}
// 0x2ec3c (D0 0x2ed78)
MenuWindow::~MenuWindow() {
    if (surface58 != 0) {
        SDL_FreeSurface(surface58);
    }
    if (iconTF != 0) {
        SDL_FreeSurface(iconTF);
    }
    if (ownsSurfaceFC && surfaceFC != 0) {
        SDL_FreeSurface(surfaceFC);
    }
}

int findEmuByLabelPrefix(std::vector<EmuInfo> *, const char *);
// 0x121074
extern SDL_Surface *g_romPreviewBackground;
// 0x1821fc
extern SDL_Surface *g_emptyListImage;
// 0x1821b4
extern SDL_Color g_countColor;
// 0x1822ac
extern SDL_Color g_pathColor;
// 0x1822a8
extern char g_countBuffer[0x100];
// 0x180754
// 0x2edac
void MenuWindow::saveState(cJSON *obj) {
    cJSON *position = cJSON_CreateNumber(menu->getCur());
    cJSON_AddItemToObject(obj, "currpos", position);
    cJSON *start = cJSON_CreateNumber(menu->getPageStart());
    cJSON_AddItemToObject(obj, "pagestart", start);
    cJSON *end = cJSON_CreateNumber(menu->getPageEnd());
    cJSON_AddItemToObject(obj, "pageend", end);
    if (emuEntry) {
        EmuInfo *emu = (EmuInfo*)emuEntry;
        cJSON *name = cJSON_CreateString(emu->label.c_str());
        if (id == 5) cJSON_AddItemToObject(obj, "emuname", name);
        else cJSON_AddItemToObject(obj, "raname", name);
    }
    if (strB4.length() != 0) cJSON_AddStringToObject(obj, "ppath", strB4.c_str());
    std::string search = menu->getSearch();
    if (search.length() != 0) cJSON_AddStringToObject(obj, "searchstr", search.c_str());
}
// 0x2efcc
int MenuWindow::restoreState(cJSON *obj) {
    cJSONItem *position = (cJSONItem*)cJSON_GetObjectItem(obj, "currpos");
    cJSONItem *start = (cJSONItem*)cJSON_GetObjectItem(obj, "pagestart");
    cJSONItem *end = (cJSONItem*)cJSON_GetObjectItem(obj, "pageend");
    if (position && position->type == 8 && start && start->type == 8 && end && end->type == 8) {
        savedCurPos = position->valueint;
        savedPageStart = start->valueint;
        savedPageEnd = end->valueint;
    }
    cJSON *name = cJSON_GetObjectItem(obj, "emuname");
    if (name) {
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_romSourcesA, label);
        if (index >= 0) {
            emuEntry = &g_romSourcesA[index];
            str100 = g_romSourcesA[index].label;
        } else emuEntry = NULL;
    } else {
        name = cJSON_GetObjectItem(obj, "raname");
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_romSourcesB, label);
        if (index >= 0) emuEntry = &g_romSourcesB[index];
        else emuEntry = NULL;
    }
    cJSONItem *folder = (cJSONItem*)cJSON_GetObjectItem(obj, "ppath");
    if (folder && folder->type == 16) strB4 = folder->valuestring;
    cJSONItem *search = (cJSONItem*)cJSON_GetObjectItem(obj, "searchstr");
    if (search && search->type == 16) path = search->valuestring;
    return 0;
}
// 0x2f290
void MenuWindow::onShow() {
    if (menu == NULL) {
        if (id == 24) {
            menu = new MultiDBCachedTextMenu(path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else if (id != 5 && id != 17) {
            menu = createMenu(id, path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else {
            if (emuEntry) {
                EmuInfo *emu = (EmuInfo*)emuEntry;
                RomListPosition position;
                menu = createRomListMenu(emu, id, &strB4, &path);
                std::string folder = emu->path;
                if (strB4.length() != 0) folder = emu->path + "/" + strB4;
                if (getRomListPosition(&g_romListPositions, folder, &position) >= 0) menu->restorePosition(position.current, position.start, position.selected);
                else menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
            }
        }
        flag1d = isListMenuId(id) && id != 4 && id != 3;
        if (menu) isListMenuId(id);
    }
}
// 0x2f6c4
void MenuWindow::updateThumbnail() {
    if (flag1d) {
        TextItem *item = (TextItem*)menu->getItem(menu->getCur());
        if (item) {
            std::string unused;
            std::string image;
            if (item->action->result == 3) {
                AppAction *action = (AppAction*)item->action;
                AppInfo *app = action->app;
                image = app->iconSelected;
            } else {
                const char *explicitPath = (const char*)menu->vfunc6(menu->getCur());
                if (explicitPath) {
                    image = explicitPath;
                    struct stat st;
                    if (stat(image.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) image = "";
                } else {
                    EmuAction *action = (EmuAction*)item->action;
                    EmuInfo *emu = action->emu;
                    std::string game = action->rompath;
                    image = makeThumbPath(&game, emu->imagePath.c_str());
                }
            }
            if (image.size() != 0 && (image.compare(str20) != 0 || field38 == NULL)) {
                str20 = image;
                field38 = (SDL_Surface*)helper80.get(image);
                if (field38 == NULL) {
                    LoadSurfaceWorker *worker = new LoadSurfaceWorker(image, &helper80);
                    helper60.addWorker(worker);
                    str3c = image;
                } else str3c = "";
            } else {
                if (image.size() == 0) {
                    str20 = "";
                    field38 = NULL;
                }
            }
        }
    }
}
// 0x2fb14
void MenuWindow::update() {
    if (menu) {
        bool changed = refreshMenu_1c0ac(menu, id);
        if (flag1c != true) flag1c = changed;
        updateThumbnail();
        menu->vfunc11();
    }
    if (themePath.compare(g_themePath) != 0) {
        if (iconTF) SDL_FreeSurface(iconTF);
        iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}
// 0x2fc74
void MenuWindow::onHide() {
    if (menu) {
        if (id == 2 && flag1c) {
            TextMenu *text = (TextMenu*)menu;
            power_label_1a4bc("/mnt/SDCARD/Roms/favourite.json", text->getItems());
        }
        if (emuEntry && (id == 5 || id == 17)) {
            EmuInfo *emu = (EmuInfo*)emuEntry;
            int position = menu->getCur();
            int start = menu->getPageStart();
            int end = menu->getPageEnd();
            std::string search = menu->getSearch();
            if (search.length() == 0) {
                if (strB4.length() != 0) setRomListPosition(&g_romListPositions, emu->path + "/" + strB4, position, start, end);
                else setRomListPosition(&g_romListPositions, std::string(emu->path), position, start, end);
            }
        }
        delete menu;
    }
}
// 0x2ff04
int MenuWindow::handleInput() {
    int rows;
    PopUpWindow *popup;
    int key;
    if (menu) {
        key = menu->vfunc3();
        if (key == 11) {
            if (id == 5 || id == 17 || id == 3 || id == 24) {
                TextItem *item = (TextItem*)menu->getItem(menu->getCur());
                if (item) {
                    unsigned char canOperate = item->action->finished();
                    if (canOperate) {
                        rows = 2;
                        TextMenu *options = new TextMenu(false, false, 6, NULL);
                        if (id == 3) options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                        else options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                        options->addItem(new TextItem(languageString(55), NULL, new AddFavoriteAction(item, id), true));
                        if (id == 5 || id == 17) {
                            DeleteRomAction *remove = new DeleteRomAction(menu, id);
                            remove->field0c = menu->getCur();
                            Action *confirm = new PopupDialogAction(116, 117, true, remove);
                            options->addItem(new TextItem(languageString(128), NULL, confirm, true));
                            rows++;
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(153), NULL, searchAction, true));
                                rows++;
                            }
                        }
                        options->setFlag5c(false);
                        popup = NULL;
                        if (rows == 2) popup = new PopUpWindow(1, options, "skin/bg-pop-menu-2.png");
                        else if (rows == 3) popup = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                        else popup = new PopUpWindow(1, options, "skin/bg-pop-menu-4.png");
                        popup->onShow();
                        g_windowStack->push_back((TrimusWindow*)popup);
                    } else {
                        if (id != 3) {
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                TextMenu *options = new TextMenu(false, true, 6, NULL);
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(153), NULL, searchAction, true));
                                options->setFlag5c(false);
                                PopUpWindow *window = NULL;
                                window = new PopUpWindow(1, options, "skin/bg-pop-menu-1.png");
                                window->onShow();
                                g_windowStack->push_back((TrimusWindow*)window);
                            }
                        }
                    }
                }
            } else if (id == 0 || id == 1 || id == 16) {
                TextMenu *options = new TextMenu(false, false, 6, NULL);
                Action *reset = new GeneralAction(-1, resetEmulatorSettings, NULL);
                Action *progress = new PopupProgressAction(53, reset);
                options->addItem(new TextItem(languageString(54), NULL, progress, true));
                options->setFlag5c(false);
                PopUpWindow *window = new PopUpWindow(53, options, "skin/bg-pop-menu-1.png");
                window->onShow();
                g_windowStack->push_back((TrimusWindow*)window);
            } else if (id == 2) {
                TextItem *item = (TextItem*)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (item->action->result == 3) options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                    else options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                    RemoveFavoriteAction *remove = new RemoveFavoriteAction(item, id);
                    Action *confirm = new PopupDialogAction(81, 82, true, remove);
                    printf("item->theaction->mWinType=%d, mitem = %p, menuitem = %p\n", item->action->result, remove, item);
                    options->addItem(new TextItem(languageString(52), NULL, confirm, true));
                    ClearFavoriteAction *clear = new ClearFavoriteAction(menu, id);
                    Action *clearConfirm = new PopupDialogAction(79, 80, true, clear);
                    options->addItem(new TextItem(languageString(50), NULL, clearConfirm, true));
                    options->setFlag5c(false);
                    PopUpWindow *window = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow*)window);
                }
            } else if (id == 10) {
                TextItem *item = (TextItem*)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (id == 3) options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                    else options->addItem(new TextItem(languageString(51), NULL, item->action, false));
                    ClearRecentAction *clear = new ClearRecentAction(menu, id);
                    Action *confirm = new PopupDialogAction(77, 78, true, clear);
                    options->addItem(new TextItem(languageString(50), NULL, confirm, true));
                    options->setFlag5c(false);
                    PopUpWindow *window = new PopUpWindow(49, options, "skin/bg-pop-menu-2.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow*)window);
                }
            }
        } else if (key == 6) {
            if (menu && menu->count() > 0 && id != 9) {
                if (field54) {
                    TextItem *item = (TextItem*)menu->getItem(menu->getCur());
                    void *data = menu->vfunc6(menu->getCur());
                    if (item && item->action) return item->action->progress((int*)item, menu->getCur());
                } else field54 = true;
            }
        } else if (key == 5) {
        } else if (key == 10) field94 = !field94;
        return key;
    } else {
        int result = handleGlobalInput(NULL);
        if (result == 2) return 1;
        else return 0;
    }
}
// 0x310ac
void MenuWindow::cacheBackground() {
    if (surface58) {
        SDL_FreeSurface(surface58);
        surface58 = NULL;
    }
    surface58 = createSurface(g_backBuffer->w, g_backBuffer->h);
    if (surface58) {
        draw(surface58);
        int x = 0;
        int next = 0;
        x = drawBatteryIndicator(surface58, true);
        next = drawWifiIndicator(g_backBuffer, x, true);
        if (next >= 0) x = next;
        if (g_statusIndicatorFlag) drawKeyStateIndicator(g_backBuffer, x, true);
    }
}
// 0x311f4
int drawPageDots_311f4(SDL_Surface *dst, int current, int total) {
    int y = 420;
    int height = TTF_FontHeight(g_labelFont);
    SDL_Rect area;
    area.x = 320;
    area.y = (60-height)/2 + y;
    if (total < 100) area.w = 300;
    else area.w = 280;
    area.h = 54;
    snprintf(g_countBuffer, 256, "%d", total);
    int width = drawText(dst, g_labelFont, area, g_countBuffer, g_emptyColor, 2, false);
    area.x = area.x-width;
    snprintf(g_countBuffer, 256, "%d/", current);
    drawText(dst, g_labelFont, area, g_countBuffer, g_countColor, 2, false);
    return 0;
}
// 0x31398
int MenuWindow::draw(SDL_Surface *dst) {
    char *canonical;
    char *relative;
    int unused = 0;
    if (titleLabelId != 157) drawScreenFrame(dst, titleLabelId >= 0 ? languageString(titleLabelId) : str98.c_str(), id, !isListMenuId(id));
    else drawScreenFrame(dst, "MIYOO", id, !isListMenuId(id));
    bool hasImage = field38 || (str3c.size() != 0 && str3c.compare(str20) == 0);
    if (menu && menu->count() > 0) {
        menu->vfunc7(dst);
        if (flag1d && hasImage) menu->field04 = 250;
        else menu->field04 = 0;
    }
    if (flag1d) {
        if (menu && menu->count() > 0 && hasImage) {
            SDL_Rect bounds;
            SDL_Rect dest;
            SDL_Rect source;
            if (g_romPreviewBackground) {
                source.x = 0;
                source.y = 0;
                source.w = g_romPreviewBackground->w;
                source.h = g_romPreviewBackground->h;
                dest.x = 640-g_romPreviewBackground->w;
                dest.y = 60;
                SDL_UpperBlit(g_romPreviewBackground, &source, dst, &dest);
                bounds.x = dest.x;
                bounds.y = dest.y;
                bounds.w = g_romPreviewBackground->w;
                bounds.h = g_romPreviewBackground->h;
            } else {
                bounds.x = 390;
                bounds.y = 60;
                bounds.w = 250;
                bounds.h = 360;
            }
            if (field38) {
                dest.x = bounds.x+(250-field38->w)/2;
                dest.y = bounds.y+(360-field38->h)/2;
                SDL_UpperBlit(field38, NULL, dst, &dest);
            }
        }
    }
    if (isListMenuId(id)) {
        SDL_Rect area;
        if (menu && menu->count() > 0) drawPageDots_311f4(dst, menu->getCur()+1, menu->count());
        else if (g_emptyListImage) {
            area.x = (640-g_emptyListImage->w)/2;
            area.y = (480-g_emptyListImage->h)/2;
            SDL_UpperBlit(g_emptyListImage, NULL, dst, &area);
        }
        area.x = area.y = 0;
        area.w = 640;
        area.h = 60;
        if (id != 5 && id != 17) {
            if (path.length() != 0) {
                std::string label(languageString(153));
                label += std::string(":")+path;
                drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
            } else drawText(dst, g_titleFont, area, files_374bc(id), g_wifiEmptyColor, 1, true);
        } else {
            EmuInfo *emu = (EmuInfo*)emuEntry;
            if (emu) {
                int now = milliseconds();
                if (field94) {
                    if (iconTF) {
                        SDL_Rect icon;
                        icon.x = 20;
                        icon.y = (area.h-iconTF->h)/2;
                        SDL_UpperBlit(iconTF, NULL, dst, &icon);
                        area.x = area.x+iconTF->w+25;
                        area.x += drawText(dst, g_imeFont, area, ":", g_pathColor, 3, true);
                    }
                    canonical = canonicalize_file_name(emu->path.c_str());
                    relative = NULL;
                    if (canonical == NULL) canonical = strdup(emu->path.c_str());
                    if (canonical) {
                        relative = strstr(canonical, "/mnt/SDCARD");
                        relative += 11;
                    }
                    if (relative) drawText(dst, g_imeFont, area, relative, g_pathColor, 3, true);
                    free(canonical);
                } else {
                    if (path.length() != 0) {
                        std::string label = emu->label+std::string(":")+path;
                        drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
                    } else drawText(dst, g_titleFont, area, emu->label.c_str(), g_wifiEmptyColor, 1, true);
                }
            }
        }
    }
    return 0;
}
// 0x31ed4
void MenuWindow::setHeaderIcon(SDL_Surface *icon, const char *text, bool scale, bool owns) {
    if (icon) {
        if (scale) {
            surfaceFC = SDL_CreateRGBSurface(0, 128, 48, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
            if (surfaceFC) {
                scalePixels(0, 0, icon->w, icon->h, 128, 48, icon->w, 0, (int*)icon->pixels, (int*)surfaceFC->pixels);
                if (owns) SDL_FreeSurface(icon);
                ownsSurfaceFC = true;
            } else {
                ownsSurfaceFC = owns;
                surfaceFC = icon;
            }
        } else {
            surfaceFC = icon;
            ownsSurfaceFC = owns;
        }
    }
    str100 = text;
}
// 0x32040
FloatWindow::FloatWindow(int title) : TrimusWindow(title, -1) {
    surface0c = NULL;
}
// 0x32094 (D0 0x320f0)
FloatWindow::~FloatWindow() {
    if (surface0c) SDL_FreeSurface(surface0c);
}
// 0x32124
int FloatWindow::draw(SDL_Surface *dst) {
    SDL_UpperBlit(surface0c, NULL, dst, NULL);
    return 0;
}
// 0x32160
void FloatWindow::onShow() {
    if (surface0c == NULL) {
        surface0c = SDL_CreateRGBSurface(0, 640, 480, 32, 0, 0, 0, 0);
        SDL_UpperBlit(g_backBuffer, NULL, surface0c, NULL);
        SDL_Surface *shade = createSurface(640, 480);
        if (shade) {
            SDL_Rect source = {
                0, 0, 640, 420
            };
            SDL_Rect dest = {
                0, 0, 0, 0
            };
            SDL_FillRect(shade, NULL, 0xaa000000);
            SDL_UpperBlit(shade, &source, surface0c, &dest);
            SDL_FreeSurface(shade);
        }
    }
}
