// p11 0x36838-0x39240: ADC information, window loop, menu factory and charging.
#include <fcntl.h>
#include "../HardwareTestMenu.h"
extern "C" {
    int SDL_PollEvent(void *);
    void SDL_PumpEvents();
    int SDL_PeepEvents(void *, int, int, unsigned);
    void SDL_UpdateRect(SDL_Surface *, int, int, unsigned, unsigned);
}

extern std::vector<TrimusWindow *> g_pendingWindows;
extern int g_emuIconX, g_emuIconY, g_rappIconX;
// 0x1821c0, 0x1821c4, 0x1821d0
extern SDL_Thread *g_loadingThread;
// 0x180854
extern unsigned char g_charging;
// 0x181914
extern TTF_Font *g_chargingFont;
// 0x182180
bool sdCardMounted();
int enableSwap();
bool dualPartition();
// 0x36838
ADCInfoWindow::ADCInfoWindow(int title, int id) : TrimusWindow(title, id) {
    timer = SDL_AddTimer(500, detailTimeout, NULL);
}
// 0x368b8 (D0 0x36914)
ADCInfoWindow::~ADCInfoWindow() {
    if (timer) SDL_RemoveTimer(timer);
}
// 0x36948
int ADCInfoWindow::handleInput() {
    int key = handleGlobalInput(NULL);
    switch (key) {
    case 2: return 1;
    default: break;
    }
    return -1;
}
// 0x36990
int ADCInfoWindow::draw(SDL_Surface *dst) {
    if (titleLabelId != 157) drawScreenFrame(dst, languageString(titleLabelId), id, true);
    else drawScreenFrame(dst, "MIYOO", id, true);
    int status = 2;
    const char *label = NULL;
    switch (status) {
    case 0: label = languageString(40);
        break;
    case 1: label = languageString(41);
        break;
    case 2: label = languageString(42);
        break;
    case 3: label = languageString(43);
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
    snprintf(g_launchCmd, 0x800, "%s     %d\n%s  %s\n", languageString(39), battery, languageString(33), label);
    drawTextLines(dst, g_imeFont, &rect, g_launchCmd, "\n", &g_imeTextColor, 1, false);
    return 0;
}
// 0x36b68
void clearWindows(std::vector<TrimusWindow *> *windows) {
    printf("%ld window to free\n", windows->size());
    while (windows->size() != 0) {
        TrimusWindow *window = windows->back();
        windows->pop_back();
        window->onHide();
        delete window;
    }
}
// 0x36c20
void flipScreen() {
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    flipDisplay();
}
// 0x36c58
void updateScreen() {
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_UpdateRect(g_screen, 0, 0, 0, 0);
}
// 0x36cb8
void flushPendingWindows() {
    for (int i = 0; i < g_pendingWindows.size(); i++) {
        g_pendingWindows[i]->onHide();
        delete g_pendingWindows[i];
    }
    g_pendingWindows.clear();
}
// 0x36d88
void pushWindow(TrimusWindow *window) {
    flushPendingWindows();
    g_pendingWindows.push_back(window);
}
// 0x36dbc
void initFramebuffer() {
    g_fbFd = open("/dev/fb0", 2);
    if (g_fbFd >= 0) {
        if (ioctl(g_fbFd, 0x4600, &g_fbVar) == -1) {
            close(g_fbFd);
            g_fbFd = -1;
            puts("cat't ioctl /dev/fb0 ");
        } else {
            g_fbVar.yoffset = 0;
            ioctl(g_fbFd, 0x4606, &g_fbVar);
        }
    } else puts("cat't open /dev/fb0 ");
}
// 0x36eb0
void closeFramebuffer() {
    if (g_fbFd >= 0) close(g_fbFd);
}
// 0x36ee8
void runWindowLoop(std::vector<TrimusWindow *> *windows, std::atomic<bool> *running) {
    int result;
    int slot;
    int count;
    TrimusWindow *window;
    int windowId;
    int next;
    MainEvent event;
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
        if (windowId != 25 && windowId != -1) {
            slot = drawBatteryIndicator(g_backBuffer, true);
            next = drawSoundCardIndicator(g_backBuffer, true);
            if (next >= 0) slot = next;
            next = drawWifiIndicator(g_backBuffer, slot, true);
            if (next >= 0) slot = next;
            if (g_statusIndicatorFlag) drawKeyStateIndicator(g_backBuffer, slot, true);
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
                    if ((result == 1 && windows->size() > 1) || result == 4) {
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
                    if (result == 3) {
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
            }
            while (SDL_PeepEvents(&event, 1, 1, ~0u) > 0);
            if (result == 3 || result == 2) break;
        }
    }
    if (result == 1) unlink("/tmp/cmd_to_run.sh");
    saveWindowState(windows, "/tmp/state.json");
    saveRomListPositions(&g_romListPositions);
    clearWindows(windows);
}
// 0x373c4
void createRootWindow(std::vector<TrimusWindow *> *windows) {
    ListMenu *menu = createMenu(0, "");
    MenuWindow *window = new MenuWindow(157, 0, menu);
    window->onShow();
    windows->push_back((TrimusWindow*)window);
}
// 0x374bc
const char *files_374bc(int id) {
    switch (id) {
    case 1: return languageString(2);
    case 2: return languageString(1);
    case 3: return languageString(107);
    case 4: return languageString(19);
    case 6: return languageString(14);
    case 7: return languageString(15);
    case 8: return languageString(16);
    case 9: return languageString(17);
    case 12: return languageString(20);
    case 13: return languageString(21);
    case 10: return languageString(18);
    case 24: return languageString(153);
    default: return NULL;
    }
}
// 0x37610
ListMenu *createRomListMenu(EmuInfo *emu, int id, std::string *dir, std::string *search) {
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
        ListMenu *menu = buildRomList_23318(emu, id, dir, search);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else return NULL;
}
// 0x37760: absent from the original Ghidra function export.
static TTF_Font *smallMenuFont() {
    return g_smallFont;
}
// The target binary replaces these constructor/append calls after compilation.
// Keep GCC's argument setup and exception cleanup, but emit the original mov r0,r0
// at the twelve patched BL sites. Other calls retain the ordinary R_ARM_CALL relocation.
// This assembler macro is scoped to createMenu; the byte checker therefore checks
// the NOPs themselves instead of masking them as unresolved calls.
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
// 0x37784
ListMenu *createMenu(int id, std::string path) {
    printf("createMenu %d\n", id);
    if (id == 0) {
        IconMenu *menu = new IconMenu(1, 4, false, true, 10, 0, true);
        if (g_recentGames.size() != 0) menu->addItem(new IconMenuItem(18, "skin/ic-recent-f.png", "skin/ic-recent-n.png", 0, 0, 10, 0, false));
        menu->addItem(new IconMenuItem(1, "skin/ic-favorite-f.png", "skin/ic-favorite-n.png", 0, 0, 2, 0, false));
        menu->addItem(new IconMenuItem(2, "skin/ic-game-f.png", "skin/ic-game-n.png", 0, 0, 1, 0, false));
        // MainUI-354-clean patches the following constructor and append calls to NOPs.
        menu->addItem(new IconMenuItem(0, "skin/ic-retroarch-f.png", "skin/ic-retroarch-n.png", 0, 0, 16, 0, false));
        menu->addItem(new IconMenuItem(107, "skin/ic-app-f.png", "skin/ic-app-n.png", 0, 0, 3, 0, false));
        menu->addItem(new IconMenuItem(15, "skin/ic-setting-f.png", "skin/ic-setting-n.png", 0, 0, 7, 0, false));
        return menu;
    } else if (id == 1) {
        IconMenu *menu = new IconMenu(2, 4, false, false, 10, 10, false);
        menu->setField74(45);
        for (int i = 0; i < g_romSourcesA.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_romSourcesA[i].label.c_str(), g_romSourcesA[i].iconSelected.c_str(), g_romSourcesA[i].icon.c_str(), g_emuIconX, g_emuIconY, 5, &g_romSourcesA[i], false);
            item->str28 = g_romSourcesA[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == 16) {
        IconMenu *menu = new IconMenu(3, 3, true, false, 0, 0, false);
        menu->setField68(192, 72);
        menu->setFontFn(smallMenuFont);
        menu->setField74(28);
        for (int i = 0; i < g_romSourcesB.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_romSourcesB[i].label.c_str(), g_romSourcesB[i].iconSelected.c_str(), g_romSourcesB[i].icon.c_str(), g_rappIconX, 0, 17, &g_romSourcesB[i], false);
            item->str28 = g_romSourcesB[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == 2) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = files_374bc(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildMenu_1a924("/mnt/SDCARD/Roms/favourite.json");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == 3) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildMenu_1bbfc(&g_appFolder);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == 4) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = files_374bc(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        FileBrowserMenu *menu = NULL;
        if (dualPartition() != true) menu = new FileBrowserMenu("/mnt/SDCARD");
        else menu = new FileBrowserMenu("/mnt/P2EXT");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == 7) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_SETTING menu %p\n", menu);
        menu->addItem((TextItem*)new SettingMenuItem(86, "skin/icon-Shutdown.png", 18));
        menu->addItem((TextItem*)new SettingMenuItem(26, "skin/icon-brightness-48.png", 3));
        menu->addItem((TextItem*)new SettingMenuItem(131, "skin/icon-setting-wifi.png", 19));
        menu->addItem((TextItem*)new SettingMenuItem(118, "skin/color.png", 13));
        std::vector<LanguageChoice> *languages = languageChoices();
        if (languages->size() > 1) menu->addItem((TextItem*)new SettingMenuItem(23, "skin/icon-language-48.png", 11));
        // Five pairs of calls below are disabled in the original; patch sites recorded separately.
        menu->addItem((TextItem*)new SettingMenuItem(22, "skin/icon-key-setting-48.png", 0));
        menu->addItem((TextItem*)new SettingMenuItem(25, "skin/sound-icon.png", 2));
        if (g_themeList.size() != 0) menu->addItem((TextItem*)new SettingMenuItem(125, "skin/icon-theme.png", 15));
        puts("insert hibernate item");
        menu->addItem((TextItem*)new SettingMenuItem(115, "skin/icon-device-info-48.png", 12));
        menu->addItem((TextItem*)new SettingMenuItem(130, "skin/fixit.png", 16));
        menu->addItem((TextItem*)new SettingMenuItem(28, "skin/icon-factory-reset-48.png", 5));
        menu->addItem((TextItem*)new SettingMenuItem(30, "skin/icon-device-info-48.png", 7));
        menu->addItem((TextItem*)new SettingMenuItem(31, "skin/icon-device-info-48.png", 8));
        return menu;
    } else if (id == 20) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_FIXES menu %p\n", menu);
        menu->addItem((TextItem*)new SettingMenuItem(129, NULL, 17));
        return menu;
    } else if (id == 9) {
        DeviceInfoMenu *menu = new DeviceInfoMenu();
        return menu;
    } else if (id == 11) {
        HardwareTestMenu *menu = new HardwareTestMenu();
        return menu;
    } else if (id == 10) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = files_374bc(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildMenu_1b488(&g_recentGames);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else {
        if (id == 14) {
            std::vector<LanguageChoice> *languages = languageChoices();
            if (languages->size() > 1) {
                LanguageMenu *menu = new LanguageMenu(*languages);
                return menu;
            }
        } else {
            if (id == 18) {
                ColorMenu *menu = new ColorMenu();
                return menu;
            }
            if (id == 19) {
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
// 0x38834
int enableSwapAction(void *arg) {
    if (sdCardMounted()) enableSwap();
    return 1;
}
// 0x38868
void startBackgroundThread() {
    g_busy = 1;
    g_loadingThread = SDL_CreateThread(loadingScreenThread, NULL);
}
// 0x388a8
void stopBackgroundThread() {
    g_busy = 0;
    SDL_WaitThread(g_loadingThread, NULL);
    g_loadingThread = NULL;
}
// 0x388f0
void ensureSwap(void *arg) {
    int elapsed = 0;
    timeval now;
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec*1000+now.tv_usec/1000;
    sync();
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec*1000+now.tv_usec/1000-elapsed;
    int size = swapSize();
    printf("swap size is %d take %dms\n", size, elapsed);
    if (size < 1) enableSwapAction(NULL);
}
// 0x389d4
static int chargingThread(void *arg) {
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
    textRect.h = TTF_FontHeight(g_chargingFont);
    textRect.x = 0;
    textRect.y = imageRect.y+20;
    while (g_charging) {
        readAxpStatus(&battery, &voltage, &state);
        if (state != 2 && state != 3) {
            puts("usb unplug, poweroff");
            system("sync");
            system("mainui_off_order");
        }
        if (battery >= 99) current = 4;
        else if (battery >= 75) current = 3;
        else if (battery >= 50) current = 2;
        else if (battery >= 25) current = 1;
        else current = 0;
        if (previous != current) {
            previous = current;
            frame = current;
        }
        if (images[frame%5]) {
            SDL_FillRect(g_backBuffer, &imageRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            imageRect.w = images[frame%5]->w;
            imageRect.h = images[frame%5]->h;
            imageRect.x = (640-imageRect.w)/2;
            imageRect.y = (480-imageRect.h)/2;
            SDL_UpperBlit(images[frame%5], NULL, g_backBuffer, &imageRect);
            textRect.y = imageRect.y+imageRect.h+20;
            SDL_FillRect(g_backBuffer, &textRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            drawText(g_backBuffer, g_chargingFont, textRect, "Charging...", g_wifiEmptyColor, 1, false);
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
// 0x38ff8
void chargingScreenLoop() {
    g_charging = true;
    SDL_Thread *thread = SDL_CreateThread(chargingThread, NULL);
    g_powerSent = false;
    system("touch /tmp/battery_charging");
    SDL_Event event;
    while (true) {
        if (SDL_WaitEvent(&event) != 0) {
            switch (event.type) {
            case 2:
                switch (event.key.keysym.sym) {
                case 320:
                    g_powerDownCount++;
                    printf("SDLK_POWER down %d, sent %d\n", g_powerDownCount, g_powerSent);
                    if ((g_powerDownCount-1)*100 > 500 && g_powerSent != true) {
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
            case 3:
                switch (event.key.keysym.sym) {
                case 320:
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
