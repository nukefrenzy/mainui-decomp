// p10 0x3225c-0x36838: popup, dialog and game-detail windows.
// SDL timer callbacks only allocate the 16-byte user-event prefix.
struct WindowUserEvent {
    unsigned char type;
    int code;
    void *data1, *data2;
};
static_assert(sizeof(WindowUserEvent) == 16, "SDL timer event prefix");
extern "C" unsigned SDL_MapRGB(SDL_PixelFormat *, unsigned char, unsigned char, unsigned char);
// 0x3225c
PopUpWindow::PopUpWindow(int titleLabelId, TextMenu *list, const char *file) : FloatWindow(titleLabelId) {
    background = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    menu = list;
    printf("load menu bg %s : %p\n", file, background);
    if (menu) {
        SDL_Rect rect;
        if (background) {
            rect.x = rect.y = 0;
            rect.w = background->w;
            rect.h = background->h-8;
            if (menu->count() > 1) menu->setRowHeight(rect.h/menu->count());
            else menu->setRowHeight(rect.h);
        } else {
            rect.x = rect.y = 0;
            rect.w = 120;
            rect.h = 75;
        }
        menu->setRect(&rect);
    }
}
// 0x3247c (D0 0x32534)
PopUpWindow::~PopUpWindow() {
    if (menu) {
        delete menu;
        menu = NULL;
    }
    if (background) SDL_FreeSurface(background);
    puts("~PopupWindow end");
}
// 0x32568
void PopUpWindow::onHide() {
}
// 0x32588
int PopUpWindow::draw(SDL_Surface *dst) {
    FloatWindow::draw(dst);
    if (g_background) {
        SDL_Rect source;
        source.x = 0;
        source.y = 420;
        source.w = 640;
        source.h = 60;
        SDL_Rect dest;
        dest.x = 0;
        dest.y = 420;
        SDL_UpperBlit(g_background, &source, dst, &dest);
        if (g_unusedIndicator) {
            source.y = g_unusedIndicator->h-60;
            SDL_UpperBlit(g_unusedIndicator, &source, dst, &dest);
        }
        drawFooter(dst, languageString(46), languageString(45));
    }
    if (menu) {
        if (background) {
            SDL_Rect dest;
            dest.x = 0;
            dest.y = 0;
            SDL_UpperBlit(background, NULL, dst, &dest);
        } else {
            SDL_Rect source = {
                0, 0, 0, 0
            };
            source.w = menu->getRect()->w;
            source.h = menu->getRect()->h;
            SDL_UpperBlit(g_background, &source, dst, menu->getRect());
        }
        menu->vfunc7(dst);
    }
    return 0;
}
// 0x32784
int PopUpWindow::handleInput() {
    if (menu) return menu->vfunc3();
    return 1;
}
// 0x327e4
NotifyWindow::NotifyWindow(const char *message, const char *button, const char *file, void (*fn)()) : FloatWindow(-1) {
    text = message;
    field10 = NULL;
    surface2c = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    surface30 = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    field34 = button;
    callback = fn;
}
// 0x32920 (D0 0x329cc)
NotifyWindow::~NotifyWindow() {
    if (field10) SDL_RemoveTimer(field10);
    if (surface2c) SDL_FreeSurface(surface2c);
    if (surface30) SDL_FreeSurface(surface30);
}
// 0x32a00
static unsigned notifyTimeout(unsigned interval, void *arg) {
    void (*fn)() = (void(*)())arg;
    WindowUserEvent event;
    event.type = 24;
    event.code = 0;
    timeval now;
    gettimeofday(&now, NULL);
    printf("timeout sec = %d\n", now.tv_sec);
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    if (fn) fn();
    return interval;
}
// 0x32aa0
void NotifyWindow::onShow() {
    FloatWindow::onShow();
    timeval now;
    gettimeofday(&now, NULL);
    printf("init sec = %d\n", now.tv_sec);
    field10 = SDL_AddTimer(5000, notifyTimeout, (void*)callback);
    if (surface2c == NULL && g_background) {
        surface2c = SDL_CreateRGBSurface(0, 240, 90, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            200, 195, 240, 90
        };
        SDL_UpperBlit(g_background, &r, surface2c, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(surface2c, &r, SDL_MapRGB(surface2c->format, 255, 255, 255));
        r.x = 0;
        r.y = 89;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(surface2c, &r, SDL_MapRGB(surface2c->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(surface2c, &r, SDL_MapRGB(surface2c->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(surface2c, &r, SDL_MapRGB(surface2c->format, 255, 255, 255));
    }
}
// 0x32d0c
int NotifyWindow::handleInput() {
    SDL_Event event;
    int key = handleGlobalInput(&event);
    if (event.type == 3) return 1;
    else if (event.type == 24 && event.user.code == 0) return 1;
    return -1;
}
// 0x32d74
int NotifyWindow::draw(SDL_Surface *dst) {
    FloatWindow::draw(dst);
    SDL_Rect r;
    if (surface2c) {
        r.x = (640-surface2c->w)/2;
        r.y = (480-surface2c->h)/2;
        r.w = surface2c->w;
        r.h = surface2c->h;
        SDL_UpperBlit(surface2c, NULL, dst, &r);
    } else {
        r.x = 200;
        r.y = 195;
        r.w = 240;
        r.h = 90;
    }
    if (surface30) {
        r.h -= surface30->h/2;
        r.y -= surface30->h/4;
    }
    drawTextLines(dst, g_titleFont, &r, text.c_str(), "\n", &g_progressColor, 1, false);
    if (surface30 && surface2c) {
        r.x = (640-surface30->w)/2;
        r.y = 468-(480-surface2c->h)/2-surface30->h;
        SDL_UpperBlit(surface30, NULL, dst, &r);
        r.w = surface30->w;
        r.h = surface30->h;
        drawText(dst, g_titleFont, r, field34, g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33060
CountDownWindow::CountDownWindow(int label, void (*timeout)(), void (*cancel)()) : NotifyWindow("", languageString(45), "skin/bg-button-f.png", timeout) {
    title = label;
    seconds = 10;
    field40 = NULL;
    onCancel = cancel;
}
// 0x330fc (D0 0x33158)
CountDownWindow::~CountDownWindow() {
    if (field40) SDL_RemoveTimer(field40);
}
// 0x3318c
static unsigned countdownTick(unsigned interval, void *arg) {
    WindowUserEvent event;
    event.type = 24;
    event.code = 22;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
// 0x331ec
void CountDownWindow::onShow() {
    NotifyWindow::onShow();
    snprintf(raw4c, 128, languageString(title), seconds);
    text = raw4c;
    if (field10) SDL_RemoveTimer(field10);
    field10 = SDL_AddTimer(seconds*1000, notifyTimeout, (void*)callback);
    field40 = SDL_AddTimer(1000, countdownTick, NULL);
}
// 0x332d8
int CountDownWindow::handleInput() {
    int key = handleGlobalInput(NULL);
    if (key != 18 && key != 19 && key != 22) {
        if (onCancel) onCancel();
        return 1;
    } else {
        if (key == 22) {
            seconds--;
            snprintf(raw4c, 128, languageString(title), seconds);
            text = raw4c;
            SDL_RemoveTimer(field40);
            field40 = SDL_AddTimer(1000, countdownTick, NULL);
        }
        return -1;
    }
}
// 0x333ec
DialogWindow::DialogWindow(int heading, int message, bool owns, Action *act) : FloatWindow(-1) {
    title = heading;
    text = message;
    selected = 1;
    surface1c = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    if (surface1c) {
        flag31 = false;
        surface20 = NULL;
    } else {
        surface20 = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        flag31 = true;
    }
    iconA = g_iconA;
    iconB = g_iconB;
    action = act;
    flag = owns;
}
// 0x33570 (D0 0x33630)
DialogWindow::~DialogWindow() {
    if (surface1c) SDL_FreeSurface(surface1c);
    if (surface20) SDL_FreeSurface(surface20);
    if (action) delete action;
}
// 0x33664
void DialogWindow::onShow() {
    FloatWindow::onShow();
    if (g_background && flag31) {
        surface1c = SDL_CreateRGBSurface(0, 240, 120, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            0, 0, 240, 120
        };
        SDL_UpperBlit(g_background, &r, surface1c, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
        r.x = 0;
        r.y = 119;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
        r.x = 0;
        r.y = 90;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
        r.x = 120;
        r.y = 90;
        r.w = 1;
        r.h = 30;
        SDL_FillRect(surface1c, &r, SDL_MapRGB(surface1c->format, 255, 255, 255));
    }
}
// 0x33934
int DialogWindow::drawFallback(SDL_Surface *dst) {
    SDL_Rect r;
    r.x = 200;
    r.y = 180;
    r.w = 240;
    r.h = 120;
    if (surface1c) SDL_UpperBlit(surface1c, NULL, dst, &r);
    r.h = 90;
    drawText(dst, g_titleFont, r, languageString(text), g_progressColor, 1, false);
    for (int i = 0; i < 2; i++) {
        r.x = 200+i*120;
        r.y = 270;
        r.w = 120;
        r.h = 30;
        if (i == selected) {
            if (surface20) {
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = r.w;
                source.h = r.h;
                SDL_UpperBlit(surface20, &source, dst, &r);
            }
            drawText(dst, g_titleFont, r, (i == 0 ? languageString(45) : languageString(46)), g_imeTextColor, 1, false);
        } else drawText(dst, g_titleFont, r, (i == 0 ? languageString(45) : languageString(46)), g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33bac
int DialogWindow::draw(SDL_Surface *dst) {
    FloatWindow::draw(dst);
    if (flag31 != true && surface1c) {
        SDL_Rect r;
        r.x = (640-surface1c->w)/2;
        r.y = (480-surface1c->h)/2;
        r.w = surface1c->w;
        r.h = surface1c->h;
        SDL_UpperBlit(surface1c, NULL, dst, &r);
        r.h = 50;
        drawText(dst, g_titleFont, r, languageString(title), g_emptyColor, 1, false);
        r.y = (480-surface1c->h)/2+50;
        r.h = 100;
        drawTextLines(dst, g_titleFont, &r, languageString(text), "\n", &g_progressColor, 1, false);
        r.x = (640-surface1c->w)/2+230;
        r.y = (480-surface1c->h)/2+160;
        r.w = surface1c->w;
        r.h = iconA->h;
        SDL_UpperBlit(iconA, NULL, dst, &r);
        r.x = r.x+iconA->w+10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(46), g_shadowColor, 3, false);
        r.x = (640-surface1c->w)/2+230;
        r.y = (480-surface1c->h)/2+iconA->h+170;
        SDL_UpperBlit(iconB, NULL, dst, &r);
        r.x = r.x+iconB->w+10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(45), g_shadowColor, 3, false);
    } else return drawFallback(dst);
    return 0;
}
// 0x3400c
int DialogWindow::handleInput() {
    int key = handleGlobalInput(NULL);
    switch (key) {
    case 1: case 4: if (selected == 1 && action) return action->execute(NULL);
        return 1;
    case 2: case 3: return 1;
    case 7: if (selected > 0) selected--;
        break;
    case 8: if (selected < 1) selected++;
        break;
    default: break;
    }
    return -1;
}
// 0x34134
static int progressThread(void *arg) {
    ProgressDialog *dialog = (ProgressDialog*)arg;
    g_busy = 1;
    dialog->runAction();
    g_busy = 0;
    return 0;
}
// 0x34184
ProgressDialog::ProgressDialog(int heading, Action *act) : FloatWindow(-1) {
    title = heading;
    action = act;
    surface30 = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    surface2c = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-button-f.png", g_backBuffer, false);
    field38 = NULL;
}
// 0x342ac (D0 0x3437c)
ProgressDialog::~ProgressDialog() {
    if (surface30) SDL_FreeSurface(surface30);
    if (surface2c) SDL_FreeSurface(surface2c);
    if (action) delete action;
}
// 0x343b0
void ProgressDialog::onShow() {
    FloatWindow::onShow();
    field38 = SDL_CreateThread(progressThread, this);
}
// 0x343f4
void ProgressDialog::runAction() {
    action->execute(NULL);
}
// 0x34438
void ProgressDialog::onHide() {
    SDL_WaitThread(field38, NULL);
}
// 0x34468
int ProgressDialog::draw(SDL_Surface *dst) {
    FloatWindow::draw(g_backBuffer);
    int frame = 0;
    SDL_Rect r;
    if (surface30) {
        r.x = (640-surface30->w)/2;
        r.y = (480-surface30->h)/2;
        r.w = surface30->w;
        r.h = surface30->h;
        SDL_UpperBlit(surface30, NULL, g_backBuffer, &r);
    } else {
        r.x = 200;
        r.y = 195;
        r.w = 240;
    }
    r.h = 50;
    drawText(g_backBuffer, g_imeFont, r, languageString(title), g_emptyColor, 1, false);
    SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, false);
    SDL_Rect target;
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    while (g_busy) {
        if (dot) {
            r.x = 265;
            r.y = (480-dot->h)/2;
            r.w = 140;
            r.h = dot->h;
            if (frame/5%4 == 0) {
                SDL_UpperBlit(surface0c, &r, g_backBuffer, &r);
                if (surface30) {
                    target = r;
                    target.x = r.x-(640-surface30->w)/2;
                    target.y = r.y-(480-surface30->h)/2;
                    SDL_UpperBlit(surface30, &target, g_backBuffer, &r);
                }
            }
            if (frame/5%4 == 1) SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            if (frame/5%4 == 2) {
                target = r;
                target.x = r.x+(140-dot->w*3)/4+dot->w;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &target);
            }
            if (frame/5%4 == 3) {
                target = r;
                target.x = r.x+(140-dot->w*3)/2+dot->w*2;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &target);
            }
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_Flip(g_screen);
        }
        SDL_Delay(28);
        frame++;
    }
    if (dot) SDL_FreeSurface(dot);
    return 0;
}
// 0x349f0
int ProgressDialog::handleInput() {
    return 1;
}
// 0x34a14
GameDetailWindow::GameDetailWindow(int title, int id) : TrimusWindow(title, id) {
    type = 0;
    param = 0;
    favorite = false;
    field84 = 0;
    field88 = 0;
    flag94 = true;
    field8c = 0;
    field98 = 0;
    param90 = 0;
}
// 0x34b1c
GameDetailWindow::GameDetailWindow(int title, int id, const char *name, const char *item, const char *game, const char *icon, int index, int kind, bool fav) : TrimusWindow(title, id) {
    this->item = item;
    this->name = name;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    type = kind;
    param = index;
    favorite = fav;
    field84 = NULL;
    field88 = NULL;
    field8c = NULL;
    flag94 = true;
    param90 = index;
    field98 = NULL;
    loadInfo();
}
// 0x34d0c (D0 0x34d98)
GameDetailWindow::~GameDetailWindow() {
}
// 0x34dcc
void GameDetailWindow::onHide() {
    if (field84) {
        SDL_FreeSurface(field84);
        field84 = NULL;
    }
    if (field88) {
        SDL_FreeSurface(field88);
        field88 = NULL;
    }
    if (field8c) {
        SDL_FreeSurface(field8c);
        field8c = NULL;
    }
}
// 0x34e6c
void GameDetailWindow::onShow() {
    printf("Load thumbnail %s\n", thumbPath.c_str());
    field84 = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (field84 == NULL) field84 = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    if (favorite) field88 = loadSkinImage(g_themePath, g_skinSubdir, "skin/ic-favorite-mark.png", g_backBuffer, false);
}
// 0x34f84
void GameDetailWindow::update() {
    if (g_windowStack && !field98) {
        if (g_windowStack->size() > 2) {
            MenuWindow *parent = (MenuWindow*)(*g_windowStack)[g_windowStack->size()-2];
            field98 = (TextMenuDrawer*)parent->getMenu();
            printf("parent win is %s, %d: %p\n", parent->getTitle(), parent->getId(), field98);
        }
    }
}
// 0x35080
void GameDetailWindow::loadInfo() {
    int length = name.length();
    images.clear();
    std::string line;
    int count = 0;
    int start = 0;
    int end = 0;
    while (start+15 < length && count <= 3 && end < length) {
        end = start+15;
        while (end < length && (name.at(end)&0xc0) == 0x80) end++;
        line = name.substr(start, end-start);
        start = end;
        count++;
        if (end < length && line.at(line.length()-1) != ' ' && count <= 3) line = line+"-";
        images.push_back(line);
    }
    if (start < length-1) {
        if (count < 4) {
            line = name.substr(start);
            images.push_back(line);
            start += line.length();
        } else {
            line = images.back();
            images.pop_back();
            line += "...";
            images.push_back(line);
        }
    }
}
// 0x353a0
void GameDetailWindow::resetDetails(const char *heading, const char *label, const char *game, const char *icon, int index, int kind, bool fav) {
    name = heading;
    item = label;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    printf("GameDetailWindow::resetDetailsInternal(name %s ,label %s) thumbnail path %s\n", heading, label, thumbPath.c_str());
    type = kind;
    param = index;
    favorite = fav;
    if (field84) SDL_FreeSurface(field84);
    field84 = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (field84 == NULL) field84 = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    loadInfo();
}
// 0x35538
void GameDetailWindow::resetFromItem(TextItem *row, int index) {
    if (row->action->result == 3) {
        AppAction *action = (AppAction*)row->action;
        AppInfo *app = action->app;
        int kind = action->result;
        std::string image = app->iconSelected;
        resetDetails(row->label.c_str(), app->label.c_str(), app->launch.c_str(), image.c_str(), index, kind, row->favorite);
    } else {
        EmuAction *action = (EmuAction*)row->action;
        EmuInfo *emu = action->emu;
        int kind = action->result;
        resetDetails(row->label.c_str(), emu->label.c_str(), action->rompath.c_str(), action->imgpath.c_str(), index, kind, row->favorite);
    }
}
// 0x356e4
int GameDetailWindow::draw(SDL_Surface *dst) {
    if (titleLabelId != 157) drawScreenFrame(dst, languageString(titleLabelId), id, true);
    else drawScreenFrame(dst, "MIYOO", id, true);
    SDL_Rect r;
    if (field84) {
        int height = 360;
        SDL_Rect source;
        SDL_Rect dest;
        if (field84->w > 250) {
            source.w = 250;
            source.x = (field84->w-250)/2;
        } else {
            source.w = field84->w;
            source.x = 0;
        }
        if (field84->h > height) {
            source.h = height;
            source.y = (field84->h-height)/2;
        } else {
            source.h = field84->h;
            source.y = 0;
        }
        dest.x = (250-source.w)/2;
        dest.y = (360-field84->h)/2+60;
        SDL_UpperBlit(field84, &source, dst, &dest);
    }
    if (g_numberBg && g_listNumber) {
        r.x = 280;
        r.y = 80;
        r.w = 124;
        r.h = 50;
        drawListNumber(param+1, &r, dst, g_imeFont);
    }
    r.x = 284;
    r.w = 350;
    if (images.size() == 0) {
        r.y = 145;
        r.h = 35;
        drawText(dst, g_titleFont, r, name.c_str(), g_imeTextColor, 0, false);
    } else {
        for (int i = 0; i < images.size(); i++) {
            r.y = i*35+145;
            r.h = 35;
            drawText(dst, g_titleFont, r, images[i].c_str(), g_imeTextColor, 0, false);
        }
    }
    if (field88 && favorite) {
        r.x = 284;
        r.y = r.y+r.h+10;
        SDL_UpperBlit(field88, NULL, dst, &r);
        r.x = r.x+field88->w+10;
        r.w = 630-r.x;
        r.h = 35;
        drawText(dst, g_titleFont, r, languageString(44), g_imeTextColor, 0, false);
    }
    return 0;
}
// 0x35bbc
int GameDetailWindow::handleInput() {
    int key = handleGlobalInput(NULL);
    switch (key) {
    case 1: case 4:
        if (type == 5) {
            int index = findEmuByLabelPrefix(&g_romSourcesA, item.c_str());
            printf("find emu %s return %d\n", item.c_str(), index);
            if (index >= 0 && (unsigned)index < g_romSourcesA.size()) return launchRom(name.c_str(), &g_romSourcesA[index], launch.c_str(), thumbPath.c_str(), type);
        } else if (type == 17) {
            int index = findEmuByLabelPrefix(&g_romSourcesB, item.c_str());
            printf("find emu %s return %d\n", item.c_str(), index);
            if (index >= 0 && (unsigned)index < g_romSourcesB.size()) return launchRom(name.c_str(), &g_romSourcesB[index], launch.c_str(), thumbPath.c_str(), type);
        } else printf("unsupport game type %d\n", type);
        return 1;
    case 2: case 3: case 7: return 1;
    case 5:
        if (field98) {
            field98->vfunc10();
            TextItem *row = (TextItem*)field98->getItem(field98->getCur());
            if (row) resetFromItem(row, field98->getCur());
        }
        break;
    case 6:
        if (field98) {
            field98->vfunc9();
            TextItem *row = (TextItem*)field98->getItem(field98->getCur());
            if (row) resetFromItem(row, field98->getCur());
        }
    // The final case falls through to this shared exit in the original.
    default: break;
    }
    return -1;
}
// 0x35fe4
extern "C" cJSON *cJSON_CreateBool(int);
void GameDetailWindow::saveState(cJSON *obj) {
    cJSON *a = cJSON_CreateString(name.c_str());
    cJSON_AddItemToObject(obj, "gamename", a);
    cJSON *b = cJSON_CreateString(launch.c_str());
    cJSON_AddItemToObject(obj, "gamepath", b);
    cJSON *c = cJSON_CreateString(thumbPath.c_str());
    cJSON_AddItemToObject(obj, "thumbpath", c);
    cJSON *d = cJSON_CreateNumber(param);
    cJSON_AddItemToObject(obj, "gameidx", d);
    cJSON *e = cJSON_CreateNumber(type);
    cJSON_AddItemToObject(obj, "gametype", e);
    cJSON *f = cJSON_CreateBool(favorite);
    cJSON_AddItemToObject(obj, "favourite", f);
}
// 0x36140
int GameDetailWindow::restoreState(cJSON *obj) {
    cJSONItem *entry = (cJSONItem*)cJSON_GetObjectItem(obj, "gamename");
    name = entry->valuestring;
    entry = (cJSONItem*)cJSON_GetObjectItem(obj, "gamepath");
    launch = entry->valuestring;
    entry = (cJSONItem*)cJSON_GetObjectItem(obj, "thumbpath");
    thumbPath = entry->valuestring;
    entry = (cJSONItem*)cJSON_GetObjectItem(obj, "gameidx");
    param = entry->valueint;
    entry = (cJSONItem*)cJSON_GetObjectItem(obj, "gametype");
    type = entry->valueint;
    entry = (cJSONItem*)cJSON_GetObjectItem(obj, "favourite");
    favorite = entry->valueint != 0;
    return 0;
}
// 0x36288
void GameDetailWindow::animateIn(SDL_Surface *dst, FlipFn flip) {
    if (flag94) {
        flag94 = false;
        SDL_Flip(g_screen);
        if (field8c && param == param90) {
            int width = field8c->w;
            int height = field8c->h;
            SDL_Surface *surface = createSurface(width, height);
            if (surface) {
                int now = 0;
                draw(surface);
                float t = 0.0;
                for (int i = 0; i < 20; i++) {
                    t = t+0.05;
                    float eased = 1.0-(1.0-t)*(1.0-t);
                    int x = eased*640.0f;
                    timeval tv;
                    gettimeofday(&tv, NULL);
                    now = tv.tv_sec*1000+tv.tv_usec/1000;
                    printf("anim %d at %d\n", x, now);
                    SDL_Rect source;
                    source.x = x;
                    source.y = 0;
                    source.w = width-x;
                    source.h = height;
                    SDL_Rect dest;
                    dest.x = 0;
                    dest.y = 0;
                    SDL_UpperBlit(field8c, &source, dst, &dest);
                    source.x = 0;
                    source.y = 0;
                    source.w = x;
                    source.h = height;
                    dest.x = width-x;
                    dest.y = 0;
                    SDL_UpperBlit(surface, &source, dst, &dest);
                    flip();
                }
                SDL_FreeSurface(surface);
            }
        }
    }
}
// 0x36534
void GameDetailWindow::animateOut(SDL_Surface *dst, FlipFn flip) {
    flag94 = true;
    if (field8c && param == param90) {
        int width = field8c->w;
        int height = field8c->h;
        SDL_Surface *surface = createSurface(width, height);
        if (surface) {
            draw(surface);
            float t = 0.0;
            for (int i = 0; i < 20; i++) {
                t = t+0.05;
                float eased = 1.0-(1.0-t)*(1.0-t);
                int x = eased*640.0f;
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = width-x;
                source.h = height;
                SDL_Rect dest;
                dest.x = x;
                dest.y = 0;
                SDL_UpperBlit(surface, &source, dst, &dest);
                source.x = width-x;
                source.y = 0;
                source.w = x;
                source.h = height;
                dest.x = 0;
                dest.y = 0;
                SDL_UpperBlit(field8c, &source, dst, &dest);
                flip();
            }
            SDL_FreeSurface(surface);
        }
    }
}
// 0x3675c
void GameDetailWindow::captureBackground(SDL_Surface *source) {
    if (source) {
        field8c = createSurface(source->w, source->h);
        if (field8c) SDL_UpperBlit(source, NULL, field8c, NULL);
    }
}
// 0x367d8
static unsigned detailTimeout(unsigned interval, void *arg) {
    WindowUserEvent event;
    event.type = 24;
    event.code = 0;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
