// p07 0x2849c-0x2b788: settings controls, actions and device information.
#include "../KeySettingWindow.h"
#include "../KeyTestWindow2.h"
#include "../WifiMenus.h"
extern "C" {


void setKeymap(const char *);
void setLanguageFile(const char *);
int MI_SYS_ReadUuid(unsigned long long *);
}

int removeAllDBFiles();
int removeEmulatorConfigs();
float maxCpuMHz();
int memoryMB();
std::string sdCardSpace();
// 0x2849c: the settings list (WIN_SETTING and WIN_FIXES): rows with an icon, a label and, for
// adjustable settings, the value between left/right arrows.
SettingMenu::SettingMenu() : TextMenu(true, true, 6, NULL)
{
    volumePreviewTimer = NULL;
    settingLeftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    rightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x28614: closing settings saves system.json (and applies a pending volume preview).
SettingMenu::~SettingMenu()
{
    freeListItem();
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
    g_settingsDirty = false;
    if (volumePreviewTimer) {
        SDL_RemoveTimer(volumePreviewTimer);
        setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    }
    if (settingLeftArrow) SDL_FreeSurface(settingLeftArrow);
    if (rightArrow) SDL_FreeSurface(rightArrow);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2877c: after a theme change: reload the row icons, arrows and highlight.
void SettingMenu::reloadTheme()
{
    if (themePath.compare(g_themePath) != 0) {
        for (int i = 0; i < items.size(); i++) {
            SettingMenuItem *item = (SettingMenuItem *)items[i];
            item->reload();
        }
        if (settingLeftArrow) SDL_FreeSurface(settingLeftArrow);
        if (rightArrow) SDL_FreeSurface(rightArrow);
        if (selectedBackground) SDL_FreeSurface(selectedBackground);
        settingLeftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
        rightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x28994: draws the visible rows: icon and label, and for adjustable settings the current value
// (volume, brightness, music volume, sleep timer, audio fix) between arrows; the Wi-Fi row shows
// the IP address when connected.
void SettingMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0};
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (row < 5 && g_listStripe) {
            dest.x = 0;
            dest.y = (row + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        }
        if (rowBackground) {   // never set: always NULL
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->iconSurface) {
            area.y = (60 - item->iconSurface->h) / 2 + y;
            SDL_UpperBlit(item->iconSurface, NULL, dst, &area);
            area.x = area.x + 60;
        }
        area.y = y;
        area.w = rect.w - (settingLeftArrow ? settingLeftArrow->w : 0) - (rightArrow ? rightArrow->w : 0) - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, shadowSelected ? i == cur : false);
        area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - 240;
        area.w = 200;
        int type = item->handler;
        int mode;
        WifiStatus *status;
        int level, timeout;
        if (type == SETTING_VOLUME || type == SETTING_BRIGHTNESS || type == SETTING_BGM_VOLUME || type == SETTING_HIBERNATE || type == SETTING_UNUSED_14 || type == SETTING_AUDIOFIX) {
            if (type == SETTING_VOLUME) {
                level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
                snprintf(text, 32, "%02d/%02d", level, 20);
            } else if (type == SETTING_BRIGHTNESS) {
                level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
                snprintf(text, 32, "%02d/%02d", level, 10);
            } else if (type == SETTING_BGM_VOLUME)
                snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME), 20);
            else if (type == SETTING_HIBERNATE) {
                timeout = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
                if (timeout < 5 || timeout > 30)
                    strncpy(text, languageString(STR_NEVER), 32);
                else
                    snprintf(text, 32, languageString(STR_N_MIN_LATER), timeout);
            } else if (type == SETTING_AUDIOFIX) {
                timeout = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
                if (timeout != 0)
                    strncpy(text, languageString(STR_ON), 32);
                else
                    strncpy(text, languageString(STR_OFF), 32);
            }
            area.y += 2;
            drawText(dst, g_imeFont, area, text, color, 1, false);
            if (settingLeftArrow) {
                area.y = (60 - settingLeftArrow->h) / 2 + y;
                area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - settingLeftArrow->w - 240;
                SDL_UpperBlit(settingLeftArrow, NULL, dst, &area);
            }
            if (rightArrow) {
                area.y = (60 - rightArrow->h) / 2 + y;
                area.x = rect.w + x - rightArrow->w - 40;
                SDL_UpperBlit(rightArrow, NULL, dst, &area);
            }
        } else if (type == SETTING_WIFI) {
            mode = wifiMode();
            if (mode == WIFI_MODE_STATION) {
                status = getWifiStatus();
                if (status->state == WIFI_STATE_COMPLETED) {
                    std::string label(status->ip);
                    if (label.size() != 0) {
                        area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - 240;
                        area.w = rightArrow ? rightArrow->w + 200 : 200;
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

// 0x294f8: timer, 150 ms after the volume last changed: apply it and play the "enter" sound as a
// preview.
static unsigned previewVolume(unsigned interval, void *param)
{
    if (g_enterSound) {
        puts("Mix_PlayChannel enter.wav");
        if (Mix_PlayChannel(0, g_enterSound, 0) == -1) printf("Mix_PlayChunk: %s\n", SDL_GetError());
    }
    setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    return 0;
}

// 0x295a8: left on an adjustable setting: one step down (volume and music 0-20, brightness 1-10),
// or the next sleep timeout (0, 5, 15, 30 min) or the audio fix switch; marks settings changed.
void SettingMenu::decrease()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case SETTING_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_VOLUME, level);
            SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
            if (volumePreviewTimer) {
                SDL_RemoveTimer(volumePreviewTimer);
                Mix_HaltChannel(0);
            }
            volumePreviewTimer = SDL_AddTimer(150, previewVolume, NULL);
            break;
        case SETTING_BRIGHTNESS:
            level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
            if (level > 1) level--;
            SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, level);
            setBacklight(level);
            break;
        case SETTING_BGM_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, level);
            Mix_VolumeMusic((level << 7) / 20);
            Mix_Volume(1, (level << 7) / 20);
            break;
        case SETTING_HIBERNATE:
            level = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
            if (level == 0)
                level = 5;
            else if (level == 5)
                level = 15;
            else if (level == 15)
                level = 30;
            else
                level = 0;
            SetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY, level);
            break;
        case SETTING_AUDIOFIX:
            level = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
            if (level == 0) {
                level = 1;
                system("rm -f /tmp/no_audioserver");
            } else {
                level = 0;
                system("touch /tmp/no_audioserver");
            }
            SetKeyShm(g_keyShm, MONITOR_AUDIOFIX, level);
            break;
        default: break;
    }
}

// 0x298ec: right: one step up; otherwise as decrease.
void SettingMenu::increase()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case SETTING_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_VOLUME, level);
            SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
            if (volumePreviewTimer) {
                SDL_RemoveTimer(volumePreviewTimer);
                Mix_HaltChannel(0);
            }
            volumePreviewTimer = SDL_AddTimer(150, previewVolume, NULL);
            break;
        case SETTING_BRIGHTNESS:
            level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
            if (level < 10) level++;
            SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, level);
            setBacklight(level);
            break;
        case SETTING_BGM_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, level);
            Mix_VolumeMusic((level << 7) / 20);
            Mix_Volume(1, (level << 7) / 20);
            break;
        case SETTING_HIBERNATE:
            level = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
            if (level == 0)
                level = 5;
            else if (level == 5)
                level = 15;
            else if (level == 15)
                level = 30;
            else
                level = 0;
            SetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY, level);
            break;
        case SETTING_AUDIOFIX:
            level = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
            if (level == 0) {
                level = 1;
                system("rm -f /tmp/no_audioserver");
            } else {
                level = 0;
                system("touch /tmp/no_audioserver");
            }
            SetKeyShm(g_keyShm, MONITOR_AUDIOFIX, level);
            break;
        default: break;
    }
}

// 0x29c30: reboot into the flashing mode (unused).
static void rebootToEfex()
{
    system("reboot efex");
}

// 0x29c4c: "Refresh Roms": delete the ROM caches and the emulator settings.
static int resetEmulatorSettings(void *arg)
{
    removeAllDBFiles();
    removeEmulatorConfigs();
    return 1;
}

// 0x29c74: factory reset: also deletes favourites, system.json and the saved list positions,
// restores the default volume, brightness and colours, clears Recent, the keymap and the language.
static int resetAllSettings(void *arg)
{
    resetEmulatorSettings(NULL);
    unlink("/mnt/SDCARD/Roms/favourite.json");
    unlink("/mnt/SDCARD/system.json");
    unlink("/appconfigs/romwinidx.json");
    g_romListPositions.clear();
    SetKeyShm(g_keyShm, MONITOR_VOLUME, 20);
    SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, 8);
    SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
    SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, 7);
    SetKeyShm(g_keyShm, MONITOR_LUMINATION, 7);
    SetKeyShm(g_keyShm, MONITOR_HUE, 10);
    SetKeyShm(g_keyShm, MONITOR_SATURATION, 10);
    SetKeyShm(g_keyShm, MONITOR_CONTRAST, 10);
    setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    setLcdColor(7, 10, 10, 10);
    clearRecent(&g_recentGames);
    setKeymap(NULL);
    setLanguageFile(NULL);
    sync();
    initAudioSettings(g_keyShm);
    return 1;
}

// 0x29dc8: A on a row: opens what it stands for (device info, fixes, IO test, ADC info, language,
// key setting, colour, themes, Wi-Fi, hotspot) or asks to confirm a reset or power-off.
void SettingMenu::select()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int type = item->handler;
    switch (type) {
        case SETTING_DEVICE_INFO: {
            MenuWindow *window = new MenuWindow(STR_DEVICE_INFO, WIN_DEVICE_INFO, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_FIXES: {
            MenuWindow *window = new MenuWindow(STR_FIXES, WIN_FIXES, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_IO_TEST: {
            KeyTestWindow2 *window = new KeyTestWindow2;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_ADC_INFO: {
            ADCInfoWindow *window = new ADCInfoWindow(STR_ADC_INFO, WIN_ADC_INFO);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_REFRESH_ROMS: {
            Action *action = new GeneralAction(-1, resetEmulatorSettings, NULL);
            Action *progress = new PopupProgressAction(STR_REFRESH, action);
            DialogWindow *window = new DialogWindow(STR_REFRESH_ROMS_2, STR_REFRESH_ROMS_TEXT, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_FACTORY_RESET: {
            Action *action = new GeneralAction(-1, resetAllSettings, NULL);
            Action *progress = new PopupProgressAction(STR_RESET, action);
            DialogWindow *window = new DialogWindow(STR_RESUME_DEFAULT_SETTING, STR_RESUME_SETTINGS_TEXT, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_LANGUAGE: {
            MenuWindow *window = new MenuWindow(STR_LANGUAGE, WIN_LANGUAGE, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_KEYMAP: {
            KeySettingWindow *window = new KeySettingWindow;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_COLOR: {
            MenuWindow *window = new MenuWindow(STR_COLOR, WIN_COLOR, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_THEME: {
            MenuWindow *window = new MenuWindow(STR_THEMES, WIN_THEME, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_POWER_OFF: {
            DialogWindow *window = new DialogWindow(STR_POWER_OFF, STR_SHUT_DOWN_TEXT, false, new GeneralAction(-1, powerOff, NULL));
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_WIFI: {
            WifiMenu *menu = new WifiMenu;
            MenuWindow *window = new MenuWindow(STR_WIFI, WIN_WIFI, (ListMenu *)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_HOTSPOT: {
            HotspotMenu *menu = new HotspotMenu;
            MenuWindow *window = new MenuWindow(STR_HOTSPOT, WIN_HOTSPOT, (ListMenu *)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        default: return;
    }
}

// 0x2a5d4: one button: up/down move, left/right adjust, A/Y select, B/X close, MENU passes through.
int SettingMenu::handleKey()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_UP:
            puts("key_up");
            moveUp();
            break;
        case BUTTON_DOWN:
            puts("key_down");
            moveDown();
            break;
        case BUTTON_LEFT:
            decrease();
            break;
        case BUTTON_RIGHT:
            increase();
            break;
        case BUTTON_A:
        case BUTTON_Y:
            puts("a");
            select();
            break;
        case BUTTON_B:
        case BUTTON_X:
            puts("b or c");
            return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        default: break;
    }
    return -1;
}

// 0x2a6ec: deletes every row.
void SettingMenu::freeListItem()
{
    puts("SettingMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        delete item;
    }
    items.clear();
}

struct DeviceInfoItem {
    DeviceInfoItem(const char *, const char *);
    ~DeviceInfoItem()
    {
    }
    std::string label, value;
};
// 0x2a7b0: one "label: value" row of the device information.
DeviceInfoItem::DeviceInfoItem(const char *labelText, const char *valueText)
{
    label = labelText;
    value = valueText;
}

// 0x2a83c: the device information list (WIN_DEVICE_INFO).
DeviceInfoMenu::DeviceInfoMenu() : TextMenu(true, true, 6, NULL)
{
    firmwareVersion = NULL;
    loadInfo();
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x2a934
DeviceInfoMenu::~DeviceInfoMenu()
{
    freeListItem();
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
    if (firmwareVersion) free(firmwareVersion);
}

// 0x2aa08: after a theme change: reload the highlight.
void DeviceInfoMenu::reloadTheme()
{
    if (themePath.compare(g_themePath) != 0) {
        if (selectedBackground) SDL_FreeSurface(selectedBackground);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x2aae4: draws the visible rows: label on the left, value on the right.
void DeviceInfoMenu::drawList(SDL_Surface *dst)
{
    TTF_Font *font = g_imeFont;
    int height = TTF_FontHeight(font);
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
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = 0;
    while (i < 5 && g_listStripe) {
        dest.x = 0;
        dest.y = (i + 2) * 60;
        SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        i++;
    }
    i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        DeviceInfoItem *item = (DeviceInfoItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (rowBackground) {
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        area.y = (60 - height) / 2 + y;
        area.w = rect.w - 40;
        area.h = 60;
        int width = drawText(dst, font, area, item->label.c_str(), color, 0, shadowSelected ? i == cur : false);
        area.x = area.x + width;
        area.w = rect.w - area.x - 20;
        drawText(dst, font, area, item->value.c_str(), color, 2, false);
        i++;
        row++;
    }
}

// 0x2b028: the chip's unique id (MI_SYS_ReadUuid) as 16 hex digits; left empty if it cannot be
// read.
static void formatDeviceUuid(char *out, size_t size)
{
    int rc = 0xa009201f;
    struct Words {
        unsigned char data[8];
    };
    Words words;
    memset(&words, 0, 8);
    union {
        unsigned long long value;
        Words parts;
    } uuid;
    uuid.value = 0;
    rc = MI_SYS_ReadUuid(&uuid.value);
    words = uuid.parts;
    if (rc == 0) snprintf(out, size, "%02X%02X%02X%02X%02X%02X%02X%02X", words.data[7], words.data[6], words.data[5], words.data[4], words.data[3], words.data[2], words.data[1], words.data[0]);
}

// 0x2b0f8: fills the rows: model MY354, serial (chip id), firmware version (miyoo_version from the
// U-Boot environment), CPU clock, memory, SD card use, website.
void DeviceInfoMenu::loadInfo()
{
    char uuid[32] = {
        0};
    char buf[16];
    formatDeviceUuid(uuid, 32);
    char *version = NULL;
    if (firmwareVersion) {
        free(firmwareVersion);
        firmwareVersion = NULL;
    }
    system("/etc/fw_printenv miyoo_version > /tmp/miyoo_version");
    FILE *f = fopen("/tmp/miyoo_version", "r");
    if (f) {
        puts("loadContent");
        readWholeFile("/tmp/miyoo_version", f, &firmwareVersion);
        puts("loadContent end");
        fclose(f);
        version = strstr(firmwareVersion, "\n");
        if (version) *version = 0;
        version = strstr(firmwareVersion, "miyoo_version=");
        if (version) version += 14;
    }
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_MODE), "MY354"));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_SERIAL), uuid));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_VERSION), version ? version : "202110071805"));
    snprintf(buf, 16, "%dMHz", (int)maxCpuMHz());
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_CPU), buf));
    snprintf(buf, 16, "%dM", memoryMB());
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_MEMORY), buf));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_SD_CARD), sdCardSpace().c_str()));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_WEBSITE), "www.lomiyoo.com"));
}

// 0x2b504: deletes every row.
void DeviceInfoMenu::freeListItem()
{
    for (int i = 0; i < items.size(); i++) {
        DeviceInfoItem *item = (DeviceInfoItem *)items[i];
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
        75,
        49,
        48,
        49,
        95,
        73,
        77,
        50,
        66,
        86,
        76,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        11,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        1,
        1,
        0,
        30,
        0,
        144,
        1,
        80,
        0,
        30,
        0,
    },
    {20, 22, 5, 13, 40, 18, 640, 480, 0, 780, 0, 0, 515, 0, 0, 26},
    0,
    0,
    0,
    160,
    255,
    80,
    0,
    0,
    0,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    1868942,
    1633113,
    1,
    0,
    {2, 4, 3, 1, 0}}; // 0x17e48c

// 0x17e548: MIPI command stream, including delays and the terminating marker.
unsigned char g_panelCommands[] = {
    0x01,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x50, // delay (ms)
    0xb9,
    0x03,
    0xf1,
    0x12,
    0x83, // command, length, payload
    0xb1,
    0x05,
    0x00,
    0x00,
    0x00,
    0xda,
    0x80, // command, length, payload
    0xb2,
    0x03,
    0x00,
    0x13,
    0x70, // command, length, payload
    0xb3,
    0x0a,
    0x10,
    0x10,
    0x28,
    0x28,
    0x03,
    0xff,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xb4,
    0x01,
    0x80, // command, length, payload
    0xb5,
    0x02,
    0x0a,
    0x0a, // command, length, payload
    0xb6,
    0x02,
    0x87,
    0x87, // command, length, payload
    0xb8,
    0x04,
    0x26,
    0x22,
    0xf0,
    0x63, // command, length, payload
    0xba,
    0x1b,
    0x31,
    0x81,
    0x05,
    0xf9,
    0x0e,
    0x0e,
    0x20,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x44,
    0x25,
    0x00,
    0x91,
    0x0a,
    0x00,
    0x00,
    0x02,
    0x4f,
    0xd1,
    0x00,
    0x00,
    0x37, // command, length, payload
    0xbc,
    0x01,
    0x47, // command, length, payload
    0xbf,
    0x03,
    0x02,
    0x11,
    0x00, // command, length, payload
    0xc0,
    0x09,
    0x73,
    0x73,
    0x50,
    0x50,
    0x00,
    0x00,
    0x12,
    0x50,
    0x00, // command, length, payload
    0xc1,
    0x0c,
    0x53,
    0x00,
    0x32,
    0x32,
    0x77,
    0xd1,
    0xcc,
    0xcc,
    0x77,
    0x77,
    0x33,
    0x33, // command, length, payload
    0xc6,
    0x06,
    0x82,
    0x00,
    0xbf,
    0xff,
    0x00,
    0xff, // command, length, payload
    0xc7,
    0x06,
    0xb8,
    0x00,
    0x0a,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xc8,
    0x04,
    0x10,
    0x40,
    0x1e,
    0x02, // command, length, payload
    0xcc,
    0x01,
    0x0b, // command, length, payload
    0x36,
    0x01,
    0xc0, // command, length, payload
    0xe0,
    0x22,
    0x00,
    0x06,
    0x0a,
    0x2a,
    0x3d,
    0x3f,
    0x3b,
    0x37,
    0x06,
    0x0b,
    0x0c,
    0x10,
    0x11,
    0x10,
    0x13,
    0x12,
    0x18,
    0x00,
    0x06,
    0x0a,
    0x2a,
    0x3d,
    0x3f,
    0x3b,
    0x37,
    0x06,
    0x0b,
    0x0c,
    0x10,
    0x11,
    0x10,
    0x13,
    0x12,
    0x18, // command, length, payload
    0xe3,
    0x0e,
    0x07,
    0x07,
    0x0b,
    0x0b,
    0x0b,
    0x0b,
    0x00,
    0x00,
    0x00,
    0x00,
    0xff,
    0x00,
    0xc0,
    0x10, // command, length, payload
    0xe9,
    0x3f,
    0xc8,
    0x10,
    0x02,
    0x00,
    0x00,
    0xb0,
    0xb1,
    0x11,
    0x31,
    0x23,
    0x28,
    0x80,
    0xb0,
    0xb1,
    0x27,
    0x08,
    0x00,
    0x04,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x02,
    0x00,
    0x00,
    0x00,
    0x88,
    0x88,
    0xba,
    0x60,
    0x24,
    0x08,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0xba,
    0x71,
    0x35,
    0x18,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xea,
    0x3d,
    0x97,
    0x0a,
    0x82,
    0x02,
    0x03,
    0x07,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x81,
    0x88,
    0xba,
    0x17,
    0x53,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x80,
    0x88,
    0xba,
    0x06,
    0x42,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x23,
    0x00,
    0x00,
    0x01,
    0xa4,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xef,
    0x03,
    0xff,
    0xff,
    0x01, // command, length, payload
    0x11,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x96, // delay (ms)
    0x29,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x14, // delay (ms)
    0xff,
    0xff, // end
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
                                     3,
                                     2,
                                     5,
                                     2,
                                     4,
                                     4,
                                     10,
                                     10,
                                     4,
                                     0,
                                     16,
                                     26,
                                     22,
                                     50,
                                     640,
                                     20,
                                     22,
                                     120,
                                     480,
                                     5,
                                     13,
                                     17,
                                     0,
                                     60,
                                     2,
                                     3,
                                     1,
                                     g_panelCommands,
                                     sizeof(g_panelCommands),
                                     1,
                                     431,
                                     441,
                                     32978,
                                     9,
                                     {0, 0, 0, 0, 0},
                                     0};
bool g_customDisplayTiming = true; // 0x17e6ec
struct DisplayAttr {
    unsigned background;
    int interface, timing;
    bool userSync;
    char pad[3];
    unsigned short sync[14];
    unsigned frameRate;
};
extern "C" int MI_DISP_GetPubAttr(int, DisplayAttr *);
extern "C" int MI_DISP_SetPubAttr(int, DisplayAttr *);
// 0x2b5bc: programs the display controller with the panel timing from g_panelInfo (sync widths,
// porches, frame rate).
void applyPanelTiming()
{
    DisplayAttr attr;
    char reserved[16]; // Retain the original unused stack storage.
    memset(&attr, 0, 48);
    MI_DISP_GetPubAttr(0, &attr);
    attr.userSync = true;
    attr.sync[0] = g_panelInfo.timing[7];
    attr.sync[1] = g_panelInfo.timing[3];
    attr.sync[2] = g_panelInfo.timing[12] - (g_panelInfo.timing[2] + g_panelInfo.timing[7] + g_panelInfo.timing[3]);
    attr.sync[3] = g_panelInfo.timing[6];
    attr.sync[4] = g_panelInfo.timing[1];
    attr.sync[5] = g_panelInfo.timing[9] - (g_panelInfo.timing[0] + g_panelInfo.timing[6] + g_panelInfo.timing[1]);
    attr.sync[7] = 0;
    attr.sync[8] = 0;
    attr.sync[9] = 0;
    attr.sync[10] = g_panelInfo.timing[0];
    attr.sync[11] = g_panelInfo.timing[2];
    attr.frameRate = g_panelInfo.timing[15] * 1000000 / (g_panelInfo.timing[9] * g_panelInfo.timing[12]);
    attr.timing = 32;
    attr.interface = 6;
    attr.background = 0xff000000;
    MI_DISP_SetPubAttr(0, &attr);
}
