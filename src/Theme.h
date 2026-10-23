// Theme configuration layout consumed by the font/image/sound loader.
#pragma once
#include <string>
#include "SDLTypes.h"
union ThemeColor {
    unsigned int value;
    SDL_Color bytes;
};
struct Theme {
    std::string name, root;
    std::string titleFontMulti, titleFontEnglish;
    int titleSize;
    ThemeColor titleColor;
    std::string hintFont;
    int hintSize;
    ThemeColor hintColor;
    std::string labelFontMulti, labelFontEnglish;
    int labelSize;
    ThemeColor labelColor;
    std::string listFont;
    int listSize;
    ThemeColor listColor;
    std::string emptyFont;
    int emptySize;
    ThemeColor emptyColor;
    std::string iconFontMulti, iconFontEnglish;
    int iconSize, smallSize;
    ThemeColor textColor, selectedColor;
    int gridIconOffsetY, gridIconOffsetX, gridTextOffsetY;
    std::string mainFontMulti, mainFontEnglish;
    int mainSize;
    ThemeColor wifiColor, imeColor;
    ThemeColor accentColor;
};
void setDefaultTheme(Theme *);
bool loadTheme(Theme *, int *);
int freeTheme();
int changeLanguageFonts(Theme *, int *);
