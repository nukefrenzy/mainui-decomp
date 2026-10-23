// tu_015b20 part p10 (0x3225c-0x36838): pop-ups, dialogs, the power-off countdown and the game detail window.

// 0x3225c: a pop-up list (the options of MENU) over the dimmed screen, sized to its background
// image (skin/bg-pop-menu-N.png).
PopUpWindow::PopUpWindow(int titleLabelId, TextMenu *list, const char *file) : FloatWindow(titleLabelId)
{
    background = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    menu = list;
    printf("load menu bg %s : %p\n", file, background);
    if (menu) {
        SDL_Rect rect;
        if (background) {
            rect.x = rect.y = 0;
            rect.w = background->w;
            rect.h = background->h - 8;
            if (menu->count() > 1)
                menu->setRowHeight(rect.h / menu->count());
            else
                menu->setRowHeight(rect.h);
        } else {
            rect.x = rect.y = 0;
            rect.w = 120;
            rect.h = 75;
        }
        menu->setRect(&rect);
    }
}
// 0x3247c (D0 0x32534)
PopUpWindow::~PopUpWindow()
{
    if (menu) {
        delete menu;
        menu = NULL;
    }
    if (background) SDL_FreeSurface(background);
    puts("~PopupWindow end");
}
// 0x32568: nothing to do on close.
void PopUpWindow::onHide()
{
}
// 0x32588: the dimmed backdrop, the footer with OK/Cancel, the pop-up background and its rows.
int PopUpWindow::draw(SDL_Surface *dst)
{
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
            source.y = g_unusedIndicator->h - 60;
            SDL_UpperBlit(g_unusedIndicator, &source, dst, &dest);
        }
        drawFooter(dst, languageString(STR_OK), languageString(STR_CANCEL));
    }
    if (menu) {
        if (background) {
            SDL_Rect dest;
            dest.x = 0;
            dest.y = 0;
            SDL_UpperBlit(background, NULL, dst, &dest);
        } else {
            SDL_Rect source = {
                0, 0, 0, 0};
            source.w = menu->getRect()->w;
            source.h = menu->getRect()->h;
            SDL_UpperBlit(g_background, &source, dst, menu->getRect());
        }
        menu->drawList(dst);
    }
    return 0;
}
// 0x32784: the list handles the buttons (1 closes).
int PopUpWindow::handleInput()
{
    if (menu) return menu->handleKey();
    return 1;
}
// 0x327e4: a short message with one button image; it closes on any key or after 5 s (and then calls
// fn).
NotifyWindow::NotifyWindow(const char *message, const char *button, const char *file, void (*fn)()) : FloatWindow(-1)
{
    text = message;
    notifyTimer = NULL;
    notifyBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    notifyButton = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    buttonLabel = button;
    callback = fn;
}
// 0x32920 (D0 0x329cc)
NotifyWindow::~NotifyWindow()
{
    if (notifyTimer) SDL_RemoveTimer(notifyTimer);
    if (notifyBackground) SDL_FreeSurface(notifyBackground);
    if (notifyButton) SDL_FreeSurface(notifyButton);
}
// 0x32a00: timer: post EVENT_NOTIFY_TIMEOUT and run the callback.
static unsigned notifyTimeout(unsigned interval, void *arg)
{
    void (*fn)() = (void (*)())arg;
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_NOTIFY_TIMEOUT;
    timeval now;
    gettimeofday(&now, NULL);
    printf("timeout sec = %d\n", now.tv_sec);
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    if (fn) fn();
    return interval;
}
// 0x32aa0: start the 5 s timer; without a pop-up skin, cut a bordered box out of the background
// instead.
void NotifyWindow::onShow()
{
    FloatWindow::onShow();
    timeval now;
    gettimeofday(&now, NULL);
    printf("init sec = %d\n", now.tv_sec);
    notifyTimer = SDL_AddTimer(5000, notifyTimeout, (void *)callback);
    if (notifyBackground == NULL && g_background) {
        notifyBackground = SDL_CreateRGBSurface(0, 240, 90, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            200, 195, 240, 90};
        SDL_UpperBlit(g_background, &r, notifyBackground, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 89;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
    }
}
// 0x32d0c: any key release or the timeout closes it.
int NotifyWindow::handleInput()
{
    SDL_Event event;
    int key = handleGlobalInput(&event);
    if (event.type == SDL_KEYUP)
        return 1;
    else if (event.type == SDL_USEREVENT && event.user.code == EVENT_NOTIFY_TIMEOUT)
        return 1;
    return -1;
}
// 0x32d74: the box, the message (several lines) and the button with its label.
int NotifyWindow::draw(SDL_Surface *dst)
{
    FloatWindow::draw(dst);
    SDL_Rect r;
    if (notifyBackground) {
        r.x = (640 - notifyBackground->w) / 2;
        r.y = (480 - notifyBackground->h) / 2;
        r.w = notifyBackground->w;
        r.h = notifyBackground->h;
        SDL_UpperBlit(notifyBackground, NULL, dst, &r);
    } else {
        r.x = 200;
        r.y = 195;
        r.w = 240;
        r.h = 90;
    }
    if (notifyButton) {
        r.h -= notifyButton->h / 2;
        r.y -= notifyButton->h / 4;
    }
    drawTextLines(dst, g_titleFont, &r, text.c_str(), "\n", &g_selectedTextColor, 1, false);
    if (notifyButton && notifyBackground) {
        r.x = (640 - notifyButton->w) / 2;
        r.y = 468 - (480 - notifyBackground->h) / 2 - notifyButton->h;
        SDL_UpperBlit(notifyButton, NULL, dst, &r);
        r.w = notifyButton->w;
        r.h = notifyButton->h;
        drawText(dst, g_titleFont, r, buttonLabel, g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33060: the power-off countdown: "Power off in N seconds" from 10; any button cancels (calling
// cancel), the end calls timeout.
CountDownWindow::CountDownWindow(int label, void (*timeout)(), void (*cancel)()) : NotifyWindow("", languageString(STR_CANCEL), "skin/bg-button-f.png", timeout)
{
    title = label;
    seconds = 10;
    tickTimer = NULL;
    onCancel = cancel;
}
// 0x330fc (D0 0x33158)
CountDownWindow::~CountDownWindow()
{
    if (tickTimer) SDL_RemoveTimer(tickTimer);
}
// 0x3318c: timer: post EVENT_COUNTDOWN_TICK every second.
static unsigned countdownTick(unsigned interval, void *arg)
{
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_COUNTDOWN_TICK;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
// 0x331ec: start the full-length timeout and the one-second tick.
void CountDownWindow::onShow()
{
    NotifyWindow::onShow();
    snprintf(countdownText, 128, languageString(title), seconds);
    text = countdownText;
    if (notifyTimer) SDL_RemoveTimer(notifyTimer);
    notifyTimer = SDL_AddTimer(seconds * 1000, notifyTimeout, (void *)callback);
    tickTimer = SDL_AddTimer(1000, countdownTick, NULL);
}
// 0x332d8: a tick counts down; any other input except status events cancels.
int CountDownWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    if (key != EVENT_BATTERY && key != EVENT_HOTPLUG && key != EVENT_COUNTDOWN_TICK) {
        if (onCancel) onCancel();
        return 1;
    } else {
        if (key == EVENT_COUNTDOWN_TICK) {
            seconds--;
            snprintf(countdownText, 128, languageString(title), seconds);
            text = countdownText;
            SDL_RemoveTimer(tickTimer);
            tickTimer = SDL_AddTimer(1000, countdownTick, NULL);
        }
        return -1;
    }
}
// 0x333ec: a yes/no question: heading and message are language strings; OK runs action (owned when
// owns).
DialogWindow::DialogWindow(int heading, int message, bool owns, Action *act) : FloatWindow(-1)
{
    title = heading;
    text = message;
    selected = 1;
    popupBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    if (popupBackground) {
        noPopupSkin = false;
        buttonHighlight = NULL;
    } else {
        buttonHighlight = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        noPopupSkin = true;
    }
    iconA = g_iconA;
    iconB = g_iconB;
    action = act;
    ownsAction = owns;
}
// 0x33570 (D0 0x33630)
DialogWindow::~DialogWindow()
{
    if (popupBackground) SDL_FreeSurface(popupBackground);
    if (buttonHighlight) SDL_FreeSurface(buttonHighlight);
    if (action) delete action;
}
// 0x33664: without a pop-up skin, draw a bordered box with two button cells instead.
void DialogWindow::onShow()
{
    FloatWindow::onShow();
    if (g_background && noPopupSkin) {
        popupBackground = SDL_CreateRGBSurface(0, 240, 120, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            0, 0, 240, 120};
        SDL_UpperBlit(g_background, &r, popupBackground, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 119;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 90;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 120;
        r.y = 90;
        r.w = 1;
        r.h = 30;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
    }
}
// 0x33934: the plain look: message and two buttons, Cancel and OK, the selected one highlighted.
int DialogWindow::drawFallback(SDL_Surface *dst)
{
    SDL_Rect r;
    r.x = 200;
    r.y = 180;
    r.w = 240;
    r.h = 120;
    if (popupBackground) SDL_UpperBlit(popupBackground, NULL, dst, &r);
    r.h = 90;
    drawText(dst, g_titleFont, r, languageString(text), g_selectedTextColor, 1, false);
    for (int i = 0; i < 2; i++) {
        r.x = 200 + i * 120;
        r.y = 270;
        r.w = 120;
        r.h = 30;
        if (i == selected) {
            if (buttonHighlight) {
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = r.w;
                source.h = r.h;
                SDL_UpperBlit(buttonHighlight, &source, dst, &r);
            }
            drawText(dst, g_titleFont, r, (i == 0 ? languageString(STR_CANCEL) : languageString(STR_OK)), g_imeTextColor, 1, false);
        } else
            drawText(dst, g_titleFont, r, (i == 0 ? languageString(STR_CANCEL) : languageString(STR_OK)), g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33bac: the skinned look: heading, message, and A = OK / B = Cancel hints.
int DialogWindow::draw(SDL_Surface *dst)
{
    FloatWindow::draw(dst);
    if (noPopupSkin != true && popupBackground) {
        SDL_Rect r;
        r.x = (640 - popupBackground->w) / 2;
        r.y = (480 - popupBackground->h) / 2;
        r.w = popupBackground->w;
        r.h = popupBackground->h;
        SDL_UpperBlit(popupBackground, NULL, dst, &r);
        r.h = 50;
        drawText(dst, g_titleFont, r, languageString(title), g_emptyColor, 1, false);
        r.y = (480 - popupBackground->h) / 2 + 50;
        r.h = 100;
        drawTextLines(dst, g_titleFont, &r, languageString(text), "\n", &g_selectedTextColor, 1, false);
        r.x = (640 - popupBackground->w) / 2 + 230;
        r.y = (480 - popupBackground->h) / 2 + 160;
        r.w = popupBackground->w;
        r.h = iconA->h;
        SDL_UpperBlit(iconA, NULL, dst, &r);
        r.x = r.x + iconA->w + 10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(STR_OK), g_clearColor, 3, false);
        r.x = (640 - popupBackground->w) / 2 + 230;
        r.y = (480 - popupBackground->h) / 2 + iconA->h + 170;
        SDL_UpperBlit(iconB, NULL, dst, &r);
        r.x = r.x + iconB->w + 10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(STR_CANCEL), g_clearColor, 3, false);
    } else
        return drawFallback(dst);
    return 0;
}
// 0x3400c: A or Y on OK runs the action; B, X or A on Cancel closes; left/right choose the button.
int DialogWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_A:
        case BUTTON_Y:
            if (selected == 1 && action) return action->execute(NULL);
            return 1;
        case BUTTON_B:
        case BUTTON_X: return 1;
        case BUTTON_LEFT:
            if (selected > 0) selected--;
            break;
        case BUTTON_RIGHT:
            if (selected < 1) selected++;
            break;
        default: break;
    }
    return -1;
}
// 0x34134: thread: run the dialog's action while g_busy is set.
static int progressThread(void *arg)
{
    ProgressDialog *dialog = (ProgressDialog *)arg;
    g_busy = 1;
    dialog->runAction();
    g_busy = 0;
    return 0;
}
// 0x34184: a "please wait" pop-up that runs an action on a thread (e.g. Wi-Fi connect, refresh
// ROMs).
ProgressDialog::ProgressDialog(int heading, Action *act) : FloatWindow(-1)
{
    title = heading;
    action = act;
    progressBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    progressButton = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-button-f.png", g_backBuffer, false);
    actionThread = NULL;
}
// 0x342ac (D0 0x3437c)
ProgressDialog::~ProgressDialog()
{
    if (progressBackground) SDL_FreeSurface(progressBackground);
    if (progressButton) SDL_FreeSurface(progressButton);
    if (action) delete action;
}
// 0x343b0: start the action thread.
void ProgressDialog::onShow()
{
    FloatWindow::onShow();
    actionThread = SDL_CreateThread(progressThread, this);
}
// 0x343f4
void ProgressDialog::runAction()
{
    action->execute(NULL);
}
// 0x34438: wait for the action to finish.
void ProgressDialog::onHide()
{
    SDL_WaitThread(actionThread, NULL);
}
// 0x34468: draws the pop-up and animates the dots until the action finishes (this blocks the window
// loop).
int ProgressDialog::draw(SDL_Surface *dst)
{
    FloatWindow::draw(g_backBuffer);
    int frame = 0;
    SDL_Rect r;
    if (progressBackground) {
        r.x = (640 - progressBackground->w) / 2;
        r.y = (480 - progressBackground->h) / 2;
        r.w = progressBackground->w;
        r.h = progressBackground->h;
        SDL_UpperBlit(progressBackground, NULL, g_backBuffer, &r);
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
            r.y = (480 - dot->h) / 2;
            r.w = 140;
            r.h = dot->h;
            if (frame / 5 % 4 == 0) {
                SDL_UpperBlit(backdrop, &r, g_backBuffer, &r);
                if (progressBackground) {
                    target = r;
                    target.x = r.x - (640 - progressBackground->w) / 2;
                    target.y = r.y - (480 - progressBackground->h) / 2;
                    SDL_UpperBlit(progressBackground, &target, g_backBuffer, &r);
                }
            }
            if (frame / 5 % 4 == 1) SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            if (frame / 5 % 4 == 2) {
                target = r;
                target.x = r.x + (140 - dot->w * 3) / 4 + dot->w;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &target);
            }
            if (frame / 5 % 4 == 3) {
                target = r;
                target.x = r.x + (140 - dot->w * 3) / 2 + dot->w * 2;
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
// 0x349f0: closes as soon as the loop gets input back (the action is done).
int ProgressDialog::handleInput()
{
    return 1;
}
// 0x34a14: an empty game detail window (state restore fills it).
GameDetailWindow::GameDetailWindow(int title, int id) : TrimusWindow(title, id)
{
    sourceWindowId = 0;
    listIndex = 0;
    favorite = false;
    thumbnailImage = 0;
    favoriteImage = 0;
    animatePending = true;
    animationCapture = 0;
    parentMenu = 0;
    openedIndex = 0;
}
// 0x34b1c: a game detail window: title, item (emulator label), game path, thumbnail folder, list
// index, window id of the list (kind) and favorite mark.
GameDetailWindow::GameDetailWindow(int title, int id, const char *name, const char *item, const char *game, const char *icon, int index, int kind, bool fav) : TrimusWindow(title, id)
{
    this->item = item;
    this->name = name;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    sourceWindowId = kind;
    listIndex = index;
    favorite = fav;
    thumbnailImage = NULL;
    favoriteImage = NULL;
    animationCapture = NULL;
    animatePending = true;
    openedIndex = index;
    parentMenu = NULL;
    loadInfo();
}
// 0x34d0c (D0 0x34d98)
GameDetailWindow::~GameDetailWindow()
{
}
// 0x34dcc: free the images.
void GameDetailWindow::onHide()
{
    if (thumbnailImage) {
        SDL_FreeSurface(thumbnailImage);
        thumbnailImage = NULL;
    }
    if (favoriteImage) {
        SDL_FreeSurface(favoriteImage);
        favoriteImage = NULL;
    }
    if (animationCapture) {
        SDL_FreeSurface(animationCapture);
        animationCapture = NULL;
    }
}
// 0x34e6c: load the thumbnail (or the default one) and the favorite mark.
void GameDetailWindow::onShow()
{
    printf("Load thumbnail %s\n", thumbPath.c_str());
    thumbnailImage = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (thumbnailImage == NULL) thumbnailImage = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    if (favorite) favoriteImage = loadSkinImage(g_themePath, g_skinSubdir, "skin/ic-favorite-mark.png", g_backBuffer, false);
}
// 0x34f84: find the list this window was opened from (the window below it), to move through it with
// up/down.
void GameDetailWindow::update()
{
    if (g_windowStack && !parentMenu) {
        if (g_windowStack->size() > 2) {
            MenuWindow *parent = (MenuWindow *)(*g_windowStack)[g_windowStack->size() - 2];
            parentMenu = (TextMenuDrawer *)parent->getMenu();
            printf("parent win is %s, %d: %p\n", parent->getTitle(), parent->getId(), parentMenu);
        }
    }
}
// 0x35080: wraps the game name onto up to four lines of about 15 bytes (not splitting UTF-8
// characters), adding "-" or "...".
void GameDetailWindow::loadInfo()
{
    int length = name.length();
    nameLines.clear();
    std::string line;
    int count = 0;
    int start = 0;
    int end = 0;
    while (start + 15 < length && count <= 3 && end < length) {
        end = start + 15;
        while (end < length && (name.at(end) & 0xc0) == 0x80) end++;
        line = name.substr(start, end - start);
        start = end;
        count++;
        if (end < length && line.at(line.length() - 1) != ' ' && count <= 3) line = line + "-";
        nameLines.push_back(line);
    }
    if (start < length - 1) {
        if (count < 4) {
            line = name.substr(start);
            nameLines.push_back(line);
            start += line.length();
        } else {
            line = nameLines.back();
            nameLines.pop_back();
            line += "...";
            nameLines.push_back(line);
        }
    }
}
// 0x353a0: show another game.
void GameDetailWindow::resetDetails(const char *heading, const char *label, const char *game, const char *icon, int index, int kind, bool fav)
{
    name = heading;
    item = label;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    printf("GameDetailWindow::resetDetailsInternal(name %s ,label %s) thumbnail path %s\n", heading, label, thumbPath.c_str());
    sourceWindowId = kind;
    listIndex = index;
    favorite = fav;
    if (thumbnailImage) SDL_FreeSurface(thumbnailImage);
    thumbnailImage = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (thumbnailImage == NULL) thumbnailImage = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    loadInfo();
}
// 0x35538: show the game of a list row (app or ROM).
void GameDetailWindow::resetFromItem(TextItem *row, int index)
{
    if (row->action->result == WIN_APPS) {
        AppAction *action = (AppAction *)row->action;
        AppInfo *app = action->app;
        int kind = action->result;
        std::string image = app->iconSelected;
        resetDetails(row->label.c_str(), app->label.c_str(), app->launch.c_str(), image.c_str(), index, kind, row->favorite);
    } else {
        EmuAction *action = (EmuAction *)row->action;
        EmuInfo *emu = action->emu;
        int kind = action->result;
        resetDetails(row->label.c_str(), emu->label.c_str(), action->rompath.c_str(), action->imgpath.c_str(), index, kind, row->favorite);
    }
}
// 0x356e4: the thumbnail on the left, the list number, the wrapped name and the favorite mark.
int GameDetailWindow::draw(SDL_Surface *dst)
{
    if (titleLabelId != 157)
        drawScreenFrame(dst, languageString(titleLabelId), id, true);   /* 157: "MIYOO" */
    else
        drawScreenFrame(dst, "MIYOO", id, true);
    SDL_Rect r;
    if (thumbnailImage) {
        int height = 360;
        SDL_Rect source;
        SDL_Rect dest;
        if (thumbnailImage->w > 250) {
            source.w = 250;
            source.x = (thumbnailImage->w - 250) / 2;
        } else {
            source.w = thumbnailImage->w;
            source.x = 0;
        }
        if (thumbnailImage->h > height) {
            source.h = height;
            source.y = (thumbnailImage->h - height) / 2;
        } else {
            source.h = thumbnailImage->h;
            source.y = 0;
        }
        dest.x = (250 - source.w) / 2;
        dest.y = (360 - thumbnailImage->h) / 2 + 60;
        SDL_UpperBlit(thumbnailImage, &source, dst, &dest);
    }
    if (g_numberBg && g_listNumber) {
        r.x = 280;
        r.y = 80;
        r.w = 124;
        r.h = 50;
        drawListNumber(listIndex + 1, &r, dst, g_imeFont);
    }
    r.x = 284;
    r.w = 350;
    if (nameLines.size() == 0) {
        r.y = 145;
        r.h = 35;
        drawText(dst, g_titleFont, r, name.c_str(), g_imeTextColor, 0, false);
    } else {
        for (int i = 0; i < nameLines.size(); i++) {
            r.y = i * 35 + 145;
            r.h = 35;
            drawText(dst, g_titleFont, r, nameLines[i].c_str(), g_imeTextColor, 0, false);
        }
    }
    if (favoriteImage && favorite) {
        r.x = 284;
        r.y = r.y + r.h + 10;
        SDL_UpperBlit(favoriteImage, NULL, dst, &r);
        r.x = r.x + favoriteImage->w + 10;
        r.w = 630 - r.x;
        r.h = 35;
        drawText(dst, g_titleFont, r, languageString(STR_FAVORITE_LOWER), g_imeTextColor, 0, false);
    }
    return 0;
}
// 0x35bbc: A/Y launch the game with its emulator; B, X or LEFT close; up/down move to the
// previous/next game of the list.
int GameDetailWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_A:
        case BUTTON_Y:
            if (sourceWindowId == WIN_ROM_LIST) {
                int index = findEmuByLabelPrefix(&g_emuFolders, item.c_str());
                printf("find emu %s return %d\n", item.c_str(), index);
                if (index >= 0 && (unsigned)index < g_emuFolders.size()) return launchRom(name.c_str(), &g_emuFolders[index], launch.c_str(), thumbPath.c_str(), sourceWindowId);
            } else if (sourceWindowId == WIN_RETROARCH_LIST) {
                int index = findEmuByLabelPrefix(&g_rappFolders, item.c_str());
                printf("find emu %s return %d\n", item.c_str(), index);
                if (index >= 0 && (unsigned)index < g_rappFolders.size()) return launchRom(name.c_str(), &g_rappFolders[index], launch.c_str(), thumbPath.c_str(), sourceWindowId);
            } else
                printf("unsupport game type %d\n", sourceWindowId);
            return 1;
        case BUTTON_B:
        case BUTTON_X:
        case BUTTON_LEFT: return 1;
        case BUTTON_UP:
            if (parentMenu) {
                parentMenu->moveUp();
                TextItem *row = (TextItem *)parentMenu->getItem(parentMenu->getCur());
                if (row) resetFromItem(row, parentMenu->getCur());
            }
            break;
        case BUTTON_DOWN:
            if (parentMenu) {
                parentMenu->moveDown();
                TextItem *row = (TextItem *)parentMenu->getItem(parentMenu->getCur());
                if (row) resetFromItem(row, parentMenu->getCur());
            }
        // The final case falls through to this shared exit in the original.
        default: break;
    }
    return -1;
}
// 0x35fe4: what /tmp/state.json keeps of this window.

void GameDetailWindow::saveState(cJSON *obj)
{
    cJSON *gameName = cJSON_CreateString(name.c_str());
    cJSON_AddItemToObject(obj, "gamename", gameName);
    cJSON *gamePath = cJSON_CreateString(launch.c_str());
    cJSON_AddItemToObject(obj, "gamepath", gamePath);
    cJSON *thumb = cJSON_CreateString(thumbPath.c_str());
    cJSON_AddItemToObject(obj, "thumbpath", thumb);
    cJSON *gameIndex = cJSON_CreateNumber(listIndex);
    cJSON_AddItemToObject(obj, "gameidx", gameIndex);
    cJSON *gameType = cJSON_CreateNumber(sourceWindowId);
    cJSON_AddItemToObject(obj, "gametype", gameType);
    cJSON *isFavorite = cJSON_CreateBool(favorite);
    cJSON_AddItemToObject(obj, "favourite", isFavorite);
}
// 0x36140: reads it back.
int GameDetailWindow::restoreState(cJSON *obj)
{
    cJSONItem *entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gamename");
    name = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gamepath");
    launch = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "thumbpath");
    thumbPath = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gameidx");
    listIndex = entry->valueint;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gametype");
    sourceWindowId = entry->valueint;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "favourite");
    favorite = entry->valueint != 0;
    return 0;
}
// 0x36288: slides the window in from the right over the captured list (20 eased steps), once.
void GameDetailWindow::animateIn(SDL_Surface *dst, FlipFn flip)
{
    if (animatePending) {
        animatePending = false;
        SDL_Flip(g_screen);
        if (animationCapture && listIndex == openedIndex) {
            int width = animationCapture->w;
            int height = animationCapture->h;
            SDL_Surface *surface = createSurface(width, height);
            if (surface) {
                int now = 0;
                draw(surface);
                float t = 0.0;
                for (int i = 0; i < 20; i++) {
                    t = t + 0.05;
                    float eased = 1.0 - (1.0 - t) * (1.0 - t);
                    int x = eased * 640.0f;
                    timeval tv;
                    gettimeofday(&tv, NULL);
                    now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
                    printf("anim %d at %d\n", x, now);
                    SDL_Rect source;
                    source.x = x;
                    source.y = 0;
                    source.w = width - x;
                    source.h = height;
                    SDL_Rect dest;
                    dest.x = 0;
                    dest.y = 0;
                    SDL_UpperBlit(animationCapture, &source, dst, &dest);
                    source.x = 0;
                    source.y = 0;
                    source.w = x;
                    source.h = height;
                    dest.x = width - x;
                    dest.y = 0;
                    SDL_UpperBlit(surface, &source, dst, &dest);
                    flip();
                }
                SDL_FreeSurface(surface);
            }
        }
    }
}
// 0x36534: slides it back out.
void GameDetailWindow::animateOut(SDL_Surface *dst, FlipFn flip)
{
    animatePending = true;
    if (animationCapture && listIndex == openedIndex) {
        int width = animationCapture->w;
        int height = animationCapture->h;
        SDL_Surface *surface = createSurface(width, height);
        if (surface) {
            draw(surface);
            float t = 0.0;
            for (int i = 0; i < 20; i++) {
                t = t + 0.05;
                float eased = 1.0 - (1.0 - t) * (1.0 - t);
                int x = eased * 640.0f;
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = width - x;
                source.h = height;
                SDL_Rect dest;
                dest.x = x;
                dest.y = 0;
                SDL_UpperBlit(surface, &source, dst, &dest);
                source.x = width - x;
                source.y = 0;
                source.w = x;
                source.h = height;
                dest.x = 0;
                dest.y = 0;
                SDL_UpperBlit(animationCapture, &source, dst, &dest);
                flip();
            }
            SDL_FreeSurface(surface);
        }
    }
}
// 0x3675c: keep a copy of the screen to slide over.
void GameDetailWindow::captureBackground(SDL_Surface *source)
{
    if (source) {
        animationCapture = createSurface(source->w, source->h);
        if (animationCapture) SDL_UpperBlit(source, NULL, animationCapture, NULL);
    }
}
// 0x367d8: timer posting EVENT_NOTIFY_TIMEOUT; the ADC info window (p11) uses it as a 500 ms
// refresh tick.
static unsigned detailTimeout(unsigned interval, void *arg)
{
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_NOTIFY_TIMEOUT;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
