// Wi-Fi/hotspot menus and actions; inline definition order fixes the template tail.
#pragma once
#include "WifiMenuItems.h"

class WifiMenuDrawer : public ListMenu {
public:
    WifiMenuDrawer();
    virtual ~WifiMenuDrawer();
    virtual int vfunc3();
    virtual int vfunc4();
    virtual void *getItem(int index);
    virtual int count();
    virtual void vfunc11() {}
    virtual void vfunc13();
    void vfunc7(SDL_Surface *) override;
    int vfunc15(int) override = 0;
    std::vector<ItemDrawable *> items;
    SDL_Surface *background;
    int rowHeight;
};
// Original declaration order places menu RTTI/vtables after the action classes.
class WifiMenu : public WifiMenuDrawer {
public:
    WifiMenu();
    virtual ~WifiMenu();
    virtual void vfunc7(SDL_Surface *);
    virtual int vfunc4();
    virtual int vfunc15(int event);
    SDL_Surface *lock, *check, *signal2, *signal3, *signal4;
    WifiAP scan[32];
    int scanCount, connectingIndex;
    SDL_Surface *overlay;
};
class HotspotMenu : public WifiMenuDrawer {
public:
    HotspotMenu();
    virtual ~HotspotMenu();
    virtual void vfunc7(SDL_Surface *);
    virtual int vfunc15(int event);
};

class WifiConnectAction : public Action {
public:
    WifiConnectAction(WifiAP *source, const char *password, bool secured)
        : Action(-1), ap(*source) {
        this->secured = secured;
        if (password) psk = password;
    }
    virtual ~WifiConnectAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }
    WifiAP ap; std::string psk; bool secured;
};
class WifiConnectDialogAction : public Action {
public:
    WifiConnectDialogAction(WifiAP *source) : Action(-1), ap(*source) {}
    virtual ~WifiConnectDialogAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }
    WifiAP ap;
};
class WifiItemAction : public Action {
public:
    WifiItemAction(WifiAP *source) : Action(-1), ap(*source) {}
    virtual ~WifiItemAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }
    WifiAP ap;
};
class WifiOnOffAction : public Action {
public:
    WifiOnOffAction() : Action(-1) {}
    virtual ~WifiOnOffAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }

};
class WifiOnOffDialogAction : public Action {
public:
    WifiOnOffDialogAction() : Action(-1) {}
    virtual ~WifiOnOffDialogAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }

};
class HotspotSwitchAction : public Action {
public:
    HotspotSwitchAction() : Action(-1) {}
    virtual ~HotspotSwitchAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }

};
class HotspotSwitchItem : public ItemDrawable {
public:
    HotspotSwitchItem() : ItemDrawable(NULL) {
        action = new HotspotSwitchAction;
        left = loadImeImage(g_themePath, g_wifiSkinRoot, g_wifiArrowLeft);
        right = loadImeImage(g_themePath, g_wifiSkinRoot, g_wifiArrowRight);
    }
    virtual ~HotspotSwitchItem() { if (left) SDL_FreeSurface(left); if (right) SDL_FreeSurface(right); }
    virtual int draw(SDL_Surface *dst, SDL_Rect *r, bool selected) {
        SDL_Rect rect = *r;
        rect.x += 20; rect.y += 5;
        drawText(dst, g_imeFont, rect, languageString(0x86), selected ? g_wifiSelectedColor : g_imeTextColor, 3, false);
        int on = getWifiConnected();
        printf(g_hotspotSwitchFormat, on);
        rect.w = 200;
        rect.x = r->x + r->w - rect.w - (right ? right->w : 0) - 40;
        if (on) drawText(dst, g_imeFont, rect, languageString(0x68), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
        else drawText(dst, g_imeFont, rect, languageString(0x69), selected ? g_wifiSelectedColor : g_imeTextColor, 1, false);
        if (left && right) {
            rect.y += (60 - left->h) / 2;
            rect.x = rect.x + rect.w - (right ? right->w : 0) - left->w - 240;
            SDL_UpperBlit(left, NULL, dst, &rect);
            rect.y += (60 - right->h) / 2;
            rect.x = rect.x + rect.w - right->w - 40;
            SDL_UpperBlit(right, NULL, dst, &rect);
        }
        return 0;
    }
    SDL_Surface *left, *right;
};
class HotspotWriteAction : public Action {
public:
    HotspotWriteAction(const std::string &f, const std::string &k) : Action(-1), file(f), key(k) {}
    virtual ~HotspotWriteAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }
    std::string file, key;
};
class HotspotImePopupAction : public Action {
public:
    HotspotImePopupAction(int t, const char *f, const char *k) : Action(-1), file(f), key(k), title(t) {}
    virtual ~HotspotImePopupAction() {}
    virtual int execute(void *);
    virtual int progress(int *, int) { return -1; }
    virtual int finished() { return 0; }
    std::string file, key;
    int title;
};
class HotspotNameItem : public ItemDrawable {
public:
    HotspotNameItem() : ItemDrawable(NULL) { action = new HotspotImePopupAction(0x97, g_hotspotConfigPath, g_hotspotSsidKey); }
    virtual ~HotspotNameItem() {}
    virtual int draw(SDL_Surface *dst, SDL_Rect *r, bool selected) {
        SDL_Rect rect = *r;
        rect.x += 20;
        drawText(dst, g_imeFont, rect, languageString(135), g_imeTextColor, 0, selected);
        // Preserve the original temporary-string lifetime.
        const char *value = readConfigValue(g_hotspotConfigPath, g_hotspotSsidKey).c_str();
        int width = 0, height = 0;
        TTF_SizeText(g_imeFont, value, &width, &height);
        if (width < 80) width = 80;
        rect.x = r->x + r->w - width - 5;
        drawText(dst, g_imeFont, rect, value, g_imeTextColor, 0, false);
        return 0;
    }
};
class HotspotPskItem : public ItemDrawable {
public:
    HotspotPskItem() : ItemDrawable(NULL) { action = new HotspotImePopupAction(0x98, g_hotspotConfigPath, g_hotspotPskKey); }
    virtual ~HotspotPskItem() {}
    virtual int draw(SDL_Surface *dst, SDL_Rect *r, bool selected) {
        SDL_Rect rect = *r;
        rect.x += 20;
        drawText(dst, g_imeFont, rect, languageString(136), g_imeTextColor, 0, selected);
        // Preserve the original temporary-string lifetime.
        const char *value = readConfigValue(g_hotspotConfigPath, g_hotspotPskKey).c_str();
        int width = 0, height = 0;
        TTF_SizeText(g_imeFont, value, &width, &height);
        if (width < 80) width = 80;
        rect.x = r->x + r->w - width - 5;
        drawText(dst, g_imeFont, rect, value, g_imeTextColor, 0, false);
        return 0;
    }
};
