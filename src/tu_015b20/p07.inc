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
    char raw[84];
    unsigned short timing[16];
};
extern PanelInfo g_panelInfo; // 0x17e48c
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
    attr.frameRate = (unsigned)g_panelInfo.timing[15]*1000000/((unsigned)g_panelInfo.timing[9]*(unsigned)g_panelInfo.timing[12]);
    attr.timing = 32;
    attr.interface = 6;
    attr.background = 0xff000000;
    MI_DISP_SetPubAttr(0, &attr);
}
