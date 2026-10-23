// WifiMenus.cpp (0x138c50-0x13edfc): the Wi-Fi and hotspot settings screens.
//
// WifiMenu lists the Wi-Fi on/off switch and, once a scan finishes, the networks found (signal
// bars, lock, check mark for the current one). Choosing a network asks for its password if it is
// secured (ImeWindow) and connects through the wpa_cli backend in Wifi.c, behind a progress
// dialog. HotspotMenu switches the device into access-point mode (hostapd + udhcpd) and edits the
// hotspot's name and password in /etc/wifi/hostapd.conf. Wi-Fi and hotspot exclude each other.
// Results come back to the menus as user events (Input.h) through the ListMenu event slot.
//
// Body 14,368 bytes plus 122 inline/template functions. Keep the ignored system() result stores
// and the conditional returns: they affect -O0 layout.
#include "PlatformApi.h"
#include "SdlApi.h"
#include "CJsonApi.h"
#include "LangStrings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include "WifiMenuItems.h"
#include "Input.h"
int findProcess(const char *, int);

// What the Wi-Fi chip is doing, judged by which daemon runs (WifiMode, PlatformApi.h).
int wifiMode()
{
    int pid = findProcess("wpa_supplicant", 0);
    if (pid >= 0) return WIFI_MODE_STATION;
    pid = findProcess("hostapd", 0);
    if (pid >= 0) return WIFI_MODE_HOTSPOT;
    return WIFI_MODE_OFF;
}
// Establish the body unwind section before action/menu inline definitions.
#include "WifiMenus.h"

extern "C" {
int getWifiOn();
void setWifiOn(int);
int getHotspotOn();
void setHotspotOn(int);
int setWifiScanEnable(int);
int wifiConnectOpen(const char *);
int wifiConnectPsk(const char *, const char *);
WifiStatus queryWifiStatus();
WifiStatus *getWifiStatus();
int wifiStartConnect();
int copyScanResults(WifiAP *, int);
int wifiSignalLevel(int);
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
class ProgressDialog : public TrimusWindow
{
public:
    ProgressDialog(int, Action *);
    virtual ~ProgressDialog();
    virtual int handleInput();
    virtual int draw(SDL_Surface *);
    virtual void onHide();
    virtual void onShow();
    virtual void saveState(cJSON *);
    virtual int restoreState(cJSON *);
    virtual void update();
    char raw[0x3c - 12];
};
class NotifyWindow : public TrimusWindow
{
public:
    NotifyWindow(const char *, const char *, const char *, void (*)());
    virtual ~NotifyWindow();
    virtual int handleInput();
    virtual int draw(SDL_Surface *);
    virtual void onHide();
    virtual void onShow();
    virtual void saveState(cJSON *);
    virtual int restoreState(cJSON *);
    virtual void update();
    char raw[0x3c - 12];
};

static_assert(sizeof(WifiAP) == 0x114, "WifiAP layout");
static_assert(sizeof(WifiStatus) == 0x74, "WifiStatus layout");
static_assert(sizeof(ListMenu) == 0x48 && sizeof(TextMenuDrawer) == 0xf8, "menu base layouts");
static_assert(sizeof(WifiMenuDrawer) == 0x5c, "WifiMenuDrawer layout");
static_assert(sizeof(WifiMenu) == 0x22fc && sizeof(HotspotMenu) == 0x5c, "Wi-Fi menu layouts");
static_assert(sizeof(WifiItem) == 0x38 && sizeof(WifiConnectAction) == 0x138, "Wi-Fi item/action layouts");
static_assert(sizeof(ProgressDialog) == 0x3c && sizeof(NotifyWindow) == 0x3c, "external window layouts");


// Original function-name objects follow local RTTI and precede inline-row COMDAT data.
extern const char wifi_function_name_0[];
extern const char wifi_function_name_1[];
extern const char wifi_function_name_2[];
extern const char wifi_function_name_3[];
extern const char wifi_function_name_4[];
extern const char wifi_function_name_5[];

// The Wi-Fi interface's IPv4 address, from `ip addr show wlan0`. Returns 0, or -1 if none.
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
                    if (end) {
                        *end = 0;
                        address = ip + 5;
                        result = 0;
                    }
                }
            }
            fclose(f);
        }
    }
    return result;
}
// Hotspot -> Wi-Fi client: stops hostapd (pid) and the DHCP server, restarts wpa_supplicant
// and the DHCP client, and saves the MAC address to /tmp/wifimac.
void switch2WpaMode(int pid)
{
    if (pid > 0) {
        printf("%s kill hostapd %d\n", wifi_function_name_0, pid);
        kill(pid, 15);
    }
    int dhcp = findProcess("udhcpd", 0);
    printf("%s kill udhcpd %d\n", wifi_function_name_0, dhcp);
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
// Wi-Fi client -> hotspot: stops wpa_supplicant and the DHCP client, gives wlan0 the address
// 192.168.0.18 and starts hostapd and the DHCP server. (The misspelt name is the original's;
// it survives in its log messages.)
void swicth2ApMode()
{
    int pid = findProcess("wpa_supplicant", 0);
    printf("%s kill wpa_supplicant %d\n", wifi_function_name_1, pid);
    if (pid > 0) kill(pid, 15);
    printf("%s kill udhcpc %d\n", wifi_function_name_1, pid);
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
// Original arrow/root literals first occur with this constructor.
extern const char g_wifiArrowLeft[] __attribute__((aligned(4))) = "skin/icon-left-arrow-24.png";
extern const char g_wifiSkinRoot[] __attribute__((aligned(4))) = "./";
extern const char g_wifiArrowRight[] __attribute__((aligned(4))) = "skin/icon-right-arrow-24.png";

// The "Wi-Fi  < On/Off >" row; not selectable while the hotspot is on.
WifiOnOffLabel::WifiOnOffLabel(Action *act, bool enabled) : ItemDrawable(act)
{
    left = loadImeImage(g_themePath, g_wifiSkinRoot, g_wifiArrowLeft);
    right = loadImeImage(g_themePath, g_wifiSkinRoot, g_wifiArrowRight);
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
    drawText(dst, g_imeFont, rect, languageString(STR_WIFI), selected ? g_wifiSelectedColor : g_imeTextColor, 3, false);
    rect.w = 200;
    rect.x = r->x + r->w - rect.w - (right ? right->w : 0) - 40;
    int on = getWifiOn();
    if (on)
        drawText(dst, g_imeFont, rect, languageString(STR_ON), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
    else
        drawText(dst, g_imeFont, rect, languageString(STR_OFF), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
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
// A white heading row shown above the switch while the hotspot is on.
WifiLabel::WifiLabel(Action *act) : ItemDrawable(act) {}
WifiLabel::~WifiLabel() {}
int WifiLabel::draw(SDL_Surface *dst, SDL_Rect *r, bool selected)
{
    SDL_Rect rect = *r;
    SDL_FillRect(dst, r, SDL_MapRGBA(dst->format, 255, 255, 255, 255));
    rect.x += 15;
    drawText(dst, g_imeFont, rect, languageString(STR_HOTSPOT_IS_ON), g_wifiSelectedColor, 3, false);
    return 0;
}
// One network: its name on the left; signal bars, lock (secured) and check mark (connected)
// right-aligned.
int WifiItem::draw(SDL_Surface *dst, SDL_Rect *r, bool selected)
{
    SDL_Rect rect = *r;
    rect.x += 20;
    drawText(dst, g_imeFont, rect, ssid.c_str(), selected ? g_wifiSelectedColor : g_imeTextColor, 3, false);
    rect.x = r->x + r->w - 20;
    if (signal) {
        rect.y = r->y + (r->h - signal->h) / 2;
        rect.x -= signal->w;
        rect.w = signal->w;
        rect.h = signal->h;
        SDL_UpperBlit(signal, NULL, dst, &rect);
        rect.x -= 5;
    }
    if (secured && lock) {
        rect.y = r->y + (r->h - lock->h) / 2;
        rect.x -= lock->w;
        rect.w = lock->w;
        rect.h = lock->h;
        SDL_UpperBlit(lock, NULL, dst, &rect);
        rect.x -= 5;
    }
    if (connected && check) {
        rect.y = r->y + (r->h - check->h) / 2;
        rect.x -= check->w;
        rect.w = check->w;
        rect.h = check->h;
        SDL_UpperBlit(check, NULL, dst, &rect);
    }
    return 0;
}
// Runs behind the progress dialog: joins the network, then checks the state once a second for
// up to 11 s while it is scanning or disabled. Success starts DHCP (and turns the hotspot off)
// and posts EVENT_WIFI_CONNECTED; anything else posts EVENT_WIFI_CONNECT_FAILED.
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
    SDL_UserEvent ev;
    do {
        usleep(1000000);
        status = queryWifiStatus();
        printf("%p connect result stage=%d, status=%d, ssid=%s, ip=%s, sig=(%d,%d,%d,%d), id=%d\n",
               this, status.state, status.status, status.ssid, status.ip, status.rssi, status.linkSpeed, status.noise, status.freq, status.id);
        i++;
    } while ((status.state == WIFI_STATE_SCANNING || status.state == WIFI_STATE_DISABLED) && i < 11);
    if (status.state == WIFI_STATE_COMPLETED) {
        puts("try dhcp");
        wifiStartConnect();
        setHotspotOn(0);
        usleep(2000000);
        ev.type = SDL_USEREVENT;
        ev.code = EVENT_WIFI_CONNECTED;
    } else {
        ev.type = SDL_USEREVENT;
        ev.code = EVENT_WIFI_CONNECT_FAILED;
    }
    if (SDL_PushEvent(&ev) < 0) printf("%s %d Failed to push SDL event %s\n", wifi_function_name_2, 241, SDL_GetError());
    return -1;
}
// The password keyboard's OK: connect with the typed password (param) behind a progress dialog.
int WifiConnectDialogAction::execute(void *param)
{
    ProgressDialog *window = new ProgressDialog(STR_WIFI, new WifiConnectAction(&ap, (char *)param, true));
    pushWindow(window);
    return 4;
}
// A network was chosen. Unless it is the current one: ask for the password if it is secured,
// otherwise connect straight away.
int WifiItemAction::execute(void *)
{
    WifiStatus *status = getWifiStatus();
    if (!status || strcmp(status->ssid, ap.ssid) != 0) {
        if (ap.secured) {
            ImeWindow *window = new ImeWindow(STR_WIFI, new WifiConnectDialogAction(&ap), STR_CONNECT);
            g_windowStack->push_back((TrimusWindow *)window);
        } else {
            ProgressDialog *window = new ProgressDialog(STR_WIFI, new WifiConnectAction(&ap, NULL, false));
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
        }
    }
    return -1;
}
// Switches Wi-Fi on (power up the chip, start wpa_supplicant and DHCP) or off, and saves the
// setting to system.json.
void toggleWifi()
{
    int on = getWifiOn();
    on = !on;
    int rc;
    setWifiOn(on);
    if (on) {
        puts("wifi on");
        rc = system("/customer/app/axp_test wifion");
        rc = system("sleep 2");
        rc = system("ifconfig wlan0 down");
        rc = system("killall -15 wpa_supplicant");
        rc = system("killall -15 udhcpc");
        rc = system("ifconfig wlan0 up");
        rc = system("wpa_supplicant -B -D nl80211 -iwlan0 -c /appconfigs/wpa_supplicant.conf");
        rc = system("udhcpc -i wlan0 -s /etc/init.d/udhcpc.script &");
        rc = system("touch /tmp/audioserver_restart");
    } else {
        puts("wifi off");
        rc = system("ifconfig wlan0 down");
        rc = system("killall -15 wpa_supplicant");
        rc = system("killall -15 udhcpc");
        rc = system("sleep 0.5");
        rc = system("/customer/app/axp_test wifioff");
    }
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
}
// Runs behind the progress dialog (not while the hotspot is on). After switching on, it waits
// up to 10 s for the first scan and passes EVENT_WIFI_SCAN_DONE on to the menu; otherwise it
// posts EVENT_WIFI_SWITCHED (or EVENT_WIFI_SWITCH_TIMEOUT).
int WifiOnOffAction::execute(void *)
{
    SDL_UserEvent out;
    out.type = SDL_USEREVENT;
    out.code = EVENT_WIFI_SWITCHED;
    if (getHotspotOn() == 0) {
        toggleWifi();
        int on = getWifiOn();
        printf("wifi on/off action %d\n", on);
        if (on) {
            setWifiScanEnable(1);
            unsigned int start = milliseconds();
            bool done = false;
            SDL_Event ev;
            while (SDL_WaitEvent(&ev)) {
                switch (ev.type) {
                    case SDL_USEREVENT:
                        if (ev.user.code == EVENT_WIFI_SCAN_DONE) {
                            puts("wifi on/off scan finish");
                            out.code = EVENT_WIFI_SCAN_DONE;
                            if (SDL_PushEvent(&out) < 0) printf("%s : %d Failed to push SDL event: %s\n", wifi_function_name_3, 362, SDL_GetError());
                            return -1;
                        }
                    default:
                        break;
                }
                unsigned int now = milliseconds();
                if (now - start > 10000) {
                    puts("wifi on/off timeout");
                    out.code = EVENT_WIFI_SWITCH_TIMEOUT;
                    break;
                }
            }
        } else
            setWifiScanEnable(0);
    }
    if (SDL_PushEvent(&out) < 0) printf("%s : %d Failed to push SDL event: %s\n", wifi_function_name_3, 386, SDL_GetError());
    return -1;
}
// Choosing the switch row: run WifiOnOffAction behind a progress dialog.
int WifiOnOffDialogAction::execute(void *)
{
    puts("wifi on/off");
    ProgressDialog *window = new ProgressDialog(STR_WIFI, new WifiOnOffAction);
    pushWindow(window);
    return -1;
}
// Shared base of both screens: a list of ItemDrawable rows, 60 px high, below the title bar.
// The ListMenu slots it overrides: handleKey (a button), onSelect (activate the row), drawList,
// clearItems (free the rows) and onUnhandledEvent (user events; an original name).
WifiMenuDrawer::WifiMenuDrawer() : ListMenu(false)
{
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    rowHeight = 60;
    background = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/bg-list-s.png");
}
WifiMenuDrawer::~WifiMenuDrawer()
{
    if (background) SDL_FreeSurface(background);
    clearItems();
}
// A button: ListMenu's handling, except that left/right on the first row (the "< On/Off >"
// switch) runs its action.
int WifiMenuDrawer::handleKey()
{
    int action = ListMenu::handleKey();
    if (action == LIST_LEFT || action == LIST_RIGHT) {
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
// Activate: run the selected row's action.
int WifiMenuDrawer::onSelect()
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
// Draws the visible page of six rows (scrolling with the cursor), the stripes behind the first
// five, the highlight, and "no network" text when Wi-Fi is on but nothing was found.
void WifiMenuDrawer::drawList(SDL_Surface *dst)
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
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
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
            rect.x = 0;
            rect.y = (index + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &rect);
        }
        if (index == cur && item->selectable()) {
            if (background) {
                pos.x = x;
                if (rowHeight > background->h)
                    pos.y = (rowHeight - background->h) / 2 + y;
                else
                    pos.y = y;
                rect.x = 0;
                rect.y = 0;
                rect.w = this->rect.w;
                rect.h = rowHeight;
                SDL_UpperBlit(background, &rect, dst, &pos);
            }
        }
        rect.x = x;
        rect.y = y;
        rect.w = this->rect.w;
        rect.h = rowHeight;
        if (item) item->draw(dst, &rect, item->selectable() ? index == cur : false);
        index++;
        row++;
    }
    if (count() <= 1 && getWifiOn()) {
        rect.x = 0;
        rect.y = 60;
        rect.w = 640;
        rect.h = 360;
        drawText(dst, font, rect, languageString(STR_SCANNING), g_wifiEmptyColor, 1, false);
        return;
    }
}
int WifiMenuDrawer::count()
{ return items.size(); }
// Deletes every row (the first one separately, as the original does).
void WifiMenuDrawer::clearItems()
{
    for (int i = 0; i < 1; i++) {
        ItemDrawable *item = items[i];
        delete item;
    }
    for (int i = 1; i < items.size(); i++) {
        ItemDrawable *item = items[i];
        delete item;
    }
    puts("TextMenu::freeListItem end");
    items.clear();
}
// Loads the icons, switches from hotspot back to Wi-Fi if needed and starts scanning; the
// network rows arrive with EVENT_WIFI_SCAN_DONE. The overlay is a translucent black sheet.
WifiMenu::WifiMenu()
{
    scanCount = 0;
    lock = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/icon-wifi-locked.png");
    check = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/icon-wifi-connected.png");
    signal2 = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/icon-wifi-signal-02.png");
    signal3 = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/icon-wifi-signal-03.png");
    signal4 = loadImeImage(g_themePath, g_wifiSkinRoot, "skin/icon-wifi-signal-04.png");
    if (getHotspotOn() == 0) {
        if (getWifiOn()) {
            int pid = findProcess("hostapd", 0);
            if (pid > 0) {
                puts("switch to wifi mode");
                switch2WpaMode(pid);
            }
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
// While the hotspot is on, the list is dimmed by the overlay.
void WifiMenu::drawList(SDL_Surface *dst)
{
    WifiMenuDrawer::drawList(dst);
    if (getHotspotOn()) {
        SDL_Rect src = {0, 0, 640, 330};
        SDL_Rect pos = {0, 90, 0, 0};
        SDL_UpperBlit(overlay, &src, dst, &pos);
    }
}
// Remembers which row is being connected, for the check mark.
int WifiMenu::onSelect()
{
    if (getHotspotOn() == 0) {
        if (cur >= 0) connectingIndex = cur;
    }
    return WifiMenuDrawer::onSelect();
}
// Wi-Fi events: rebuild the rows from the scan (current network first), move the check mark
// after a successful connect, or show "connection failed".
// Original quirk, kept as-is: the current network's bars use different thresholds from the
// others (level <= 0 / == 2 instead of == 0 / == 1).
int WifiMenu::onUnhandledEvent(int event)
{
    if (event == EVENT_WIFI_SCAN_DONE) {
        clearItems();
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
                    if (level <= 0)
                        signal = signal2;
                    else if (level == 2)
                        signal = signal3;
                    else
                        signal = signal4;
                    items.push_back(new WifiItem(new WifiItemAction(&scan[i]), scan[i].ssid, scan[i].rssi,
                                                 scan[i].secured, scan[i].current, lock, check, signal));
                    printf("wifi item %s %d %d\n", scan[i].ssid, scan[i].secured, scan[i].current);
                    break;
                }
            }
            for (int i = 0; i < scanCount; i++) {
                if (i != current) {
                    level = wifiSignalLevel(scan[i].rssi);
                    if (level == 0)
                        signal = signal2;
                    else if (level == 1)
                        signal = signal3;
                    else
                        signal = signal4;
                    items.push_back(new WifiItem(new WifiItemAction(&scan[i]), scan[i].ssid, scan[i].rssi,
                                                 scan[i].secured, scan[i].current, lock, check, signal));
                    printf("wifi item %s %d %d\n", scan[i].ssid, scan[i].secured, scan[i].current);
                }
            }
            setWifiScanEnable(0);
        }
    } else if (event == EVENT_WIFI_CONNECTED) {
        printf("event wifi connected %d\n", connectingIndex);
        for (int i = 1; i < items.size(); i++) {
            WifiItem *item = (WifiItem *)items[i];
            if (i == connectingIndex)
                item->connected = 1;
            else
                item->connected = 0;
        }
        SDL_UserEvent ev;
        ev.type = SDL_USEREVENT;
        ev.code = EVENT_REDRAW;
        if (SDL_PushEvent(&ev) < 0) printf("%s : %d Failed to push SDL event: %s\n", wifi_function_name_4, 731, SDL_GetError());
    } else if (event == EVENT_WIFI_CONNECT_FAILED) {
        printf("event wifi connect %d fail\n", connectingIndex);
        for (int i = 1; i < items.size(); i++) {
            WifiItem *item = (WifiItem *)items[i];
            item->connected = 0;
        }
        NotifyWindow *window = new NotifyWindow(languageString(STR_CONNECT_FAILED_TEXT), languageString(STR_OK), "skin/bg-button-f.png", 0);
        window->onShow();
        g_windowStack->push_back(window);
    } else if (event == EVENT_WIFI_SWITCHED) {
        puts("wifi on off ok");
        clearItems();
        items.push_back(new WifiOnOffLabel(new WifiOnOffDialogAction, true));
    }
    return -1;
}
// The hotspot switch: on switches to access-point mode; off returns to Wi-Fi (if it is on) or
// just stops hostapd and the DHCP server.
int HotspotSwitchAction::execute(void *)
{
    int on = getHotspotOn();
    on = !on;
    setHotspotOn(on);
    if (on) {
        puts("turn on ap mode");
        swicth2ApMode();
    } else {
        int wifi = getWifiOn();
        if (wifi) {
            printf("turn on wifi mode, wififlag %d\n", wifi);
            int pid = findProcess("hostapd", 0);
            switch2WpaMode(pid);
        } else {
            printf("ap/wifi off, wififlag %d\n", wifi);
            int pid = findProcess("hostapd", 0);
            printf("%s kill hostapd %d\n", wifi_function_name_5, pid);
            if (pid > 0) kill(pid, 15);
            pid = findProcess("udhcpd", 0);
            printf("%s kill udhcpd %d\n", wifi_function_name_5, pid);
            if (pid > 0) kill(pid, 9);
        }
    }
    return -1;
}
extern const char g_hotspotSwitchFormat[] __attribute__((aligned(4))) = "hotspot switch flag %d\n";

// Replaces "key=..." with "key=value" in a config file (sed -i); returns system()'s result.
// The value is not escaped.
int writeConfigValue(const std::string &file, const std::string &key, const std::string &value)
{
    std::string command("sed 's/^");
    command += key + "=.\\+$/" + key + "=" + value + "/g' -i " + file;
    return system(command.c_str());
}
// The value of the first "key=..." word in a config file, or "".
std::string readConfigValue(const std::string &file, const std::string &key)
{
    std::ifstream input(file);
    if (!input) {
        printf("opening %s fail\n", file.c_str());
        return "";
    }
    std::string line;
    std::string prefix = key + "=";
    while (input >> line) {
        if (line.compare(0, prefix.size(), prefix) == 0) return line.substr(prefix.size());
    }
    return "";
}
// The keyboard's OK for the hotspot name or password: write it, and restart the hotspot if on.
int HotspotWriteAction::execute(void *param)
{
    std::string value((char *)param);
    if (writeConfigValue(file, key, value) == 0) {
        if (getHotspotOn()) {
            swicth2ApMode();
            return 1;
        }
    }
    return -1;
}
// Opens the keyboard to edit a hotspot setting.
int HotspotImePopupAction::execute(void *)
{
    ImeWindow *window = new ImeWindow(title, new HotspotWriteAction(file, key), 0x5d);
    g_windowStack->push_back((TrimusWindow *)window);
    return -1;
}
// Original inline-row literal pool follows configuration helpers.
extern const char g_hotspotSsidKey[] __attribute__((aligned(4))) = "ssid";
extern const char g_hotspotConfigPath[] __attribute__((aligned(4))) = "/etc/wifi/hostapd.conf";
extern const char g_hotspotPskKey[] __attribute__((aligned(4))) = "wpa_passphrase";

// Three rows: the hotspot switch, its name (ssid) and its password (wpa_passphrase).
HotspotMenu::HotspotMenu()
{
    items.push_back(new HotspotSwitchItem);
    items.push_back(new HotspotNameItem);
    items.push_back(new HotspotPskItem);
}
// Saves system.json on close.
HotspotMenu::~HotspotMenu()
{
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
}
void HotspotMenu::drawList(SDL_Surface *dst)
{ WifiMenuDrawer::drawList(dst); }
int HotspotMenu::onUnhandledEvent(int event)
{ return -1; }


// __func__ strings of the original functions, used in the log messages above.
const char wifi_function_name_0[] __attribute__((section(".rodata.function_names"), aligned(4))) = "switch2WpaMode";
const char wifi_function_name_1[] __attribute__((section(".rodata.function_names"), aligned(4))) = "swicth2ApMode";
const char wifi_function_name_2[] __attribute__((section(".rodata.function_names"), aligned(4))) = "dowork";
const char wifi_function_name_3[] __attribute__((section(".rodata.function_names"), aligned(4))) = "dowork";
const char wifi_function_name_4[] __attribute__((section(".rodata.function_names"), aligned(4))) = "onUnhandledEvent";
const char wifi_function_name_5[] __attribute__((section(".rodata.function_names"), aligned(4))) = "dowork";
