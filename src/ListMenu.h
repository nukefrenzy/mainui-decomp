// ListMenu (0x48 bytes) / TextMenuDrawer (0xf8): lists inside a MenuWindow.
// Slot names remain signposts where the base bodies have not been rewritten.
// Vtables (0x153fdc ListMenu, 0x153e04 TextMenuDrawer) show which slots each class defines and which
// are pure; the inline bodies sit in tu_015b20's tail (0x39520-0x39750, 0x3a678-0x3a6dc).
#pragma once
#include <string>
#include <vector>
#include "SDLTypes.h"

class ListMenu {
public:
    ListMenu(bool flag);                                      // 0x1d8fc
    virtual ~ListMenu() {}                                     // slots 0-1, inline (0x39520, D0 0x39570)
    virtual void restorePosition(int a, int cur, int c);       // slot 2, 0x1d9d8
    virtual int  vfunc3();                                     // slot 3, 0x1de34 (key input)
    virtual int  vfunc4() = 0;                                 // slot 4: onSelect
    virtual void *getItem(int index) = 0;                      // slot 5
    virtual void *vfunc6(int index) { return 0; }              // slot 6, 0x39678
    virtual void vfunc7(SDL_Surface *dst) = 0;                 // slot 7: draw
    virtual int  count() = 0;                                  // slot 8
    virtual int  vfunc9();                                     // slot 9, 0x1dae8
    virtual int  vfunc10();                                    // slot 10, 0x1db6c
    virtual void vfunc11() {}                                  // slot 11, 0x396a0
    virtual void vfunc12(int) {}                               // slot 12, 0x396c0
    virtual void vfunc13() {}                                  // slot 13, 0x396e4 (reload)
    virtual void vfunc14(int) {}                               // slot 14, 0x39704
    virtual int  vfunc15(int event) { return -1; }             // slot 15, 0x39728
    virtual int  vfunc16();                                    // slot 16, 0x1dbdc
    virtual int  vfunc17();                                    // slot 17, 0x1dc30
    int getCur() { return cur; }                               // 0x395a4, inline
    int getPageStart() { return pageStart; }                   // 0x395cc, inline
    int getPageEnd() { return pageEnd; }                       // 0x395f4, inline
    std::string getSearch() { return str18; }                  // 0x3961c, inline
    SDL_Rect *getRect() { return &rect; }                      // 0x39650, inline
    void setRect(SDL_Rect *r);                                 // 0x1d99c

    int         field04;        // +0x04  0 in the ctor
    bool        flag08;         // +0x08  ctor argument (IconMenu true, FileBrowserMenu/TextMenuDrawer false)
    int         cur;            // +0x0c  current item
    int         pageStart;      // +0x10
    int         pageEnd;        // +0x14
    std::string str18;          // +0x18  search string for the DB menus
    SDL_Rect    rect;           // +0x30
    int         field38;
    std::vector<int> history;   // +0x3c; base ends at +0x48
};

class TextMenuDrawer : public ListMenu {
public:
    TextMenuDrawer(bool a, bool b, int c, const char *background); // 0x1e128; a, b unverified (bytes)
    virtual ~TextMenuDrawer();                                     // 0x1e3f8 (D0 0x1e4e4)
    virtual int  vfunc3();                                         // 0x204d4
    virtual int  vfunc4();                                         // 0x1e518 onSelect: runs the item's Action
    virtual void vfunc7(SDL_Surface *dst);                         // 0x1f6f0 draw
    virtual void vfunc11();                                        // 0x1e5d0
    void setRowHeight(int h) { field58 = h; }                      // 0x3a678, inline
    void setFlag5c(bool f) { flag5c = f; }                         // 0x3a6a8, inline
    int         field48;        // +0x48  0
    SDL_Surface *selectedBackground; // +0x4c
    int         field50;        // +0x50  unverified
    int         field54;        // +0x54  0
    int         field58;        // +0x58  row height (0x3c)
    bool        flag5c;         // +0x5c  1
    char        raw5d[0xa0 - 0x5d];
    std::string strA0;          // +0xa0
    SDL_Color   textColor;      // +0xb8  g_imeTextColor
    SDL_Color   colorBC;        // +0xbc  g_imeTextColor
    bool        flagC0;         // +0xc0  1
    bool        flagC1;         // +0xc1  ctor b
    int         fieldC4;        // +0xc4  ctor c
    std::string themePath;      // +0xc8  copy of g_themePath
    std::string background;     // +0xe0  "skin/bg-list-s.png" unless given
};
