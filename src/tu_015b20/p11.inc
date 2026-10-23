// tu_015b20 part p11 (0x36838-0x39240): ADC info, the window loop, the menu factory and the charging screen.
#include <fcntl.h>
#include "../HardwareTestMenu.h"

extern std::vector<TrimusWindow *> g_pendingWindows;
extern SDL_Surface *g_gameFocused, *g_gameNormal, *g_raRow;
// 0x1821c0, 0x1821c4, 0x1821d0
extern SDL_Thread *g_loadingThread;
// 0x180854
extern TTF_Font *g_iconFont;
// 0x182180
bool sdCardMounted();
int enableSwap();
bool dualPartition();
// 0x36838: "ADC INFO": shows the battery reading, refreshed every 500 ms.
ADCInfoWindow::ADCInfoWindow(int title, int id) : TrimusWindow(title, id)
{
    timer = SDL_AddTimer(500, detailTimeout, NULL);
}
// 0x368b8 (D0 0x36914)
ADCInfoWindow::~ADCInfoWindow()
{
    if (timer) SDL_RemoveTimer(timer);
}
// 0x36948: B closes.
int ADCInfoWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_B: return 1;
        default: break;
    }
    return -1;
}
// 0x36990: the battery value and a status line. Original quirk, kept as-is: the status is fixed at
// "Disconnected".
int ADCInfoWindow::draw(SDL_Surface *dst)
{
    if (titleLabelId != 157)
        drawScreenFrame(dst, languageString(titleLabelId), id, true);
    else
        drawScreenFrame(dst, "MIYOO", id, true);
    int status = 2;   // never read from the device
    const char *label = NULL;
    switch (status) {
        case 0:
            label = languageString(STR_CHARGING);
            break;
        case 1:
            label = languageString(STR_OTG);
            break;
        case 2:
            label = languageString(STR_DISCONNECTED);
            break;
        case 3:
            label = languageString(STR_ERROR);
            break;
    }
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    int battery = 100;
    int voltage = 0;
    int charging = 0;
    readAxpStatus(&battery, &voltage, &charging);
    snprintf(g_launchCmd, 0x800, "%s     %d\n%s  %s\n", languageString(STR_ADC), battery, languageString(STR_STATUS), label);
    drawTextLines(dst, g_imeFont, &rect, g_launchCmd, "\n", &g_imeTextColor, 1, false);
    return 0;
}
// 0x36b68: closes and deletes every window, top first.
void clearWindows(std::vector<TrimusWindow *> *windows)
{
    printf("%ld window to free\n", windows->size());
    while (windows->size() != 0) {
        TrimusWindow *window = windows->back();
        windows->pop_back();
        window->onHide();
        delete window;
    }
}
// 0x36c20: show the back buffer: copy it to the screen and pan the framebuffer.
void flipScreen()
{
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    flipDisplay();
}
// 0x36c58: show the back buffer with SDL_UpdateRect (the charging screen).
void updateScreen()
{
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_UpdateRect(g_screen, 0, 0, 0, 0);
}
// 0x36cb8: deletes windows queued by pushWindow that were never shown.
void flushPendingWindows()
{
    for (int i = 0; i < g_pendingWindows.size(); i++) {
        g_pendingWindows[i]->onHide();
        delete g_pendingWindows[i];
    }
    g_pendingWindows.clear();
}
// 0x36d88: queue a window to open on top; the window loop shows it after the current frame (only
// the last one queued is kept).
void pushWindow(TrimusWindow *window)
{
    flushPendingWindows();
    g_pendingWindows.push_back(window);
}
// 0x36dbc: opens /dev/fb0 and pans back to the first page.
void initFramebuffer()
{
    g_fbFd = open("/dev/fb0", 2);
    if (g_fbFd >= 0) {
        if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &g_fbVar) == -1) {
            close(g_fbFd);
            g_fbFd = -1;
            puts("cat't ioctl /dev/fb0 ");
        } else {
            g_fbVar.yoffset = 0;
            ioctl(g_fbFd, FBIOPAN_DISPLAY, &g_fbVar);
        }
    } else
        puts("cat't open /dev/fb0 ");
}
// 0x36eb0
void closeFramebuffer()
{
    if (g_fbFd >= 0) close(g_fbFd);
}
// 0x36ee8: the window loop. Each frame: update, slide in, draw the top window and the status icons
// (battery, sound card, Wi-Fi, volume), show it; open a queued window if there is one; otherwise
// hand every pending input event to the top window and act on its result (WindowResult). Ends when
// the stack is empty, a window quits or launches, or SIGTERM; then saves the window state for the
// next start and the ROM list positions.
void runWindowLoop(std::vector<TrimusWindow *> *windows, std::atomic<bool> *running)
{
    int result;
    int slot;
    int count;
    TrimusWindow *window;
    int windowId;
    int next;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    }
    g_windowStack = windows;
    count = 0;
    while (windows->size() != 0 && *running) {
        window = windows->back();
        window->update();
        window->animateIn(g_backBuffer, flipScreen);
        window->draw(g_backBuffer);
        windowId = window->getId();
        if (windowId != WIN_TEST && windowId != -1) {
            slot = drawBatteryIndicator(g_backBuffer, true);
            next = drawSoundCardIndicator(g_backBuffer, true);
            if (next >= 0) slot = next;
            next = drawWifiIndicator(g_backBuffer, slot, true);
            if (next >= 0) slot = next;
            if (g_volumeIndicatorVisible) drawKeyStateIndicator(g_backBuffer, slot, true);
        }
        flipScreen();
        if (g_pendingWindows.size() != 0) {
            TrimusWindow *pending = g_pendingWindows.back();
            pending->onShow();
            windows->push_back(pending);
            g_pendingWindows.pop_back();
            flushPendingWindows();
            continue;
        } else {
            count = 0;
            do {
                if ((result = window->handleInput()) >= 0) {
                    if ((result == WINDOW_CLOSE && windows->size() > 1) || result == WINDOW_FORCE_CLOSE) {
                        window = windows->back();
                        window->animateOut(g_backBuffer, flipScreen);
                        window->onHide();
                        printf("hide window:");
                        window->dump();
                        delete window;
                        puts("top windows is delete");
                        windows->pop_back();
                        break;
                    }
                    if (result == WINDOW_LAUNCH) {
                        window = windows->back();
                        if (window->getId() < 0) {
                            window->onHide();
                            delete window;
                            windows->pop_back();
                        }
                        break;
                    }
                }
                SDL_PumpEvents();
                count++;
            } while (SDL_PeepEvents(&event, 1, 1, ~0u) > 0);
            if (result == WINDOW_LAUNCH || result == WINDOW_QUIT) break;
        }
    }
    if (result == WINDOW_CLOSE) unlink("/tmp/cmd_to_run.sh");
    saveWindowState(windows, "/tmp/state.json");
    saveRomListPositions(&g_romListPositions);
    clearWindows(windows);
}
// 0x373c4: the root window: the main menu, titled "MIYOO".
void createRootWindow(std::vector<TrimusWindow *> *windows)
{
    ListMenu *menu = createMenu(WIN_MAIN, "");
    MenuWindow *window = new MenuWindow(157, WIN_MAIN, menu) /* 157: MenuWindow::draw titles it "MIYOO" */;
    window->onShow();
    windows->push_back((TrimusWindow *)window);
}
// 0x374bc: the title of a window id, or NULL.
const char *windowTitle(int id)
{
    switch (id) {
        case WIN_GAMES: return languageString(STR_GAME);
        case WIN_FAVORITES: return languageString(STR_FAVORITE);
        case WIN_APPS: return languageString(STR_APP);
        case WIN_FILES: return languageString(STR_FILE);
        case WIN_OTHER: return languageString(STR_OTHER);
        case WIN_SETTING: return languageString(STR_SETTING);
        case WIN_GAME_INFO: return languageString(STR_GAME_INFO_TITLE);
        case WIN_DEVICE_INFO: return languageString(STR_DEVICE_INFO_TITLE);
        case WIN_ADC_INFO: return languageString(STR_ADC_INFO_TITLE);
        case WIN_ENCDEC_TEST: return languageString(STR_ENCDEC_TEST);
        case WIN_RECENT: return languageString(STR_RECENT);
        case WIN_SEARCH: return languageString(STR_SEARCH);
        default: return NULL;
    }
}
// 0x37610: builds a ROM list behind the busy screen (the cache may need building first).
ListMenu *createRomListMenu(EmuInfo *emu, int id, std::string *dir, std::string *search)
{
    g_progressCur = 0;
    if (emu->path.size() != 0) {
        printf("load files in %s %s\n", emu->path.c_str(), dir->c_str());
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = emu->label.c_str();
        g_progressTotal = 0;
        g_progressCur = 0;
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        ListMenu *menu = buildRomList(emu, id, dir, search);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else
        return NULL;
}
// 0x37760: returns the theme's small font.
static TTF_Font *smallMenuFont()
{
    return g_smallFont;
}
// Onion patched twelve calls in createMenu to NOPs (mov r0, r0) after compiling, which disables
// six menu items (docs/onion-changes.md). This macro reproduces that: while createMenu is
// assembled, a `bl` at one of the listed offsets from the function start becomes the NOP, and
// every other call stays a normal call. The arguments are still set up, as in the original.
asm(R"(
.set .Lmainui_patched_calls, 0
.macro bl target
    .set .Lmainui_skip_call, 0
    .irp patch, 0x210, 0x21c, 0xa4c, 0xa58, 0xae4, 0xaf0, 0xb60, 0xb6c, 0xb98, 0xba4, 0xc08, 0xc14
        .if (. - _Z10createMenuiNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE) == \patch
            .set .Lmainui_skip_call, 1
        .endif
    .endr
    .if .Lmainui_skip_call
        mov r0, r0
        .set .Lmainui_patched_calls, .Lmainui_patched_calls + 1
    .else
        .inst 0xebfffffe
        .reloc . - 4, R_ARM_CALL, \target
    .endif
.endm
)");
// 0x37784: builds the list for a window id: the main menu, the games and RetroArch icons,
// favorites, apps, file browser, settings (and the fixes submenu), device info, hardware test,
// Recent, language, color and theme lists, or the ROM list of an emulator.
ListMenu *createMenu(int id, std::string path)
{
    printf("createMenu %d\n", id);
    if (id == WIN_MAIN) {
        IconMenu *menu = new IconMenu(1, 4, false, true, 10, 0, true);
        if (g_recentGames.size() != 0) menu->addItem(new IconMenuItem(STR_RECENT, "skin/ic-recent-f.png", "skin/ic-recent-n.png", 0, 0, WIN_RECENT, 0, false));
        menu->addItem(new IconMenuItem(STR_FAVORITE, "skin/ic-favorite-f.png", "skin/ic-favorite-n.png", 0, 0, WIN_FAVORITES, 0, false));
        menu->addItem(new IconMenuItem(STR_GAME, "skin/ic-game-f.png", "skin/ic-game-n.png", 0, 0, WIN_GAMES, 0, false));
        // MainUI-354-clean patches the following constructor and append calls to NOPs.
        menu->addItem(new IconMenuItem(STR_RETROARCH, "skin/ic-retroarch-f.png", "skin/ic-retroarch-n.png", 0, 0, WIN_RETROARCH, 0, false));
        menu->addItem(new IconMenuItem(STR_APP, "skin/ic-app-f.png", "skin/ic-app-n.png", 0, 0, WIN_APPS, 0, false));
        menu->addItem(new IconMenuItem(STR_SETTING, "skin/ic-setting-f.png", "skin/ic-setting-n.png", 0, 0, WIN_SETTING, 0, false));
        return menu;
    } else if (id == WIN_GAMES) {
        IconMenu *menu = new IconMenu(2, 4, false, false, 10, 10, false);
        menu->setLabelOffsetY(45);
        for (int i = 0; i < g_emuFolders.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_emuFolders[i].label.c_str(), g_emuFolders[i].iconSelected.c_str(), g_emuFolders[i].icon.c_str(), g_gameFocused, g_gameNormal, WIN_ROM_LIST, &g_emuFolders[i], false);
            item->verboseLabel = g_emuFolders[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == WIN_RETROARCH) {
        IconMenu *menu = new IconMenu(3, 3, true, false, 0, 0, false);
        menu->setIconCropSize(192, 72);
        menu->setFontFn(smallMenuFont);
        menu->setLabelOffsetY(28);
        for (int i = 0; i < g_rappFolders.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_rappFolders[i].label.c_str(), g_rappFolders[i].iconSelected.c_str(), g_rappFolders[i].icon.c_str(), g_raRow, 0, WIN_RETROARCH_LIST, &g_rappFolders[i], false);
            item->verboseLabel = g_rappFolders[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == WIN_FAVORITES) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildFavoriteList("/mnt/SDCARD/Roms/favourite.json");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_APPS) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildAppList(&g_appFolder);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_FILES) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        FileBrowserMenu *menu = NULL;
        if (dualPartition() != true)
            menu = new FileBrowserMenu("/mnt/SDCARD");
        else
            menu = new FileBrowserMenu("/mnt/P2EXT");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_SETTING) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_SETTING menu %p\n", menu);
        menu->addItem((TextItem *)new SettingMenuItem(STR_POWER_OFF, "skin/icon-Shutdown.png", SETTING_POWER_OFF));
        menu->addItem((TextItem *)new SettingMenuItem(STR_BRIGHTNESS, "skin/icon-brightness-48.png", SETTING_BRIGHTNESS));
        menu->addItem((TextItem *)new SettingMenuItem(STR_WIFI, "skin/icon-setting-wifi.png", SETTING_WIFI));
        menu->addItem((TextItem *)new SettingMenuItem(STR_COLOR, "skin/color.png", SETTING_COLOR));
        std::vector<LanguageChoice> *languages = languageChoices();
        if (languages->size() > 1) menu->addItem((TextItem *)new SettingMenuItem(STR_LANGUAGE, "skin/icon-language-48.png", SETTING_LANGUAGE));
        // Five pairs of calls below are disabled in the original; patch sites recorded separately.
        menu->addItem((TextItem *)new SettingMenuItem(STR_KEYS_MAP, "skin/icon-key-setting-48.png", SETTING_KEYMAP));
        menu->addItem((TextItem *)new SettingMenuItem(STR_BGM_VOLUME, "skin/sound-icon.png", SETTING_BGM_VOLUME));
        if (g_themeList.size() != 0) menu->addItem((TextItem *)new SettingMenuItem(STR_THEMES, "skin/icon-theme.png", SETTING_THEME));
        puts("insert hibernate item");
        menu->addItem((TextItem *)new SettingMenuItem(STR_HIBERNATE, "skin/icon-device-info-48.png", SETTING_HIBERNATE));
        menu->addItem((TextItem *)new SettingMenuItem(STR_FIXES, "skin/fixit.png", SETTING_FIXES));
        menu->addItem((TextItem *)new SettingMenuItem(STR_FACTORY_RESET, "skin/icon-factory-reset-48.png", SETTING_FACTORY_RESET));
        menu->addItem((TextItem *)new SettingMenuItem(STR_DEVICE_INFO, "skin/icon-device-info-48.png", SETTING_DEVICE_INFO));
        menu->addItem((TextItem *)new SettingMenuItem(STR_HARDWARE_TEST, "skin/icon-device-info-48.png", SETTING_IO_TEST));
        return menu;
    } else if (id == WIN_FIXES) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_FIXES menu %p\n", menu);
        menu->addItem((TextItem *)new SettingMenuItem(STR_AUDIO_FIX, NULL, SETTING_AUDIOFIX));
        return menu;
    } else if (id == WIN_DEVICE_INFO) {
        DeviceInfoMenu *menu = new DeviceInfoMenu();
        return menu;
    } else if (id == WIN_HARDWARE_TEST) {
        HardwareTestMenu *menu = new HardwareTestMenu();
        return menu;
    } else if (id == WIN_RECENT) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildRecentList(&g_recentGames);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else {
        if (id == WIN_LANGUAGE) {
            std::vector<LanguageChoice> *languages = languageChoices();
            if (languages->size() > 1) {
                LanguageMenu *menu = new LanguageMenu(*languages);
                return menu;
            }
        } else {
            if (id == WIN_COLOR) {
                ColorMenu *menu = new ColorMenu();
                return menu;
            }
            if (id == WIN_THEME) {
                ThemeMenu *menu = new ThemeMenu();
                return menu;
            }
        }
        return NULL;
    }
}

asm(R"(
.purgem bl
.if .Lmainui_patched_calls != 12
    .error "createMenu must retain all twelve MainUI-354-clean patched calls"
.endif
)");
// 0x38834: make sure there is swap space (needs the SD card).
int enableSwapAction(void *arg)
{
    if (sdCardMounted()) enableSwap();
    return 1;
}
// 0x38868: show the "LOADING" animation on a thread during start-up.
void startBackgroundThread()
{
    g_busy = 1;
    g_loadingThread = SDL_CreateThread(loadingScreenThread, NULL);
}
// 0x388a8: stop it.
void stopBackgroundThread()
{
    g_busy = 0;
    SDL_WaitThread(g_loadingThread, NULL);
    g_loadingThread = NULL;
}
// 0x388f0: at start-up: enable the swap file if no swap is active.
void ensureSwap(void *arg)
{
    int elapsed = 0;
    timeval now;
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec * 1000 + now.tv_usec / 1000;
    sync();
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec * 1000 + now.tv_usec / 1000 - elapsed;
    int size = swapSize();
    printf("swap size is %d take %dms\n", size, elapsed);
    if (size < 1) enableSwapAction(NULL);
}
// 0x389d4: charging screen thread: the battery picture (five levels, animated upwards from the
// current one) and "Charging..."; powers off if the charger is unplugged.
static int chargingThread(void *arg)
{
    SDL_FillRect(g_backBuffer, NULL, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
    int frame = 0;
    int current = 0;
    int previous = 0;
    int count = 5;
    int battery;
    int voltage;
    int state;
    SDL_Rect imageRect;
    SDL_Rect textRect;
    SDL_Surface *images[5];
    images[0] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-01.png", g_backBuffer, false);
    images[1] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-02.png", g_backBuffer, false);
    images[2] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-03.png", g_backBuffer, false);
    images[3] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-04.png", g_backBuffer, false);
    images[4] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-05.png", g_backBuffer, false);
    imageRect.x = 224;
    imageRect.y = 192;
    imageRect.w = 192;
    imageRect.h = 96;
    textRect.w = 640;
    textRect.h = TTF_FontHeight(g_iconFont);
    textRect.x = 0;
    textRect.y = imageRect.y + 20;
    while (g_charging) {
        readAxpStatus(&battery, &voltage, &state);
        if (state != 2 && state != 3) {
            puts("usb unplug, poweroff");
            system("sync");
            system(g_powerOffCommand);
        }
        if (battery >= 99)
            current = 4;
        else if (battery >= 75)
            current = 3;
        else if (battery >= 50)
            current = 2;
        else if (battery >= 25)
            current = 1;
        else
            current = 0;
        if (previous != current) {
            previous = current;
            frame = current;
        }
        if (images[frame % 5]) {
            SDL_FillRect(g_backBuffer, &imageRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            imageRect.w = images[frame % 5]->w;
            imageRect.h = images[frame % 5]->h;
            imageRect.x = (640 - imageRect.w) / 2;
            imageRect.y = (480 - imageRect.h) / 2;
            SDL_UpperBlit(images[frame % 5], NULL, g_backBuffer, &imageRect);
            textRect.y = imageRect.y + imageRect.h + 20;
            SDL_FillRect(g_backBuffer, &textRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            drawText(g_backBuffer, g_iconFont, textRect, "Charging...", g_wifiEmptyColor, 1, false);
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_UpdateRect(g_screen, 0, 0, 0, 0);
        }
        usleep(150000);
        frame++;
        if (frame > 4) frame = current;
    }
    SDL_FreeSurface(images[0]);
    SDL_FreeSurface(images[1]);
    SDL_FreeSurface(images[2]);
    SDL_FreeSurface(images[3]);
    SDL_FreeSurface(images[4]);
    return 0;
}
// 0x38ff8: the charging screen, shown instead of the menus when the device was started by plugging
// in a charger. Holding power for over half a second reboots into the normal menus.
void chargingScreenLoop()
{
    g_charging = true;
    SDL_Thread *thread = SDL_CreateThread(chargingThread, NULL);
    g_powerSent = false;
    system("touch /tmp/battery_charging");
    SDL_Event event;
    while (true) {
        if (SDL_WaitEvent(&event) != 0) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case MIYOO_KEY_POWER:
                            g_powerDownCount++;
                            printf("SDLK_POWER down %d, sent %d\n", g_powerDownCount, g_powerSent);
                            if ((g_powerDownCount - 1) * 100 > 500 && g_powerSent != true) {
                                g_powerSent = true;
                                system("sync");
                                system("reboot");
                            }
                            break;
                        default:
                            printf("event.key.keysym.sym = %d\n", event.key.keysym.sym);
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case MIYOO_KEY_POWER:
                            g_powerDownCount = 0;
                            g_powerSent = false;
                            break;
                        default: break;
                    }
                default: break;
            }
        }
    }
}
// The compiler emits the original static-init pair here (0x3919c and 0x39228).
