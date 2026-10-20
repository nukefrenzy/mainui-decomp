// Hardware-test window layouts, proven with tu_04cb68.cpp.
#pragma once
#include "TrimusWindow.h"
#include "SDLTypes.h"
class SDLKeyTestWindow : public TrimusWindow {
public:
    SDLKeyTestWindow(const int*,int,int(*)(),const char*,int);
    ~SDLKeyTestWindow() override;
    int handleInput() override;
    int draw(SDL_Surface*) override;
    void onHide() override;
    void onShow() override;
    void saveState(cJSON*) override;
    int restoreState(cJSON*) override;
    void update() override;
    virtual const char*keyName(int);
    void restartTimer();
    int current;
    char message[64];
    int seconds;
    void*timer;
    int(*readInput)();
    const int*keys;
    int keyCount,testIndex;
};
struct TestKeyImage {
    TestKeyImage(const char*,const char*);
    ~TestKeyImage();
    SDL_Surface *normal,*focused;
};
class KeyTestWindow : public SDLKeyTestWindow {
public:
    KeyTestWindow(const int*,int,int(*)(),const char*,int);
    int draw(SDL_Surface*) override;
    const char*keyName(int) override;
    bool expecting(int);
    TestKeyImage left,right,up,down,circle,rectangle;
};
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
union TestColor { unsigned value; SDL_Color bytes; };
class LCDTestWindow : public TrimusWindow {
public:
    LCDTestWindow();
    ~LCDTestWindow() override;
    int handleInput() override;
    int draw(SDL_Surface*) override;
    void onHide() override;
    void onShow() override;
    void saveState(cJSON*) override;
    int restoreState(cJSON*) override;
    void update() override;
    int mode,colorIndex;
    TestColor colors[5];
};
class SoundTestWindow : public TrimusWindow {
public:
    SoundTestWindow();
    ~SoundTestWindow() override;
    int handleInput() override;
    int draw(SDL_Surface*) override;
    void onHide() override;
    void onShow() override;
    void saveState(cJSON*) override;
    int restoreState(cJSON*) override;
    void update() override;
    void playNext();
    int phase;void*timer;
    char reserved14[0x94-0x14];
};
struct SDL_Thread;
class SDCardTestWindow : public TrimusWindow {
public:
    SDCardTestWindow();
    ~SDCardTestWindow() override;
    int handleInput() override;
    int draw(SDL_Surface*) override;
    void onHide() override;
    void onShow() override;
    void saveState(cJSON*) override;
    int restoreState(cJSON*) override;
    void update() override;
    void testWrite();void testRead();
    SDL_Thread*thread;
    int writeBytes,writeMS,readBytes,readMS;
    bool readDone,writeDone;
    char message[64];
    int lastUpdate;
};
#include <string>
#include <vector>
#include "ListMenu.h"
struct TestMenuItem {
    TestMenuItem(const char*,const char*);
    ~TestMenuItem();
    SDL_Surface*icon;
    std::string label;
};
class TextMenu : public TextMenuDrawer {
public:
    TextMenu(int,int,int,const char*);
    ~TextMenu() override;
    int vfunc3() override;
    void*getItem(int) override;
    void vfunc7(SDL_Surface*) override;
    void vfunc11() override;
    void vfunc12() override;
    void vfunc13() override;
    virtual void vfunc18();
    void addItem(TestMenuItem*);
    std::vector<TestMenuItem*>items;
};
class HardwareTestMenu : public TextMenu {
public:
    HardwareTestMenu();
    ~HardwareTestMenu() override;
    int vfunc4() override;
    void vfunc7(SDL_Surface*) override;
    void vfunc18() override;
    void initializeTests();
    int stressSeconds;
};

// Inline definitions follow the original object emission order.
inline void KeyTestWindow2::onHide() {  }
inline void KeyTestWindow2::onShow() {  }
inline void KeyTestWindow2::saveState(cJSON*) {  }
inline int KeyTestWindow2::restoreState(cJSON*) { return 0; }
inline void KeyTestWindow2::update() {  }
inline void SDLKeyTestWindow::saveState(cJSON*) {  }
inline int SDLKeyTestWindow::restoreState(cJSON*) { return 0; }
inline void SDLKeyTestWindow::update() {  }
inline void LCDTestWindow::saveState(cJSON*) {  }
inline int LCDTestWindow::restoreState(cJSON*) { return 0; }
inline void LCDTestWindow::update() {  }
inline void SoundTestWindow::saveState(cJSON*) {  }
inline int SoundTestWindow::restoreState(cJSON*) { return 0; }
inline void SoundTestWindow::update() {  }
inline void SDCardTestWindow::saveState(cJSON*) {  }
inline int SDCardTestWindow::restoreState(cJSON*) { return 0; }
inline void SDCardTestWindow::update() {  }
inline SDLKeyTestWindow::~SDLKeyTestWindow() {}
inline SDCardTestWindow::~SDCardTestWindow() {}
inline SoundTestWindow::~SoundTestWindow() {}
inline LCDTestWindow::~LCDTestWindow() {}
