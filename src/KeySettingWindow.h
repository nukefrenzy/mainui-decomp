// Key remapping screen: recovered 0x15c-byte layout.
#pragma once
#include <string>
#include "TrimusWindow.h"
#include "SDLTypes.h"

class KeySettingWindow : public TrimusWindow {
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

    SDL_Surface *menuIcon, *highlight, *background; // +0x0c
    int selected; // +0x18
    std::string keys[8]; // +0x1c
    SDL_Rect positions[8]; // +0xdc
    char keymap[64]; // +0x11c
};
