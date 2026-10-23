// MenuWindow: one screen holding a ListMenu. sizeof == 0x11c.
#pragma once
#include <string>
#include "CJsonApi.h"
#include "TrimusWindow.h"
#include "MenuHelpers.h"

class ListMenu;   // IconMenu, TextMenu, FileBrowserMenu, ... (+0x0c current item, +0x10 page start, +0x14 page end)

class MenuWindow : public TrimusWindow
{
public:
    MenuWindow(int titleLabelId, int id, void *emuEntry);    // 0x2e424 (by menu id)
    MenuWindow(int titleLabelId, int id, ListMenu *menu);    // 0x2e6d4
    virtual ~MenuWindow();                                    // 0x2ec3c (D1), 0x2ed78 (D0)
    MenuWindow(const char *path, int id, void *emuEntry);    // 0x2e978 (ROM folder)
    virtual int handleInput();                  // 0x2ff04
    virtual int draw(SDL_Surface *dst);         // 0x31398
    virtual void onHide();                       // 0x2fc74
    virtual void onShow();                       // 0x2f290 builds `menu` lazily
    virtual void saveState(cJSON *obj);          // 0x2edac currpos/pagestart/pageend/emuname/raname/ppath/searchstr
    virtual int restoreState(cJSON *obj);       // 0x2efcc
    virtual void update();                       // 0x2fb14
    void updateThumbnail();                            // 0x2f6c4
    void cacheBackground();                            // 0x310ac
    void setHeaderIcon(SDL_Surface *, const char *, bool, bool); // 0x31ed4
    ListMenu *getMenu() { return menu; }                 // 0x3aad4
    void setParentPath(const std::string &s) { parentPath = s; }   // 0x3aafc, inline
    void setPath(const std::string &p) { path = p; }   // 0x3ab30, inline (search string for ROM lists)

    ListMenu *menu;            // +0x0c  deleted by onHide, not by the destructor
    int savedCurPos;     // +0x10  restored into the list on show
    int savedPageStart;  // +0x14
    int savedPageEnd;    // +0x18
    bool menuChanged;          // +0x1c
    bool hasPreview;          // +0x1d  set in onShow
    std::string shownImageKey;           // +0x20  selected thumbnail cache key
    SDL_Surface *previewImage;         // +0x38  thumbnail borrowed from thumbnailCache
    std::string pendingImageKey;           // +0x3c  thumbnail key awaiting a worker
    bool rightOpensDetail;         // +0x54  initialized to true
    SDL_Surface *cachedScreen;       // +0x58  freed in destructor
    void *emuEntry;        // +0x5c  emulator / RetroArch app entry for ROM lists (ids 5, 0x11)
    WorkerQueue thumbnailLoader;        // +0x60
    LruCache thumbnailCache;        // +0x80
    bool showFullPath;         // +0x94  toggle full ROM path in the header
    std::string titleText;           // +0x98  literal title for the path constructor
    SDL_Surface *iconTF;          // +0xb0  skin/icon-TF.png, freed in destructor
    std::string parentPath;           // +0xb4  used when building ROM lists
    std::string path;            // +0xcc  folder path passed to createMenu
    std::string themePath;       // +0xe4  copy of g_themePath
    SDL_Surface *headerIcon;       // +0xfc  freed only if ownsHeaderIcon
    std::string headerText;          // +0x100
    bool ownsHeaderIcon;   // +0x118
};
