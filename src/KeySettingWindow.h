// KeySettingWindow: the button-mapping screen (tu_1376fc.cpp), 0x15c bytes.
#pragma once
#include <string>
#include "CJsonApi.h"
#include "TrimusWindow.h"
#include "SDLTypes.h"

class KeySettingWindow : public TrimusWindow
{
public:
    KeySettingWindow();
    virtual ~KeySettingWindow();
    virtual int handleInput();
    virtual int draw(SDL_Surface *);
    virtual void onHide() {}
    virtual void onShow() {}
    virtual void saveState(cJSON *) {}
    virtual int restoreState(cJSON *) { return 0; }
    virtual void update() {}
    void loadResources();

    SDL_Surface *menuIcon, *highlight, *background; // +0x0c: footer MENU icon, slot highlight, screen background
    int selected; // +0x18: slot the next press assigns (0-7)
    std::string keys[8]; // +0x1c: button name per slot, in screen order L, L2, R, R2, X, A, B, Y
    SDL_Rect positions[8]; // +0xdc: screen rectangle of each slot
    char keymap[64]; // +0x11c: keymap string built on save
};
