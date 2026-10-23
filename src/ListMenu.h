// ListMenu (0x48 bytes): base of the lists inside a MenuWindow.
// Slot names come from what each slot does across the subclasses (onUnhandledEvent and
// freeListItem are original names, from log messages); they were numbered vfuncN before.
// Vtable 0x153fdc; inline definitions sit in MainUI.cpp's tail (0x39520-0x39750).
#pragma once
#include <string>
#include <vector>
#include "SDLTypes.h"

class ListMenu
{
public:
    ListMenu(bool flag);                                      // 0x1d8fc
    virtual ~ListMenu();                                     // slots 0-1, inline (0x39520, D0 0x39570)
    virtual void restorePosition(int cur, int pageStart, int pageEnd); // slot 2, 0x1d9d8
    virtual int handleKey();                                     // slot 3, 0x1de34: one button (see MainUI.cpp)
    virtual int onSelect() = 0;                                 // slot 4: A on the current row
    virtual void *getItem(int index) = 0;                      // slot 5
    virtual void *getItemData(int index);                         // slot 6, 0x39678: a row's extra data
    virtual void drawList(SDL_Surface *dst) = 0;                 // slot 7: draw the visible rows
    virtual int count() = 0;                                  // slot 8
    virtual int moveDown();                                     // slot 9, 0x1dae8 (next, wraps)
    virtual int moveUp();                                    // slot 10, 0x1db6c (previous, wraps)
    virtual void reloadTheme();                                  // slot 11, 0x396a0: reload images after a theme change
    virtual void removeItem(int arg);                           // slot 12, 0x396c0: delete a row (and its ROM)
    virtual void clearItems();                                  // slot 13, 0x396e4: empty the list
    virtual void setSearch(const char *arg);                   // slot 14, 0x39704: filter by a search string
    virtual int onUnhandledEvent(int event);                          // slot 15, 0x39728: a user event (Input.h)
    virtual int pageUp();                                    // slot 16, 0x1dbdc (6 up)
    virtual int pageDown();                                    // slot 17, 0x1dc30 (6 down)
    void recordKey(int key);                                   // 0x1dcc8
    bool keyRepeating();                                       // 0x1ddf4
    int getCur();                                            // 0x395a4, inline
    int getPageStart();                                      // 0x395cc, inline
    int getPageEnd();                                        // 0x395f4, inline
    std::string getSearch();                                 // 0x3961c, inline
    SDL_Rect *getRect();                                     // 0x39650, inline
    void setRect(SDL_Rect *r);                                 // 0x1d99c

    int previewWidth;        // +0x04  0 in the ctor
    bool gridMode;         // +0x08  ctor argument (IconMenu true, FileBrowserMenu/TextMenuDrawer false)
    int cur;            // +0x0c  current item
    int pageStart;      // +0x10
    int pageEnd;        // +0x14
    std::string searchText;          // +0x18  search string for the DB menus
    SDL_Rect rect;           // +0x30
    int lastKeyTime;        // +0x38  milliseconds() of the last recorded key
    std::vector<int> history;   // +0x3c  keys repeated within 200 ms; base ends at +0x48
};

// Inline definition order follows the original main UI header.
inline ListMenu::~ListMenu() {}
inline int ListMenu::getCur()
{ return cur; }
inline int ListMenu::getPageStart()
{ return pageStart; }
inline int ListMenu::getPageEnd()
{ return pageEnd; }
inline std::string ListMenu::getSearch()
{ return searchText; }
inline SDL_Rect *ListMenu::getRect()
{ return &rect; }
inline void *ListMenu::getItemData(int index)
{ return 0; }
inline void ListMenu::reloadTheme() {}
inline void ListMenu::removeItem(int arg) {}
inline void ListMenu::clearItems() {}
inline void ListMenu::setSearch(const char *arg) {}
inline int ListMenu::onUnhandledEvent(int event)
{ return -1; }
