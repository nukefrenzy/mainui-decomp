// Hardware keyboard test, shared by settings and the hardware diagnostics unit.
#pragma once
#include "TrimusWindow.h"
#include "SDLTypes.h"
enum SDLKey { SDLKeyUnknown=0, SDLKeyLast=323 };
class KeyTestWindow2 : public TrimusWindow {
public:
    KeyTestWindow2();
    ~KeyTestWindow2() override;
    int handleInput() override;
    int draw(SDL_Surface*) override;
    void onHide() override;
    void onShow() override;
    void saveState(cJSON*) override;
    int restoreState(cJSON*) override;
    void update() override;
    void loadMasks();
    int mapKey(SDLKey);
    SDL_Surface *maskCross,*maskStart,*maskMenu,*maskLR,*maskABXY,*maskAdd,*maskMinus,*background;
    int pressed[18],released[18];
    bool menuHeld;
};

inline void KeyTestWindow2::onHide() {  }
inline void KeyTestWindow2::onShow() {  }
inline void KeyTestWindow2::saveState(cJSON*) {  }
inline int KeyTestWindow2::restoreState(cJSON*) { return 0; }
inline void KeyTestWindow2::update() {  }
