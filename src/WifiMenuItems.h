// The custom-drawn rows of the Wi-Fi and hotspot screens (WifiMenus.cpp).
#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include "Action.h"
#include "TextMenuDrawer.h"
#include "ImeWindow.h"

// Shared literal identities preserve first-use order in the Wi-Fi owner's read-only data.
extern const char g_wifiSkinRoot[], g_wifiArrowLeft[], g_wifiArrowRight[];
extern const char g_hotspotSwitchFormat[], g_hotspotSsidKey[], g_hotspotConfigPath[], g_hotspotPskKey[];

struct WifiAP {
    char unused0[8];
    char ssid[0x58];
    int rssi;
    char unused64[8];
    int freq;
    char unused70[4];
    char bssid[0x18];
    char flags[0x80];
    int secured, current;
};
#include "WifiStatus.h"
extern char *g_themePath;
extern void *g_imeFont;
extern SDL_Color g_imeTextColor, g_wifiSelectedColor;
SDL_Surface *loadImeImage(const char *, const char *, const char *);
const char *languageString(int);
int drawText(SDL_Surface *, void *, SDL_Rect, const char *, SDL_Color, int, bool);


extern "C" int getHotspotOn();
std::string readConfigValue(const std::string &, const std::string &);
class ItemDrawable
{
public:
    ItemDrawable(Action *a) : action(a) {}
    virtual ~ItemDrawable()
    {
        if (action) delete action;
    }
    virtual int draw(SDL_Surface *, SDL_Rect *, bool) = 0;
    virtual bool selectable() { return true; }
    virtual int event(int) { return -1; }
    Action *action;
};
class WifiOnOffLabel : public ItemDrawable
{
public:
    WifiOnOffLabel(Action *, bool);
    virtual ~WifiOnOffLabel();
    virtual int draw(SDL_Surface *, SDL_Rect *, bool);
    virtual bool selectable() { return enabled; }
    SDL_Surface *left, *right;
    bool enabled;
};
class WifiLabel : public ItemDrawable
{
public:
    WifiLabel(Action *);
    virtual ~WifiLabel();
    virtual int draw(SDL_Surface *, SDL_Rect *, bool);
    virtual bool selectable() { return false; }
};
class WifiItem : public ItemDrawable
{
public:
    WifiItem(Action *a, const char *s, int r, int sec, int connected,
             SDL_Surface *lock, SDL_Surface *check, SDL_Surface *sig)
        : ItemDrawable(a), ssid(s), rssi(r), secured(sec), connected(connected),
          lock(lock), check(check), signal(sig) {}
    virtual ~WifiItem() {}
    virtual int draw(SDL_Surface *, SDL_Rect *, bool);
    std::string ssid;
    int rssi, secured, connected;
    SDL_Surface *lock, *check, *signal;
};
