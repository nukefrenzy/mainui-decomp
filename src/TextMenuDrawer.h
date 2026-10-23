// Drawing base for text rows; included after Action helpers to preserve the main UI tail order.
#pragma once
#include "ListMenu.h"

class TextMenuDrawer : public ListMenu
{
public:
    TextMenuDrawer(bool noSelectedBg, bool stripes, int rows, const char *background); // 0x1e128
    virtual ~TextMenuDrawer();                                     // 0x1e3f8 (D0 0x1e4e4)
    virtual int handleKey();                                         // 0x204d4
    virtual int onSelect();                                         // 0x1e518 onSelect: runs the item's Action
    // Still pure, but redeclared here: calls from TextMenuDrawer's own methods then evaluate `this`
    // after the arguments, as in 0x1e518 and 0x1f6f0.
    virtual void *getItem(int index) = 0;
    virtual void drawList(SDL_Surface *dst);                         // 0x1f6f0 draw
    virtual int count() = 0;
    virtual void reloadTheme();                                        // 0x1e5d0
    void setRowHeight(int h) { textRowHeight = h; }                      // 0x3a678, inline
    void setUnusedPopupFlag(bool f) { unusedPopupFlag = f; }                         // 0x3a6a8, inline
    SDL_Surface *makeSelectedBackground(SDL_Surface *src);         // 0x1e76c
    void drawHighlighted(SDL_Surface *dst, void *font, SDL_Rect rect, const char *text,
                         SDL_Color color, int align, bool shadow, const char *key,
                         const char *initials);                    // 0x1ede4
    void setBackground(const char *file);                          // 0x1f5e0  no caller found
    void freeSelectedBackground(bool really);                      // 0x1f694  no caller found
    SDL_Surface *rowBackground;     // +0x48  drawn behind every row if set; nothing ever loads it (always NULL)
    SDL_Surface *selectedBackground; // +0x4c
    SDL_Surface *favoriteMark;  // +0x50  skin/ic-favorite-mark.png
    SDL_Surface *customBackground;     // +0x54  0; freed by the dtor
    int textRowHeight;        // +0x58  row height (0x3c)
    bool unusedPopupFlag; // +0x5c  true; pop-up option lists clear it (setUnusedPopupFlag), nothing reads it
    char shortLabel[0xa0 - 0x5d]; // +0x5d  first 64 bytes of a label longer than 48
    std::string textScratch;          // +0xa0  scratch for drawHighlighted's pieces
    SDL_Color textColor;      // +0xb8  g_imeTextColor
    SDL_Color colorBC;        // +0xbc  g_imeTextColor
    bool shadowSelected;         // +0xc0  1
    bool drawStripes;         // +0xc1  ctor stripes: draw g_listStripe between rows
    int rowsPerPage;        // +0xc4  ctor rows: rows per page
    std::string themePath;      // +0xc8  copy of g_themePath
    std::string background;     // +0xe0  "skin/bg-list-s.png" unless given
};
