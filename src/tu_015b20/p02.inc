// p02 0x18c54-0x1a44c: actions: App, RemoveFavorite, Search, GlobalSearch, Popup*, General, Clear*, AddFavorite
// Functions in address order.

// 0x18c54: launch the app (writes /tmp/cmd_to_run.sh; the caller then quits MainUI).
int AppAction::execute(void *param)
{
    return launchApp_buildCommand(app, result);
}

// 0x18c94: open the detail window for the selected item.
int AppAction::progress(int *a, int b)
{
    TextItem *item = (TextItem *)a;
    if (detail) {
        GameDetailWindow *w = new GameDetailWindow(0x54, 8, app->label.c_str(), item->label.c_str(),
                                                   app->launch.c_str(), app->iconSelected.c_str(),
                                                   b, result, item->favorite);
        w->onShow();
        g_windowStack->push_back(w);
        return 0;
    } else {
        return -1;
    }
}

// 0x18dd4: number of decimal digits.
int digitCount(int n)
{
    int count = 0;
    do {
        count++;
        n /= 10;
    } while (n != 0);
    return count;
}

// 0x18e3c: animate four dots (and "cur/total") until g_busy clears; 28 ms per frame, 5 frames a step.
void animateBusy(SDL_Surface *bg, SDL_Surface *dot, SDL_Rect bgRect, SDL_Rect dst, bool showCount)
{
    int frame = 0;
    SDL_Rect r;
    SDL_Rect src;
    while (g_busy) {
        if (dot != NULL && bg != NULL) {
            r.y = dst.y;
            if (frame / 5 % 4 == 0)
                SDL_UpperBlit(bg, &bgRect, g_backBuffer, &dst);
            if (frame / 5 % 4 == 1) {
                r.x = dst.x + (dst.w - dot->w * 3) / 4;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (frame / 5 % 4 == 2) {
                r.x = dst.x + (dst.w - dot->w * 3) / 2 + dot->w;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (frame / 5 % 4 == 3) {
                r.x = dst.x + (dst.w - dot->w * 3) * 3 / 4 + dot->w * 2;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (showCount && g_progressTotal > 0) {
                r.x = dst.x - 15;
                r.w = dst.w + 30;
                r.y = dst.y + dot->h + 10;
                r.h = dst.h - dot->h - 10;
                if (frame / 5 % 4 != 0) {
                    src.x = bgRect.x - 15;
                    src.w = bgRect.w + 30;
                    src.y = bgRect.y + dot->h + 10;
                    src.h = bgRect.h - dot->h - 10;
                    SDL_UpperBlit(bg, &src, g_backBuffer, &r);
                }
                snprintf(g_launchCmd, 0x800, "%*d/%d", digitCount(g_progressTotal), g_progressCur, g_progressTotal);
                drawText(g_backBuffer, g_titleFont, r, g_launchCmd, g_selectedTextColor, 1, 0);
                SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
                SDL_Flip(g_screen);
            } else {
                SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
                SDL_Flip(g_screen);
            }
        }
        frame++;
        SDL_Delay(28);
    }
}

// 0x193a8: thread body: draw the frame and status bar, dim the screen, wait up to 280 ms,
// then show the "please wait" popup with the dot animation until g_busy clears.
int busyScreenThread(void *arg)
{
    int x;
    BusyData *sdata = (BusyData *)arg;
    x = 0;
    int r = 0;
    SDL_Rect rect;
    SDL_Rect inner;
    printf("sdata->type is %d, sdata->title is %s\n", sdata->type, sdata->title);
    drawScreenFrame(g_backBuffer, sdata->title, sdata->type, 1);
    x = drawBatteryIndicator(g_backBuffer, 1);
    r = drawSoundCardIndicator(g_backBuffer, 1);
    if (r >= 0)
        x = r;
    r = drawWifiIndicator(g_backBuffer, x, 1);
    if (r >= 0)
        x = r;
    if (g_volumeIndicatorVisible)
        drawKeyStateIndicator(g_backBuffer, x, 1);
    SDL_Surface *dim = createSurface(640, 480);
    if (dim != NULL) {
        SDL_Rect src = {0, 0, 640, 360};
        SDL_Rect dst;
        dst.x = 0;
        dst.y = 60;
        dst.w = 0;
        dst.h = 0;
        SDL_FillRect(dim, NULL, 0xaa000000);
        SDL_UpperBlit(dim, &src, g_backBuffer, &dst);
        SDL_FreeSurface(dim);
    }
    for (x = 0; x < 10 && g_busy; x++)
        SDL_Delay(28);
    if (g_busy) {
        SDL_Surface *bg = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, 0);
        if (bg != NULL) {
            rect.x = (640 - bg->w) / 2;
            rect.y = (480 - bg->h) / 2 + 10;
            rect.w = bg->w;
            rect.h = bg->h;
            SDL_UpperBlit(bg, NULL, g_backBuffer, &rect);
            rect.h = 50;
            drawText(g_backBuffer, g_imeFont, rect, languageString(STR_SCAN_GAMES), g_selectedTextColor, 1, 0);
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_Flip(g_screen);
            SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, 0);
            if (dot != NULL) {
                int h = dot->h;
                h += 40;
                rect.x = 255;
                rect.y = (480 - h) / 2 + 5;
                rect.w = 140;
                rect.h = h;
                inner.x = (bg->w - 140) / 2 + 5;
                inner.w = 140;
                inner.y = (bg->h - h) / 2 + 5;
                inner.h = h;
                animateBusy(bg, dot, inner, rect, true);
                SDL_FreeSurface(dot);
            }
            SDL_FreeSurface(bg);
        }
    }
    return 0;
}

// 0x198a8: thread body: plain background, a centred text line and the dot animation below it.
int loadingScreenThread(void *arg)
{
    drawBackground(g_backBuffer);
    int ret = 0;
    void *font = g_imeFont;
    int fh = TTF_FontHeight(font);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    int w = drawText(g_backBuffer, font, rect, languageString(STR_LOADING_CAPS), g_wifiEmptyColor, 1, 0);
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, 0);
    rect.x = (640 - w) / 2 + 5;
    rect.y = (480 - fh) / 2 + fh + 6;
    rect.w = w;
    if (dot != NULL)
        rect.h = dot->h;
    animateBusy(g_background, dot, rect, rect, false);
    if (dot != NULL)
        SDL_FreeSurface(dot);
    return 0;
}

// 0x19acc: mark the item removed from the favourites list.
int RemoveFavoriteAction::execute(void *param)
{
    item->removed = true;
    return 1;
}

// 0x19b04: open the result list for a search string (param), in a menu with the given id.
int SearchAction::execute(void *param)
{
    const char *key = (const char *)param;
    MenuWindow *w = new MenuWindow(0x99, id, emu);
    w->setPath(key);
    w->onShow();
    g_windowStack->push_back(w);
    return -1;
}

// 0x19c18: the same, over every ROM list (menu 0x18).
int GlobalSearchAction::execute(void *param)
{
    const char *key = (const char *)param;
    MenuWindow *w = new MenuWindow(0x99, 0x18, (void *)0);
    w->setPath(key);
    w->onShow();
    g_windowStack->push_back(w);
    return -1;
}

// 0x19d24: ask for a search string; the keyboard runs a SearchAction with it.
int PopupSearchIME::execute(void *param)
{
    ImeWindow *w = new ImeWindow(0x99, new SearchAction(arg, id, emu), 0x99);
    pushWindow(w);
    return 4;
}

// 0x19dc0
int PopupDialogAction::execute(void *param)
{
    DialogWindow *w = new DialogWindow(title, text, flag, action);
    pushWindow(w);
    return 4;
}

// 0x19e44
int PopupProgressAction::execute(void *param)
{
    ProgressDialog *w = new ProgressDialog(title, action);
    w->onShow();
    pushWindow(w);
    return 4;
}

// 0x19ecc: call a plain function.
int GeneralAction::execute(void *param)
{
    if (fn != 0) {
        if (verbose)
            puts("GeneralAction dowork");
        return fn(arg);
    } else {
        if (verbose)
            printf("GeneralAction dowork dosth %p\n", fn);
        return -1;
    }
}

// 0x19f64
int ClearFavoriteAction::execute(void *param)
{
    if (!unlink("/mnt/SDCARD/Roms/favourite.json")) {
        menu->vfunc13();
        sync();
    }
    return 1;
}

// 0x19fd8
int ClearRecentAction::execute(void *param)
{
    clearRecent(&g_recentGames);
    menu->vfunc13();
    return 1;
}

// 0x1a02c: append the item to favourite.json; result is the window id it was opened from.
int AddFavoriteAction::execute(void *param)
{
    const char *label = 0;
    const char *launch = 0;
    const char *rompath = 0;
    if (result != 3 && result != 5 && result != 0x11 && result != 0x18) {
        printf("add to favorite error: unkown win %d\n", result);
        return 1;
    }
    FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "a+");
    if (f != NULL) {
        printf("open %s %p\n", "/mnt/SDCARD/Roms/favourite.json", f);
        cJSON *obj = cJSON_CreateObject();
        if (obj != NULL) {
            if (result != 3) {
                EmuAction *emu = (EmuAction *)item->action;
                if (emu != NULL) {
                    if (emu->emu != NULL) {
                        launch = emu->emu->launch.c_str();
                        label = item->label.c_str();
                    } else {
                        printf("add to favorite error: emuitem %p\n", emu->emu);
                    }
                    rompath = emu->rompath.c_str();
                } else {
                    printf("add to favorite error: emu param %p\n", emu);
                }
                printf("add to favorite label %s, launch %s, type %d, rompath %s\n", label, launch, result, rompath);
                if (launch != NULL && !isFavourite(0, f, rompath)) {
                    if (cJSON_AddStringToObject(obj, "label", label) && cJSON_AddStringToObject(obj, "launch", launch) &&
                        cJSON_AddNumberToObject(obj, "type", result) && cJSON_AddStringToObject(obj, "rompath", rompath)) {
                        char *s = cJSON_PrintUnformatted(obj);
                        fprintf(f, "%s\n", s);
                        free(s);
                        item->favorite = true;
                    }
                }
            } else {
                AppAction *app = (AppAction *)item->action;
                if (app != NULL) {
                    label = app->app->label.c_str();
                    launch = app->app->launch.c_str();
                } else {
                    printf("add to favorite error: app param %p\n", app);
                }
                if (cJSON_AddStringToObject(obj, "label", label) && cJSON_AddStringToObject(obj, "launch", launch) &&
                    cJSON_AddNumberToObject(obj, "type", result)) {
                    char *s = cJSON_PrintUnformatted(obj);
                    fprintf(f, "%s\n", s);
                    free(s);
                    item->favorite = true;
                }
            }
            cJSON_Delete(obj);
        }
        fclose(f);
        sync();
    }
    return 1;
}
