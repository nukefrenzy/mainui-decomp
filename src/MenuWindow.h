// MenuWindow: one screen holding a ListMenu. sizeof == 0x11c.
// Offsets verified byte-exact against MainUI-354-clean (see MATCHES.txt).
#pragma once
#include <string>
#include "TrimusWindow.h"
#include "MenuHelpers.h"

class ListMenu;   // IconMenu, TextMenu, FileBrowserMenu, ... (+0x0c current item, +0x10 page start, +0x14 page end)

class MenuWindow : public TrimusWindow {
public:
    MenuWindow(int titleLabelId, int id, ListMenu *menu);    // 0x2e6d4
    virtual ~MenuWindow();                                    // 0x2ec3c (D1), 0x2ed78 (D0)
    MenuWindow(const char *path, int id, void *emuEntry);    // 0x2e978 (ROM folder)
    virtual int  handleInput();                  // 0x2ff04
    virtual int  draw(SDL_Surface *dst);         // 0x31398
    virtual void onHide();                       // 0x2fc74
    virtual void onShow();                       // 0x2f290 builds `menu` lazily
    virtual void saveState(cJSON *obj);          // 0x2edac currpos/pagestart/pageend/emuname/raname/ppath/searchstr
    virtual int  restoreState(cJSON *obj);       // 0x2efcc
    virtual void update();                       // 0x2fb14
    void setStrB4(const std::string &s) { strB4 = s; }   // 0x3aafc, inline

    ListMenu    *menu;            // +0x0c  not freed by the window
    int          savedCurPos;     // +0x10  restored into the list on show
    int          savedPageStart;  // +0x14
    int          savedPageEnd;    // +0x18
    bool         flag1c;          // +0x1c
    bool         flag1d;          // +0x1d  set in onShow
    std::string  str20;           // +0x20
    int          field38;         // +0x38
    std::string  str3c;           // +0x3c
    bool         field54;         // +0x54  initialised to true
    SDL_Surface *surface58;       // +0x58  freed in destructor
    void        *emuEntry;        // +0x5c  emulator / RetroArch app entry for ROM lists (ids 5, 0x11)
    MenuHelper60 helper60;        // +0x60
    MenuHelper80 helper80;        // +0x80
    int          field94;         // +0x94  not touched by the constructor
    std::string  str98;           // +0x98
    SDL_Surface *iconTF;          // +0xb0  skin/icon-TF.png, freed in destructor
    std::string  strB4;           // +0xb4  used when building ROM lists
    std::string  path;            // +0xcc  folder path passed to createMenu
    std::string  themePath;       // +0xe4  copy of g_themePath
    SDL_Surface *surfaceFC;       // +0xfc  freed only if ownsSurfaceFC
    std::string  str100;          // +0x100
    bool         ownsSurfaceFC;   // +0x118
};
