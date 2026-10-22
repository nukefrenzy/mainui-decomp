// Wi-Fi/hotspot menus, actions and drawable rows; source order follows MainUI.
// Proven 0x138c50-0x13edfc: 14,368-byte body plus 122 inline/template functions.
// Keep ignored system() result stores and the conditional returns: they affect -O0 layout.
#include "PlatformApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include "WifiMenus.h"

extern "C" {
int SDL_UpperBlit(SDL_Surface *, SDL_Rect *, SDL_Surface *, SDL_Rect *);
void SDL_FreeSurface(SDL_Surface *);
int SDL_FillRect(SDL_Surface *, SDL_Rect *, unsigned int);
unsigned int SDL_MapRGBA(SDL_PixelFormat *, unsigned char, unsigned char, unsigned char, unsigned char);
int TTF_FontHeight(void *);
int TTF_SizeText(void *, const char *, int *, int *);
int SDL_PushEvent(void *);
int SDL_WaitEvent(void *);
const char *SDL_GetError();
int getWifiOn(); void setWifiOn(int);
// Legacy names in tu_14b860.c: this flag actually selects hotspot/AP mode.
int getWifiConnected(); void setWifiConnected(int);
int setWifiScanEnable(int);
int wifiConnectOpen(const char *); int wifiConnectPsk(const char *, const char *);
WifiStatus queryWifiStatus(); WifiStatus *getWifiStatus();
int wifiStartConnect(); int copyScanResults(WifiAP *, int); int wifiSignalLevel(int);
void saveSystemJson(void *, const char *, int);
}
extern char *g_themePath;
extern char g_keyShm[];
extern int g_fontSize;
char g_wlanStatusBuffer[128]; // 0x182418
extern void *g_imeFont;
extern SDL_Color g_imeTextColor; // 0x182284
extern SDL_Color g_wifiSelectedColor; // 0x182280
extern SDL_Color g_wifiEmptyColor; // 0x182274
extern SDL_Surface *g_listStripe; // 0x1821c8
extern std::vector<TrimusWindow *> *g_windowStack;
SDL_Surface *loadImeImage(const char *, const char *, const char *);
const char *languageString(int);
int drawText(SDL_Surface *, void *, SDL_Rect, const char *, SDL_Color, int, bool);
int findProcess(const char *, int);
unsigned long milliseconds();
SDL_Surface *createSurface(int, int);
void pushWindow(TrimusWindow *);
struct UserEvent { unsigned char type; int code; void *data1, *data2; };
union InputEvent { unsigned char type; UserEvent user; char storage[20]; };
class ProgressDialog : public TrimusWindow {
public:
    ProgressDialog(int, Action *);
    virtual ~ProgressDialog();
    virtual int handleInput(); virtual int draw(SDL_Surface *);
    virtual void onHide(); virtual void onShow();
    virtual void saveState(cJSON *); virtual int restoreState(cJSON *); virtual void update();
    char raw[0x3c - 12];
};
class NotifyWindow : public TrimusWindow {
public:
    NotifyWindow(const char *, const char *, const char *, void (*)());
    virtual ~NotifyWindow();
    virtual int handleInput(); virtual int draw(SDL_Surface *);
    virtual void onHide(); virtual void onShow();
    virtual void saveState(cJSON *); virtual int restoreState(cJSON *); virtual void update();
    char raw[0x3c - 12];
};

static_assert(sizeof(WifiAP) == 0x114, "WifiAP layout");
static_assert(sizeof(WifiStatus) == 0x74, "WifiStatus layout");
static_assert(sizeof(UserEvent) == 16 && sizeof(InputEvent) == 20, "SDL event layouts");
static_assert(sizeof(ListMenu) == 0x48 && sizeof(TextMenuDrawer) == 0xf8, "menu base layouts");
static_assert(sizeof(WifiMenuDrawer) == 0x5c, "WifiMenuDrawer layout");
static_assert(sizeof(WifiMenu) == 0x22fc && sizeof(HotspotMenu) == 0x5c, "Wi-Fi menu layouts");
static_assert(sizeof(WifiItem) == 0x38 && sizeof(WifiConnectAction) == 0x138, "Wi-Fi item/action layouts");
static_assert(sizeof(ProgressDialog) == 0x3c && sizeof(NotifyWindow) == 0x3c, "external window layouts");

int wifiMode()
{
    int pid = findProcess("wpa_supplicant", 0);
    if (pid >= 0) return 2;
    pid = findProcess("hostapd", 0);
    if (pid >= 0) return 3;
    return 1;
}
int wlanAddress(std::string &address)
{
    int result = -1;
    if (system("ip addr show wlan0 > /tmp/wlan0_status") == 0) {
        FILE *f = fopen("/tmp/wlan0_status", "r");
        if (f) {
            char *ip = NULL;
            char *end = NULL;
            while (fgets(g_wlanStatusBuffer, 128, f)) {
                ip = strstr(g_wlanStatusBuffer, "inet ");
                if (ip) {
                    end = strstr(ip, "/");
                    if (end) { *end = 0; address = ip + 5; result = 0; }
                }
            }
            fclose(f);
        }
    }
    return result;
}
void switch2WpaMode(int pid)
{
    if (pid > 0) { printf("%s kill hostapd %d\n", "switch2WpaMode", pid); kill(pid, 15); }
    int dhcp = findProcess("udhcpd", 0);
    printf("%s kill udhcpd %d\n", "switch2WpaMode", dhcp);
    if (dhcp > 0) kill(dhcp, 9);
    int rc;
    rc = system("ifconfig wlan0 down");
    rc = system("killall -15 wpa_supplicant");
    rc = system("killall -15 udhcpc");
    rc = system("ifconfig wlan0 up");
    rc = system("wpa_supplicant -B -D nl80211 -iwlan0 -c /appconfigs/wpa_supplicant.conf");
    rc = system("udhcpc -i wlan0 -s /etc/init.d/udhcpc.script &");
    rc = system("ifconfig | grep HWaddr | sed 's/[ ]\\+/ /g' | cut -d' ' -f5 > /tmp/wifimac");
}
void swicth2ApMode()
{
    int pid = findProcess("wpa_supplicant", 0);
    printf("%s kill wpa_supplicant %d\n", "swicth2ApMode", pid);
    if (pid > 0) kill(pid, 15);
    printf("%s kill udhcpc %d\n", "swicth2ApMode", pid);
    pid = findProcess("udhcpc", 0);
    if (pid > 0) kill(pid, 15);
    int rc;
    rc = system("ifconfig wlan0 down");
    rc = system("killall  -9 udhcpd");
    rc = system("killall  -15 hostapd");
    rc = system("ifconfig wlan0 192.168.0.18 up");
    usleep(500000);
    rc = system("hostapd /etc/wifi/hostapd.conf -B");
    rc = system("udhcpd /etc/wifi/udhcpd.conf -f &");
    rc = system("ifconfig | grep HWaddr | sed 's/[ ]\\+/ /g' | cut -d' ' -f5 > /tmp/wifimac");
}
WifiOnOffLabel::WifiOnOffLabel(Action *a, bool enabled) : ItemDrawable(a)
{
    left = loadImeImage(g_themePath, "./", "skin/icon-left-arrow-24.png");
    right = loadImeImage(g_themePath, "./", "skin/icon-right-arrow-24.png");
    this->enabled = enabled;
}
WifiOnOffLabel::~WifiOnOffLabel()
{
    if (left) SDL_FreeSurface(left);
    if (right) SDL_FreeSurface(right);
}
int WifiOnOffLabel::draw(SDL_Surface *dst, SDL_Rect *r, bool selected)
{
    SDL_Rect rect = *r;
    rect.x += 20;
    drawText(dst, g_imeFont, rect, languageString(0x83), selected ? g_wifiSelectedColor : g_imeTextColor, 3, false);
    rect.w = 200;
    rect.x = r->x + r->w - rect.w - (right ? right->w : 0) - 40;
    int on = getWifiOn();
    if (on) drawText(dst, g_imeFont, rect, languageString(0x68), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
    else drawText(dst, g_imeFont, rect, languageString(0x69), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
    if (left && right) {
        rect.y = r->y + (60 - left->h) / 2;
        rect.x = r->x + r->w - (right ? right->w : 0) - left->w - 240;
        SDL_UpperBlit(left, NULL, dst, &rect);
        rect.y = r->y + (60 - right->h) / 2;
        rect.x = r->x + r->w - right->w - 40;
        SDL_UpperBlit(right, NULL, dst, &rect);
    }
    return 0;
}
WifiLabel::WifiLabel(Action *a) : ItemDrawable(a) {}
WifiLabel::~WifiLabel() {}
int WifiLabel::draw(SDL_Surface *dst, SDL_Rect *r, bool selected)
{
    SDL_Rect rect = *r;
    SDL_FillRect(dst, r, SDL_MapRGBA(dst->format, 255, 255, 255, 255));
    rect.x += 15;
    drawText(dst, g_imeFont, rect, languageString(0x91), g_wifiSelectedColor, 3, false);
    return 0;
}
int WifiItem::draw(SDL_Surface *dst, SDL_Rect *r, bool selected)
{
    SDL_Rect rect = *r;
    rect.x += 20;
    drawText(dst, g_imeFont, rect, ssid.c_str(), selected ? g_wifiSelectedColor : g_imeTextColor, 3, false);
    rect.x = r->x + r->w - 20;
    if (signal) {
        rect.y = r->y + (r->h - signal->h) / 2; rect.x -= signal->w;
        rect.w = signal->w; rect.h = signal->h;
        SDL_UpperBlit(signal, NULL, dst, &rect);
        rect.x -= 5;
    }
    if (secured && lock) {
        rect.y = r->y + (r->h - lock->h) / 2; rect.x -= lock->w;
        rect.w = lock->w; rect.h = lock->h;
        SDL_UpperBlit(lock, NULL, dst, &rect); rect.x -= 5;
    }
    if (connected && check) {
        rect.y = r->y + (r->h - check->h) / 2; rect.x -= check->w;
        rect.w = check->w; rect.h = check->h;
        SDL_UpperBlit(check, NULL, dst, &rect);
    }
    return 0;
}
int WifiConnectAction::execute(void *param)
{
    puts("WifiConnectAction dowork");
    if (secured) {
        printf("wifi %p connect %s %s\n", this, ap.ssid, psk.c_str());
        wifiConnectPsk(ap.ssid, psk.c_str());
    } else {
        printf("wifi connect %s\n", ap.ssid);
        wifiConnectOpen(ap.ssid);
    }
    int i = 0;
    WifiStatus status;
    UserEvent ev;
    do {
        usleep(1000000);
        status = queryWifiStatus();
        printf("%p connect result stage=%d, status=%d, ssid=%s, ip=%s, sig=(%d,%d,%d,%d), id=%d\n",
               this, status.state, status.status, status.ssid, status.ip, status.rssi, status.linkSpeed, status.noise, status.freq, status.id);
        i++;
    } while ((status.state == 2 || status.state == 0) && i < 11);
    if (status.state == 4) { puts("try dhcp"); wifiStartConnect(); setWifiConnected(0); usleep(2000000); ev.type = 24; ev.code = 30; }
    else { ev.type = 24; ev.code = 39; }
    if (SDL_PushEvent(&ev) < 0) printf("%s %d Failed to push SDL event %s\n", "dowork", 241, SDL_GetError());
    return -1;
}
int WifiConnectDialogAction::execute(void *param)
{
    ProgressDialog *window = new ProgressDialog(0x83, new WifiConnectAction(&ap, (char *)param, true));
    pushWindow(window);
    return 4;
}
int WifiItemAction::execute(void *)
{
    WifiStatus *status = getWifiStatus();
    if (!status || strcmp(status->ssid, ap.ssid) != 0) {
        if (ap.secured) {
            ImeWindow *window = new ImeWindow(0x83, new WifiConnectDialogAction(&ap), 0x85);
            g_windowStack->push_back((TrimusWindow *)window);
        } else {
            ProgressDialog *window = new ProgressDialog(0x83, new WifiConnectAction(&ap, NULL, false));
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
        }
    }
    return -1;
}
void toggleWifi()
{
    int on = getWifiOn();
    on = !on;
    int rc;
    setWifiOn(on);
    if (on) {
        puts("wifi on"); rc = system("/customer/app/axp_test wifion"); rc = system("sleep 2");
        rc = system("ifconfig wlan0 down"); rc = system("killall -15 wpa_supplicant"); rc = system("killall -15 udhcpc");
        rc = system("ifconfig wlan0 up");
        rc = system("wpa_supplicant -B -D nl80211 -iwlan0 -c /appconfigs/wpa_supplicant.conf");
        rc = system("udhcpc -i wlan0 -s /etc/init.d/udhcpc.script &"); rc = system("touch /tmp/audioserver_restart");
    } else {
        puts("wifi off"); rc = system("ifconfig wlan0 down"); rc = system("killall -15 wpa_supplicant");
        rc = system("killall -15 udhcpc"); rc = system("sleep 0.5"); rc = system("/customer/app/axp_test wifioff");
    }
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
}
int WifiOnOffAction::execute(void *)
{
    UserEvent out;
    out.type = 24; out.code = 42;
    if (getWifiConnected() == 0) {
        toggleWifi();
        int on = getWifiOn();
        printf("wifi on/off action %d\n", on);
        if (on) {
            setWifiScanEnable(1);
            unsigned int start = milliseconds();
            bool done = false;
            InputEvent ev;
            while (SDL_WaitEvent(&ev)) {
                switch (ev.type) {
                case 24:
                    if (ev.user.code == 28) {
                        puts("wifi on/off scan finish"); out.code = 28;
                        if (SDL_PushEvent(&out) < 0) printf("%s : %d Failed to push SDL event: %s\n", "dowork", 362, SDL_GetError());
                        return -1;
                    }
                default:
                    break;
                }
                unsigned int now = milliseconds();
                if (now - start > 10000) {
                    puts("wifi on/off timeout"); out.code = 44; break;
                }
            }
        } else setWifiScanEnable(0);
    }
    if (SDL_PushEvent(&out) < 0) printf("%s : %d Failed to push SDL event: %s\n", "dowork", 386, SDL_GetError());
    return -1;
}
int WifiOnOffDialogAction::execute(void *)
{
    puts("wifi on/off");
    ProgressDialog *window = new ProgressDialog(0x83, new WifiOnOffAction);
    pushWindow(window);
    return -1;
}
WifiMenuDrawer::WifiMenuDrawer() : ListMenu(false)
{
    rect.x = 0; rect.y = 60; rect.w = 640; rect.h = 360;
    rowHeight = 60;
    background = loadImeImage(g_themePath, "./", "skin/bg-list-s.png");
}
WifiMenuDrawer::~WifiMenuDrawer()
{
    if (background) SDL_FreeSurface(background);
    vfunc13();
}
int WifiMenuDrawer::vfunc3()
{
    int action = ListMenu::vfunc3();
    if (action == 5 || action == 6) {
        if (cur == 0) {
            ItemDrawable *item = (ItemDrawable *)getItem(cur);
            printf("mi=%p, mi->theaction=%p\n", item, item ? item->action : NULL);
            if (item && item->action) {
                int result;
                if ((result = item->action->execute(item)) >= 0) return result;
            }
        }
        return -1;
    }
    return action;
}
int WifiMenuDrawer::vfunc4()
{
    ItemDrawable *item = (ItemDrawable *)getItem(cur);
    if (item && item->action) {
        int result;
        if ((result = item->action->execute(item)) >= 0) return result;
    }
    return -1;
}
void *WifiMenuDrawer::getItem(int index)
{
    if (index >= 0 && index < items.size()) return items.at(index);
    return NULL;
}
void WifiMenuDrawer::drawItems(SDL_Surface *dst)
{
    int index;
    int row;
    void *font;
    int fontHeight;
    int x, y;
    SDL_Rect rect;
    SDL_Rect pos;
    font = g_imeFont;
    fontHeight = TTF_FontHeight(font);
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) { pageStart = cur; pageEnd = pageStart + 5; }
    if (cur > pageEnd) { pageEnd = cur; pageStart = pageEnd - 5; }
    if (pageStart < 0) pageStart = 0;
    if (pageEnd >= count() - 1) pageEnd = count() - 1;
    index = pageStart;
    row = 0;
    while (index <= pageEnd) {
        ItemDrawable *item = (ItemDrawable *)getItem(index);
        if (item == NULL) break;
        x = this->rect.x;
        y = this->rect.y + 2 + row * rowHeight;
        if (index < 5 && g_listStripe) {
            rect.x = 0; rect.y = (index + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &rect);
        }
        if (index == cur && item->selectable()) {
            if (background) {
                pos.x = x;
                if (rowHeight > background->h) pos.y = (rowHeight - background->h) / 2 + y;
                else pos.y = y;
                rect.x = 0; rect.y = 0; rect.w = this->rect.w; rect.h = rowHeight;
                SDL_UpperBlit(background, &rect, dst, &pos);
            }
        }
        rect.x = x; rect.y = y; rect.w = this->rect.w; rect.h = rowHeight;
        if (item) item->draw(dst, &rect, item->selectable() ? index == cur : false);
        index++; row++;
    }
    if (count() <= 1 && getWifiOn()) {
        rect.x = 0; rect.y = 60; rect.w = 640; rect.h = 360;
        drawText(dst, font, rect, languageString(0x8c), g_wifiEmptyColor, 1, false);
        return;
    }
}
int WifiMenuDrawer::count() { return items.size(); }
void WifiMenuDrawer::vfunc13()
{
    for (int i = 0; i < 1; i++) { ItemDrawable *item = items[i]; delete item; }
    for (int i = 1; i < items.size(); i++) { ItemDrawable *item = items[i]; delete item; }
    puts("TextMenu::freeListItem end");
    items.clear();
}
WifiMenu::WifiMenu()
{
    scanCount = 0;
    lock = loadImeImage(g_themePath, "./", "skin/icon-wifi-locked.png");
    check = loadImeImage(g_themePath, "./", "skin/icon-wifi-connected.png");
    signal2 = loadImeImage(g_themePath, "./", "skin/icon-wifi-signal-02.png");
    signal3 = loadImeImage(g_themePath, "./", "skin/icon-wifi-signal-03.png");
    signal4 = loadImeImage(g_themePath, "./", "skin/icon-wifi-signal-04.png");
    if (getWifiConnected() == 0) {
        if (getWifiOn()) {
            int pid = findProcess("hostapd", 0);
            if (pid > 0) { puts("switch to wifi mode"); switch2WpaMode(pid); }
            setWifiScanEnable(1);
        }
        items.push_back(new WifiOnOffLabel(new WifiOnOffDialogAction, true));
    } else {
        items.push_back(new WifiLabel(NULL));
        items.push_back(new WifiOnOffLabel(new WifiOnOffDialogAction, false));
    }
    overlay = createSurface(640, 480);
    if (overlay) SDL_FillRect(overlay, NULL, 0xaa000000);
    connectingIndex = -1;
}
WifiMenu::~WifiMenu()
{
    if (lock) SDL_FreeSurface(lock);
    if (check) SDL_FreeSurface(check);
    if (signal2) SDL_FreeSurface(signal2);
    if (signal3) SDL_FreeSurface(signal3);
    if (signal4) SDL_FreeSurface(signal4);
    if (overlay) SDL_FreeSurface(overlay);
}
void WifiMenu::vfunc7(SDL_Surface *dst)
{
    drawItems(dst);
    if (getWifiConnected()) {
        SDL_Rect src = {0, 0, 640, 330};
        SDL_Rect pos = {0, 90, 0, 0};
        SDL_UpperBlit(overlay, &src, dst, &pos);
    }
}
int WifiMenu::vfunc4()
{
    if (getWifiConnected() == 0) {
        if (cur >= 0) connectingIndex = cur;
    }
    return WifiMenuDrawer::vfunc4();
}
int WifiMenu::vfunc15(int event)
{
    if (event == 28) {
        vfunc13();
        items.push_back(new WifiOnOffLabel(new WifiOnOffDialogAction, true));
        scanCount = copyScanResults(scan, 32);
        printf("get_wifi_scan_result return %d\n", scanCount);
        if (scanCount > 0) {
            int current = -1;
            SDL_Surface *signal = NULL;
            int level = -1;
            for (int i = 0; i < scanCount; i++) {
                if (scan[i].current) {
                    current = i;
                    level = wifiSignalLevel(scan[i].rssi);
                    if (level <= 0) signal = signal2;
                    else if (level == 2) signal = signal3;
                    else signal = signal4;
                    items.push_back(new WifiItem(new WifiItemAction(&scan[i]), scan[i].ssid, scan[i].rssi,
                                                 scan[i].secured, scan[i].current, lock, check, signal));
                    printf("wifi item %s %d %d\n", scan[i].ssid, scan[i].secured, scan[i].current);
                    break;
                }
            }
            for (int i = 0; i < scanCount; i++) {
                if (i != current) {
                    level = wifiSignalLevel(scan[i].rssi);
                    if (level == 0) signal = signal2;
                    else if (level == 1) signal = signal3;
                    else signal = signal4;
                    items.push_back(new WifiItem(new WifiItemAction(&scan[i]), scan[i].ssid, scan[i].rssi,
                                                 scan[i].secured, scan[i].current, lock, check, signal));
                    printf("wifi item %s %d %d\n", scan[i].ssid, scan[i].secured, scan[i].current);
                }
            }
            setWifiScanEnable(0);
        }
    } else if (event == 30) {
        printf("event wifi connected %d\n", connectingIndex);
        for (int i = 1; i < items.size(); i++) {
            WifiItem *item = (WifiItem *)items[i];
            if (i == connectingIndex) item->connected = 1;
            else item->connected = 0;
        }
        UserEvent ev;
        ev.type = 24; ev.code = 24;
        if (SDL_PushEvent(&ev) < 0) printf("%s : %d Failed to push SDL event: %s\n", "onUnhandledEvent", 731, SDL_GetError());
    } else if (event == 39) {
        printf("event wifi connect %d fail\n", connectingIndex);
        for (int i = 1; i < items.size(); i++) {
            WifiItem *item = (WifiItem *)items[i];
            item->connected = 0;
        }
        NotifyWindow *window = new NotifyWindow(languageString(0x9a), languageString(0x2e), "skin/bg-button-f.png", 0);
        window->onShow();
        g_windowStack->push_back(window);
    } else if (event == 42) {
        puts("wifi on off ok"); vfunc13();
        items.push_back(new WifiOnOffLabel(new WifiOnOffDialogAction, true));
    }
    return -1;
}
int HotspotSwitchAction::execute(void *)
{
    int on = getWifiConnected();
    on = !on;
    setWifiConnected(on);
    if (on) {
        puts("turn on ap mode"); swicth2ApMode();
    } else {
        int wifi = getWifiOn();
        if (wifi) {
            printf("turn on wifi mode, wififlag %d\n", wifi);
            int pid = findProcess("hostapd", 0);
            switch2WpaMode(pid);
        } else {
            printf("ap/wifi off, wififlag %d\n", wifi);
            int pid = findProcess("hostapd", 0);
            printf("%s kill hostapd %d\n", "dowork", pid);
            if (pid > 0) kill(pid, 15);
            pid = findProcess("udhcpd", 0);
            printf("%s kill udhcpd %d\n", "dowork", pid);
            if (pid > 0) kill(pid, 9);
        }
    }
    return -1;
}
int writeConfigValue(const std::string &file, const std::string &key, const std::string &value)
{
    std::string command("sed 's/^");
    command += key + "=.\\+$/" + key + "=" + value + "/g' -i " + file;
    return system(command.c_str());
}
std::string readConfigValue(const std::string &file, const std::string &key)
{
    std::ifstream input(file);
    if (!input) { printf("opening %s fail\n", file.c_str()); return ""; }
    std::string line;
    std::string prefix = key + "=";
    while (input >> line) {
        if (line.compare(0, prefix.size(), prefix) == 0) return line.substr(prefix.size());
    }
    return "";
}
int HotspotWriteAction::execute(void *param)
{
    std::string value((char *)param);
    if (writeConfigValue(file, key, value) == 0) {
        if (getWifiConnected()) { swicth2ApMode(); return 1; }
    }
    return -1;
}
int HotspotImePopupAction::execute(void *)
{
    ImeWindow *window = new ImeWindow(title, new HotspotWriteAction(file, key), 0x5d);
    g_windowStack->push_back((TrimusWindow *)window);
    return -1;
}
HotspotMenu::HotspotMenu()
{
    items.push_back(new HotspotSwitchItem);
    items.push_back(new HotspotNameItem);
    items.push_back(new HotspotPskItem);
}
HotspotMenu::~HotspotMenu()
{
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
}
void HotspotMenu::vfunc7(SDL_Surface *dst) { drawItems(dst); }
int HotspotMenu::vfunc15(int event) { return -1; }
