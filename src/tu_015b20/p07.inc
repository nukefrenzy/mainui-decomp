// p07 0x2849c-0x2b788: settings controls, actions and device information.
#include "../KeySettingWindow.h"
#include "../KeyTestWindow2.h"
#include "../WifiMenus.h"
extern "C" {
    void *SDL_AddTimer(unsigned, unsigned (*)(unsigned, void*), void*);
    int SDL_RemoveTimer(void *);
    int Mix_HaltChannel(int);
    void setKeymap(const char*);
    void setLanguageFile(const char*);
    int MI_SYS_ReadUuid(unsigned long long*);
}

int removeAllDBFiles();
int removeEmulatorConfigs();
float maxCpuMHz();
int memoryMB();
std::string sdCardSpace();
// 0x2849c
SettingMenu::SettingMenu():TextMenu(true, true, 6, NULL) {
    field104 = NULL;
    surface108 = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    surface10c = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x28614
SettingMenu::~SettingMenu() {
    vfunc18();
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
    g_settingsDirty = false;
    if (field104) {
        SDL_RemoveTimer(field104);
        setVolumeTinymix(GetKeyShm(g_keyShm, 0));
    }
    if (surface108)SDL_FreeSurface(surface108);
    if (surface10c)SDL_FreeSurface(surface10c);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2877c
void SettingMenu::vfunc11() {
    if (themePath.compare(g_themePath) != 0) {
        for (int i = 0;i<items.size();i++) {
            SettingMenuItem *item = (SettingMenuItem*)items[i];
            item->reload();
        }
        if (surface108)SDL_FreeSurface(surface108);
        if (surface10c)SDL_FreeSurface(surface10c);
        if (selectedBackground)SDL_FreeSurface(selectedBackground);
        surface108 = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
        surface10c = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x28994
void SettingMenu::vfunc7(SDL_Surface *dst) {
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0
    };
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0)pageEnd = 5;
    if (cur<pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur>pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd-5;
    }
    if (pageStart<0)pageStart = 0;
    if ((unsigned)pageEnd >= items.size()-1)pageEnd = items.size()-1;
    int i = pageStart;
    int row = 0;
    for (;i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem*)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row*60;
        if (row<5 && g_listStripe) {
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
            area.x = area.x + 60;
        }
        area.y = y;
        area.w = rect.w-(surface108?surface108->w:0)-(surface10c?surface10c->w:0)-20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, flagC0?i == cur:false);
        area.x = rect.w + x-(surface10c?surface10c->w:0)-240;
        area.w = 200;
        int type = item->handler;
        int mode;
        WifiStatus *status;
        int level, timeout;
        if (type == 1 || type == 3 || type == 2 || type == 12 || type == 14 || type == 17) {
            if (type == 1) {
                level = GetKeyShm(g_keyShm, 0);
                snprintf(text, 32, "%02d/%02d", level, 20);
            } else if (type == 3) {
                level = GetKeyShm(g_keyShm, 1);
                snprintf(text, 32, "%02d/%02d", level, 10);
            } else if (type == 2)snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, 5), 20);
            else if (type == 12) {
                timeout = GetKeyShm(g_keyShm, 6);
                if (timeout<5 || timeout>30)strncpy(text, languageString(114), 32);
                else snprintf(text, 32, languageString(113), timeout);
            } else if (type == 17) {
                timeout = GetKeyShm(g_keyShm, 13);
                if (timeout != 0)strncpy(text, languageString(104), 32);
                else strncpy(text, languageString(105), 32);
            }
            area.y += 2;
            drawText(dst, g_imeFont, area, text, color, 1, false);
            if (surface108) {
                area.y = (60-surface108->h)/2 + y;
                area.x = rect.w + x-(surface10c?surface10c->w:0)-surface108->w-240;
                SDL_UpperBlit(surface108, NULL, dst, &area);
            }
            if (surface10c) {
                area.y = (60-surface10c->h)/2 + y;
                area.x = rect.w + x-surface10c->w-40;
                SDL_UpperBlit(surface10c, NULL, dst, &area);
            }
        } else if (type == 19) {
            mode = wifiMode();
            if (mode == 2) {
                status = getWifiStatus();
                if (status->state == 4) {
                    std::string label(status->ip);
                    if (label.size() != 0) {
                        area.x = rect.w + x-(surface10c?surface10c->w:0)-240;
                        area.w = surface10c?surface10c->w + 200:200;
                        area.y += 2;
                        drawText(dst, g_imeFont, area, label.c_str(), color, 4, false);
                    }
                }
            }
        }
        i++;
        row++;
    }
}

// 0x294f8
static unsigned previewVolume(unsigned interval, void *param) {
    if (g_enterSound) {
        puts("Mix_PlayChannel enter.wav");
        if (Mix_PlayChannel(0, g_enterSound, 0) == -1)printf("Mix_PlayChunk: %s\n", SDL_GetError());
    }
    setVolumeTinymix(GetKeyShm(g_keyShm, 0));
    return 0;
}

// 0x295a8
void SettingMenu::decrease() {
    SettingMenuItem *item = (SettingMenuItem*)items[cur];
    if (item == NULL)return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case 1:level = GetKeyShm(g_keyShm, 0);
        if (level>0)level--;
        SetKeyShm(g_keyShm, 0, level);
        SetKeyShm(g_keyShm, 3, 0);
        if (field104) {
            SDL_RemoveTimer(field104);
            Mix_HaltChannel(0);
        }
        field104 = SDL_AddTimer(150, previewVolume, NULL);
        break;
        case 3:level = GetKeyShm(g_keyShm, 1);
        if (level>1)level--;
        SetKeyShm(g_keyShm, 1, level);
        setBacklight(level);
        break;
        case 2:level = GetKeyShm(g_keyShm, 5);
        if (level>0)level--;
        SetKeyShm(g_keyShm, 5, level);
        Mix_VolumeMusic((level<<7)/20);
        Mix_Volume(1, (level<<7)/20);
        break;
        case 12:level = GetKeyShm(g_keyShm, 6);
        if (level == 0)level = 5;
        else if (level == 5)level = 15;
        else if (level == 15)level = 30;
        else level = 0;
        SetKeyShm(g_keyShm, 6, level);
        break;
        case 17:level = GetKeyShm(g_keyShm, 13);
        if (level == 0) {
            level = 1;
            system("rm -f /tmp/no_audioserver");
        } else {
            level = 0;
            system("touch /tmp/no_audioserver");
        }
        SetKeyShm(g_keyShm, 13, level);
        break;
        default:break;
    }
}

// 0x298ec
void SettingMenu::increase() {
    SettingMenuItem *item = (SettingMenuItem*)items[cur];
    if (item == NULL)return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case 1:level = GetKeyShm(g_keyShm, 0);
        if (level<20)level++;
        SetKeyShm(g_keyShm, 0, level);
        SetKeyShm(g_keyShm, 3, 0);
        if (field104) {
            SDL_RemoveTimer(field104);
            Mix_HaltChannel(0);
        }
        field104 = SDL_AddTimer(150, previewVolume, NULL);
        break;
        case 3:level = GetKeyShm(g_keyShm, 1);
        if (level<10)level++;
        SetKeyShm(g_keyShm, 1, level);
        setBacklight(level);
        break;
        case 2:level = GetKeyShm(g_keyShm, 5);
        if (level<20)level++;
        SetKeyShm(g_keyShm, 5, level);
        Mix_VolumeMusic((level<<7)/20);
        Mix_Volume(1, (level<<7)/20);
        break;
        case 12:level = GetKeyShm(g_keyShm, 6);
        if (level == 0)level = 5;
        else if (level == 5)level = 15;
        else if (level == 15)level = 30;
        else level = 0;
        SetKeyShm(g_keyShm, 6, level);
        break;
        case 17:level = GetKeyShm(g_keyShm, 13);
        if (level == 0) {
            level = 1;
            system("rm -f /tmp/no_audioserver");
        } else {
            level = 0;
            system("touch /tmp/no_audioserver");
        }
        SetKeyShm(g_keyShm, 13, level);
        break;
        default:break;
    }
}

// 0x29c30
static void rebootToEfex() {
    system("reboot efex");
}

// 0x29c4c
static int resetEmulatorSettings(void *arg) {
    removeAllDBFiles();
    removeEmulatorConfigs();
    return 1;
}

// 0x29c74
static int resetAllSettings(void *arg) {
    resetEmulatorSettings(NULL);
    unlink("/mnt/SDCARD/Roms/favourite.json");
    unlink("/mnt/SDCARD/system.json");
    unlink("/appconfigs/romwinidx.json");
    g_romListPositions.clear();
    SetKeyShm(g_keyShm, 0, 20);
    SetKeyShm(g_keyShm, 5, 8);
    SetKeyShm(g_keyShm, 3, 0);
    SetKeyShm(g_keyShm, 1, 7);
    SetKeyShm(g_keyShm, 8, 7);
    SetKeyShm(g_keyShm, 9, 10);
    SetKeyShm(g_keyShm, 10, 10);
    SetKeyShm(g_keyShm, 11, 10);
    setVolumeTinymix(GetKeyShm(g_keyShm, 0));
    display_getlcd_1bea4(7, 10, 10, 10);
    clearRecent(&g_recentGames);
    setKeymap(NULL);
    setLanguageFile(NULL);
    sync();
    initAudioSettings(g_keyShm);
    return 1;
}

// 0x29dc8
void SettingMenu::select() {
    SettingMenuItem *item = (SettingMenuItem*)items[cur];
    if (item == NULL)return;
    g_settingsDirty = true;
    int type = item->handler;
    switch (type) {
        case 7: {
            MenuWindow *window = new MenuWindow(30, 9, (void*)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 16: {
            MenuWindow *window = new MenuWindow(130, 20, (void*)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 8: {
            KeyTestWindow2 *window = new KeyTestWindow2;
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 9: {
            ADCInfoWindow *window = new ADCInfoWindow(32, 12);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 4: {
            Action *action = new GeneralAction(-1, resetEmulatorSettings, NULL);
            Action *progress = new PopupProgressAction(53, action);
            DialogWindow *window = new DialogWindow(54, 69, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 5: {
            Action *action = new GeneralAction(-1, resetAllSettings, NULL);
            Action *progress = new PopupProgressAction(70, action);
            DialogWindow *window = new DialogWindow(67, 68, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 11: {
            MenuWindow *window = new MenuWindow(23, 14, (void*)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 0: {
            KeySettingWindow *window = new KeySettingWindow;
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 13: {
            MenuWindow *window = new MenuWindow(118, 18, (void*)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 15: {
            MenuWindow *window = new MenuWindow(125, 19, (void*)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 18: {
            DialogWindow *window = new DialogWindow(86, 108, false, new GeneralAction(-1, powerOff, NULL));
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 19: {
            WifiMenu *menu = new WifiMenu;
            MenuWindow *window = new MenuWindow(131, 21, (ListMenu*)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        case 20: {
            HotspotMenu *menu = new HotspotMenu;
            MenuWindow *window = new MenuWindow(134, 23, (ListMenu*)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow*)window);
            return;
        }
        default:return;
    }
}

// 0x2a5d4
int SettingMenu::vfunc3() {
    int key = handleGlobalInput(NULL);
    switch (key) {
        case 5:puts("key_up");
        vfunc10();
        break;
        case 6:puts("key_down");
        vfunc9();
        break;
        case 7:decrease();
        break;
        case 8:increase();
        break;
        case 1:case 4:puts("a");
        select();
        break;
        case 2:case 3:puts("b or c");
        return 1;
        case 11:return 11;
        default:break;
    }
    return -1;
}

// 0x2a6ec
void SettingMenu::vfunc18() {
    puts("SettingMenu::freeListItem");
    for (int i = 0;i<items.size();i++) {
        SettingMenuItem *item = (SettingMenuItem*)items[i];
        delete item;
    }
    items.clear();
}

struct DeviceInfoItem {
    DeviceInfoItem(const char *, const char *);
    ~DeviceInfoItem() {
    }
    std::string label, value;
};
// 0x2a7b0
DeviceInfoItem::DeviceInfoItem(const char *a, const char *b) {
    label = a;
    value = b;
}

// 0x2a83c
DeviceInfoMenu::DeviceInfoMenu():TextMenu(true, true, 6, NULL) {
    field104 = NULL;
    loadInfo();
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x2a934
DeviceInfoMenu::~DeviceInfoMenu() {
    vfunc18();
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
    if (field104)free(field104);
}

// 0x2aa08
void DeviceInfoMenu::vfunc11() {
    if (themePath.compare(g_themePath) != 0) {
        if (selectedBackground)SDL_FreeSurface(selectedBackground);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x2aae4
void DeviceInfoMenu::vfunc7(SDL_Surface *dst) {
    TTF_Font *font = g_imeFont;
    int height = TTF_FontHeight(font);
    SDL_Rect area;
    SDL_Color color;
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0)pageEnd = 5;
    if (cur<pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur>pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd-5;
    }
    if (pageStart<0)pageStart = 0;
    if ((unsigned)pageEnd >= items.size()-1)pageEnd = items.size()-1;
    int i = 0;
    while (i<5 && g_listStripe) {
        dest.x = 0;
        dest.y = (i + 2)*60;
        SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        i++;
    }
    i = pageStart;
    int row = 0;
    for (;i <= pageEnd;) {
        DeviceInfoItem *item = (DeviceInfoItem*)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row*60;
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
        area.y = (60-height)/2 + y;
        area.w = rect.w-40;
        area.h = 60;
        int width = drawText(dst, font, area, item->label.c_str(), color, 0, flagC0?i == cur:false);
        area.x = area.x + width;
        area.w = rect.w-area.x-20;
        drawText(dst, font, area, item->value.c_str(), color, 2, false);
        i++;
        row++;
    }
}

// 0x2b028
static void formatDeviceUuid(char *out, size_t size) {
    int rc = 0xa009201f;
    struct Words {
        unsigned char data[8];
    };
    Words words;
    memset(&words, 0, 8);
    union {
        unsigned long long value;
        Words parts;
    }
    uuid;
    uuid.value = 0;
    rc = MI_SYS_ReadUuid(&uuid.value);
    words = uuid.parts;
    if (rc == 0)snprintf(out, size, "%02X%02X%02X%02X%02X%02X%02X%02X", words.data[7], words.data[6], words.data[5], words.data[4], words.data[3], words.data[2], words.data[1], words.data[0]);
}

// 0x2b0f8
void DeviceInfoMenu::loadInfo() {
    char uuid[32] = {
        0
    };
    char buf[16];
    formatDeviceUuid(uuid, 32);
    char *version = NULL;
    if (field104) {
        free(field104);
        field104 = NULL;
    }
    system("/etc/fw_printenv miyoo_version > /tmp/miyoo_version");
    FILE *f = fopen("/tmp/miyoo_version", "r");
    if (f) {
        puts("loadContent");
        readWholeFile("/tmp/miyoo_version", f, &field104);
        puts("loadContent end");
        fclose(f);
        version = strstr(field104, "\n");
        if (version)*version = 0;
        version = strstr(field104, "miyoo_version=");
        if (version)version += 14;
    }
    addItem((TextItem*)new DeviceInfoItem(languageString(59), "MY354"));
    addItem((TextItem*)new DeviceInfoItem(languageString(60), uuid));
    addItem((TextItem*)new DeviceInfoItem(languageString(61), version?version:"202110071805"));
    snprintf(buf, 16, "%dMHz", (int)maxCpuMHz());
    addItem((TextItem*)new DeviceInfoItem(languageString(62), buf));
    snprintf(buf, 16, "%dM", memoryMB());
    addItem((TextItem*)new DeviceInfoItem(languageString(64), buf));
    addItem((TextItem*)new DeviceInfoItem(languageString(66), sdCardSpace().c_str()));
    addItem((TextItem*)new DeviceInfoItem(languageString(155), "www.lomiyoo.com"));
}

// 0x2b504
void DeviceInfoMenu::vfunc18() {
    for (int i = 0;i<items.size();i++) {
        DeviceInfoItem *item = (DeviceInfoItem*)items[i];
        delete item;
    }
    items.clear();
}

struct PanelInfo {
    unsigned char raw[84];
    unsigned short timing[16];
    unsigned short minDclk, spreadStep, spreadSpan;
    unsigned char pwmPeriodLow, pwmPeriodHigh, pwmDuty, deinterlace;
    int aspect;
    unsigned short lvdsSwap;
    int tiBitMode, outputFormat;
    unsigned char swapOddRg, swapEvenRg, swapOddGb, swapEvenGb, doubleClock;
    unsigned maxSet, minSet;
    int outputTiming;
    unsigned char noiseDither;
    int channels[5];
};
static_assert(sizeof(PanelInfo) == 188, "MI_PANEL_ParamConfig_t storage");
// SDK-compatible full object; timing[] retains the prefix used by adjustColor.
PanelInfo g_panelInfo = {
    {
        75, 49, 48, 49, 95, 73, 77, 50, 66, 86, 76, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 11, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1,
        1, 1, 1, 0, 30, 0, 144, 1, 80, 0, 30, 0,
    },
    {20, 22, 5, 13, 40, 18, 640, 480, 0, 780, 0, 0, 515, 0, 0, 26},
    0, 0, 0, 160, 255, 80, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 1868942, 1633113, 1, 0, {2, 4, 3, 1, 0}
}; // 0x17e48c

// 0x17e548: MIPI command stream, including delays and the terminating marker.
unsigned char g_panelCommands[] = {
    0x01, 0x01, 0x00, // command, length, payload
    0xfe, 0xfe, 0x50, // delay (ms)
    0xb9, 0x03, 0xf1, 0x12, 0x83, // command, length, payload
    0xb1, 0x05, 0x00, 0x00, 0x00, 0xda, 0x80, // command, length, payload
    0xb2, 0x03, 0x00, 0x13, 0x70, // command, length, payload
    0xb3, 0x0a, 0x10, 0x10, 0x28, 0x28, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00, // command, length, payload
    0xb4, 0x01, 0x80, // command, length, payload
    0xb5, 0x02, 0x0a, 0x0a, // command, length, payload
    0xb6, 0x02, 0x87, 0x87, // command, length, payload
    0xb8, 0x04, 0x26, 0x22, 0xf0, 0x63, // command, length, payload
    0xba, 0x1b, 0x31, 0x81, 0x05, 0xf9, 0x0e, 0x0e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x91, 0x0a, 0x00, 0x00, 0x02, 0x4f, 0xd1, 0x00, 0x00, 0x37, // command, length, payload
    0xbc, 0x01, 0x47, // command, length, payload
    0xbf, 0x03, 0x02, 0x11, 0x00, // command, length, payload
    0xc0, 0x09, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x12, 0x50, 0x00, // command, length, payload
    0xc1, 0x0c, 0x53, 0x00, 0x32, 0x32, 0x77, 0xd1, 0xcc, 0xcc, 0x77, 0x77, 0x33, 0x33, // command, length, payload
    0xc6, 0x06, 0x82, 0x00, 0xbf, 0xff, 0x00, 0xff, // command, length, payload
    0xc7, 0x06, 0xb8, 0x00, 0x0a, 0x00, 0x00, 0x00, // command, length, payload
    0xc8, 0x04, 0x10, 0x40, 0x1e, 0x02, // command, length, payload
    0xcc, 0x01, 0x0b, // command, length, payload
    0x36, 0x01, 0xc0, // command, length, payload
    0xe0, 0x22, 0x00, 0x06, 0x0a, 0x2a, 0x3d, 0x3f, 0x3b, 0x37, 0x06, 0x0b, 0x0c, 0x10, 0x11, 0x10, 0x13, 0x12, 0x18, 0x00, 0x06, 0x0a, 0x2a, 0x3d, 0x3f, 0x3b, 0x37, 0x06, 0x0b, 0x0c, 0x10, 0x11, 0x10, 0x13, 0x12, 0x18, // command, length, payload
    0xe3, 0x0e, 0x07, 0x07, 0x0b, 0x0b, 0x0b, 0x0b, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xc0, 0x10, // command, length, payload
    0xe9, 0x3f, 0xc8, 0x10, 0x02, 0x00, 0x00, 0xb0, 0xb1, 0x11, 0x31, 0x23, 0x28, 0x80, 0xb0, 0xb1, 0x27, 0x08, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x88, 0x88, 0xba, 0x60, 0x24, 0x08, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xba, 0x71, 0x35, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // command, length, payload
    0xea, 0x3d, 0x97, 0x0a, 0x82, 0x02, 0x03, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x88, 0xba, 0x17, 0x53, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x80, 0x88, 0xba, 0x06, 0x42, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x23, 0x00, 0x00, 0x01, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // command, length, payload
    0xef, 0x03, 0xff, 0xff, 0x01, // command, length, payload
    0x11, 0x01, 0x00, // command, length, payload
    0xfe, 0xfe, 0x96, // delay (ms)
    0x29, 0x01, 0x00, // command, length, payload
    0xfe, 0xfe, 0x14, // delay (ms)
    0xff, 0xff, // end
};
static_assert(sizeof(g_panelCommands) == 341, "panel command stream");
struct PanelMipiConfig {
    unsigned char hsTrail, hsPrepare, hsZero, clkHsPrepare, clkHsExit, clkTrail;
    unsigned char clkZero, clkHsPost, dataHsExit, contDet, lpx, taGet, taSure, taGo;
    unsigned short hActive, hPulse, hBackPorch, hFrontPorch;
    unsigned short vActive, vPulse, vBackPorch, vFrontPorch, bllp, fps;
    int laneMode, format, controlMode;
    unsigned char *commands;
    unsigned commandSize;
    unsigned char syncCalibrate;
    unsigned short virtualHsyncStart, virtualHsyncEnd, vsyncRef, dataClockSkew;
    unsigned char polarity[5];
    int packetType;
};
static_assert(sizeof(PanelMipiConfig) == 76, "MI_PANEL_MipiDsiConfig_t storage");
PanelMipiConfig g_panelMipiConfig = { // 0x17e6a0
    3, 2, 5, 2, 4, 4, 10, 10, 4, 0, 16, 26, 22, 50,
    640, 20, 22, 120, 480, 5, 13, 17, 0, 60,
    2, 3, 1, g_panelCommands, sizeof(g_panelCommands),
    1, 431, 441, 32978, 9, {0, 0, 0, 0, 0}, 0
};
bool g_customDisplayTiming = true; // 0x17e6ec
struct DisplayAttr {
    unsigned background;
    int interface, timing;
    bool userSync;
    char pad[3];
    unsigned short sync[14];
    unsigned frameRate;
};
extern "C" int MI_DISP_GetPubAttr(int, DisplayAttr*);
extern "C" int MI_DISP_SetPubAttr(int, DisplayAttr*);
// 0x2b5bc
void display_memset_2b5bc() {
    DisplayAttr attr;
    char reserved[16]; // Retain the original unused stack storage.
    memset(&attr, 0, 48);
    MI_DISP_GetPubAttr(0, &attr);
    attr.userSync = true;
    attr.sync[0] = g_panelInfo.timing[7];
    attr.sync[1] = g_panelInfo.timing[3];
    attr.sync[2] = g_panelInfo.timing[12]-(g_panelInfo.timing[2] + g_panelInfo.timing[7] + g_panelInfo.timing[3]);
    attr.sync[3] = g_panelInfo.timing[6];
    attr.sync[4] = g_panelInfo.timing[1];
    attr.sync[5] = g_panelInfo.timing[9]-(g_panelInfo.timing[0] + g_panelInfo.timing[6] + g_panelInfo.timing[1]);
    attr.sync[7] = 0;
    attr.sync[8] = 0;
    attr.sync[9] = 0;
    attr.sync[10] = g_panelInfo.timing[0];
    attr.sync[11] = g_panelInfo.timing[2];
    attr.frameRate = g_panelInfo.timing[15]*1000000/(g_panelInfo.timing[9]*g_panelInfo.timing[12]);
    attr.timing = 32;
    attr.interface = 6;
    attr.background = 0xff000000;
    MI_DISP_SetPubAttr(0, &attr);
}
