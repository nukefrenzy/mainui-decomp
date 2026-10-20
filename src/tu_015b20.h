// Shared declarations for tu_015b20 (0x15b20-0x49c3c): window and menu classes, actions, the window
// loop and createMenu. Each part file in src/tu_015b20/ adds only the declarations it needs here.
#pragma once
#include <stdio.h>
#include <unistd.h>
#include "TrimusWindow.h"
#include "MenuWindow.h"
#include <list>
#include "Action.h"
#include "ConfigTypes.h"
#include "MultiDBCachedTextMenu.h"   // EmuInfo
#include "ListMenu.h"
#include "FileBrowserCache.h"
#include "ImeWindow.h"

// p01
#include <string>
#include <list>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include "SDLTypes.h"
#include "ConfigTypes.h"
#include "MultiDBCachedTextMenu.h"   // EmuInfo
typedef void TTF_Font;
extern "C" {
int SDL_Flip(SDL_Surface *s);
int SDL_UpperBlit(SDL_Surface *src, SDL_Rect *srcRect, SDL_Surface *dst, SDL_Rect *dstRect);
SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg);
int TTF_FontHeight(TTF_Font *font);
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *rect, unsigned int color);
int GetKeyShm(void *shm, int key);
int SetKeyShm(void *shm, int key, int value);
int readAxpStatus(int *battery, int *voltage, int *charging);   // 0x14b198
int hasDsp1(int card);                                          // 0x14b53c
int wifiSignalLevel(int rssi);                                  // 0x14e118
int SDL_PushEvent(void *event);
int SDL_WaitEvent(void *event);
int putenv(char *s);
int setVolumeTinymix(int vol);                                  // 0x14ad50
const char *getKeymap();                                        // 0x14bab8
void setupKeymap(const char *keymap);                           // 0x14bb64
void encrypt_path(const char *path, int len);
void Mix_CloseAudio();
int Mix_OpenAudio(int freq, unsigned short format, int channels, int chunkSize);
int Mix_PlayMusic(void *music, int loops);
void Mix_PauseMusic();
int Mix_PlayChannelTimed(int channel, void *chunk, int loops, int ticks);
char *SDL_GetError();
}
#define Mix_PlayChannel(channel, chunk, loops) Mix_PlayChannelTimed(channel, chunk, loops, -1)
const char *languageString(int id);         // 0x135150
void saveSystemJson(void *shm, const char *themePath, int fontSize);   // 0x14c5fc
extern int g_fontSize;                      // 0x17e764
extern bool g_settingsDirty;                // 0x1808fc, save system.json before power-off
unsigned long milliseconds();               // 0x1238ac
int swapSize();                             // 0x11ffa8
int addRecent(std::list<RecentItem> *list, const char *label, const char *launch,
              const char *rompath, const char *imgpath, int type, bool save);   // 0x1204c4
extern std::list<RecentItem> g_recentGames; // 0x18192c
extern std::vector<TrimusWindow *> *g_windowStack;   // 0x17ff38
extern char g_launchCmd[0x800];             // 0x17ff54, command written to /tmp/cmd_to_run.sh
extern char g_cwdBuf[0x1000];               // 0x180908
extern int g_fbFd;                          // 0x17e488
extern fb_var_screeninfo g_fbVar;           // 0x18085c
extern int g_powerDownCount;                // 0x180900, power key repeats while held
extern bool g_powerSent;                    // 0x180904, power-off dialog already shown
extern bool g_startHeld;                    // 0x180905
extern bool g_selectHeld;                   // 0x180906
extern bool g_statusIndicatorFlag;          // 0x180907, user events 40/41
extern void *g_themeMusic;                  // 0x182268
extern void *g_changeSound;                 // 0x18226c
// SDL 1.2 events as handleGlobalInput reads them.
struct SDL_keysym { unsigned char scancode; int sym; int mod; unsigned short unicode; };
struct SDL_KeyboardEvent { unsigned char type, which, state; SDL_keysym keysym; };
struct SDL_MouseButtonEvent { unsigned char type, which, button, state; unsigned short x, y; };
union SDL_Event {
    unsigned char type;
    SDL_KeyboardEvent key;
    SDL_MouseButtonEvent button;
    struct {
        unsigned char type;
        int code;
        void *data1, *data2;
    } user;
};
union MainEvent {                           // SDL_Event as pushed by MainUI (SDL_USEREVENT = 24)
    unsigned char type;
    struct {
        unsigned char type;
        int code;
        void *data1, *data2;
    } user;
    char storage[20];
};
struct WifiStatus {                         // 0x74 bytes, see tu_14e0cc.c
    int state;                              // 4 = COMPLETED
    char raw4[4];
    char ssid[0x40];
    char ip[0x18];
    int rssi;                               // +0x60
    int linkSpeed, noise, freq, id;
};
extern "C" WifiStatus *getWifiStatus();     // 0x14e564
int wifiMode();                             // 0x138c50: 2 station, 3 hotspot
#define SDL_BlitSurface SDL_UpperBlit
extern SDL_Surface *g_screen;               // 0x181924
extern TTF_Font *g_imeFont;                 // 0x18217c
extern TTF_Font *g_labelFont;               // 0x182188
extern TTF_Font *g_titleFont;               // 0x18218c
extern char g_keyShm[];                    // 0x181968
extern SDL_Surface *g_background;           // 0x182190
extern SDL_Surface *g_unusedIndicator;      // 0x182194, bottom bar
extern SDL_Surface *g_unusedStatus;         // 0x182198, status-icon frame (48x48 when absent)
extern SDL_Surface *g_navItem;              // 0x18219c, footer background
extern SDL_Surface *g_titleBar;             // 0x1821a8
extern SDL_Surface *g_miniLogo;             // 0x1821b0
extern SDL_Surface *g_listStripe;           // 0x1821c8
extern SDL_Surface *g_iconA, *g_iconB;      // 0x1821d4, 0x1821d8: A/B button hints
extern SDL_Surface *g_chargeIcon;           // 0x1821dc
extern SDL_Surface *g_battery0, *g_battery1, *g_battery2, *g_battery3, *g_battery4;   // 0x1821e0..
extern SDL_Surface *g_hotspotIcon;          // 0x182200
extern SDL_Surface *g_wifi4, *g_wifi3, *g_wifi2;                                     // 0x182204..
extern SDL_Surface *g_headphoneIcon;        // 0x182210
extern SDL_Surface *g_keyStateIcons[21];    // 0x182214, indexed by key shm slot 0
extern void *g_enterSound;                  // 0x182270
extern SDL_Color g_wifiEmptyColor;          // 0x182274, title text
extern SDL_Color g_shadowColor;             // 0x182278
extern SDL_Color g_labelColor;              // 0x18227c
extern SDL_Color g_selectedColor;           // 0x18229c
const char *keyEventName(int key);          // 0x15b20, unreferenced
void onUserActivity();                      // 0x1772c, empty
int powerOff(void *arg);                    // 0x1761c
void powerOffNow();                         // 0x176a8
void pushWindow(TrimusWindow *w);           // 0x36d88 (p11)
#include "Action.h"
class GeneralAction : public Action {
public:
    GeneralAction(int result, int (*fn)(void *), void *arg) : Action(result) { this->fn = fn; this->arg = arg; verbose = false; } // 0x39bac
    virtual ~GeneralAction() {}
    virtual int execute(void *param);                        // 0x19ecc
    int (*fn)(void *);  // +0x08
    void *arg;          // +0x0c
    bool verbose;       // +0x10
};
int enableSwapAction(void *arg);            // 0x38834 (p11)
#include "FileBrowserCache.h"               // TextItem
// A ROM (or ROM folder) in a list; sizeof == 0x74. Ctor 0x39948, dtors 0x39ac4/0x39b34 and
// finished 0x39b6c are inline in the tail.
class EmuAction : public Action {
public:
    EmuAction(EmuInfo *emu, const char *label, const char *rompath, const char *imgpath, int result,
              bool detail, int isDir)
        : Action(result), emu(emu), imgpath(imgpath), rompath(rompath), label(label), detail(detail),
          isDir(isDir) {}
    virtual ~EmuAction() {}
    virtual int execute(void *param);           // 0x1897c, param: the TextItem
    virtual int progress(int *item, int *arg);  // 0x18b00, opens the game detail window
    virtual int finished() { return isDir == 0; }
    EmuInfo *emu;           // +0x08
    std::string imgpath;    // +0x0c
    std::string rompath;    // +0x24
    std::string label;      // +0x3c
    bool detail;            // +0x54  has a detail window
    int isDir;              // +0x58  opens a ROM folder instead of launching
    std::string folderArg;  // +0x5c  handed to the folder window (MenuWindow::strB4)
};
// p10 windows, only as far as p01 constructs them (sizes from operator new).
class GameDetailWindow : public TrimusWindow {
public:
    GameDetailWindow(int title, int id, const char *name, const char *item, const char *launch,
                     const char *icon, int *param, int type, bool favorite); // 0x34b1c
    virtual int handleInput(); virtual int draw(SDL_Surface *); virtual void onHide();
    virtual void onShow(); virtual void saveState(cJSON *); virtual int restoreState(cJSON *);
    virtual void update();
    char fields[0x9c - 0xc];
};
class ProgressDialog : public TrimusWindow {
public:
    ProgressDialog(int title, Action *action);             // 0x34184
    virtual int handleInput(); virtual int draw(SDL_Surface *); virtual void onHide();
    virtual void onShow(); virtual void saveState(cJSON *); virtual int restoreState(cJSON *);
    virtual void update();
    char fields[0x3c - 0xc];
};
class DialogWindow : public TrimusWindow {
public:
    DialogWindow(int title, int text, bool flag, Action *action);   // 0x333ec
    virtual int handleInput(); virtual int draw(SDL_Surface *); virtual void onHide();
    virtual void onShow(); virtual void saveState(cJSON *); virtual int restoreState(cJSON *);
    virtual void update();
    char fields[0x34 - 0xc];
};
class CountDownWindow : public TrimusWindow {
public:
    CountDownWindow(int title, void (*onTimeout)(), void (*onCancel)());   // 0x33060
    virtual int handleInput(); virtual int draw(SDL_Surface *); virtual void onHide();
    virtual void onShow(); virtual void saveState(cJSON *); virtual int restoreState(cJSON *);
    virtual void update();
    char fields[0xcc - 0xc];
};
void writeCmdToRun(const char *cmd);        // 0x17fc0
void drawBackground(SDL_Surface *dst);      // 0x17450

// p02: actions. Inline ctors/dtors sit in the tail at 0x3984c-0x3a678, in this declaration order
// (EmuAction and GeneralAction are declared with p01 above, DeleteRomAction belongs to p03; FileAction is in FileBrowserCache.h).
// Every class here inherits Action's slots 3/4 (0x397fc returns -1, 0x39828 returns 0) except AppAction.
struct cJSON;
extern "C" {
    cJSON *cJSON_CreateObject();
    cJSON *cJSON_AddStringToObject(cJSON *, const char *, const char *);
    cJSON *cJSON_AddNumberToObject(cJSON *, const char *, double);
    char *cJSON_PrintUnformatted(const cJSON *);
    void cJSON_Delete(cJSON *);
}
int clearRecent(std::list<RecentItem> *list);                // 0x120e20 (tu_11cdc8)
bool isFavourite(const char *label, FILE *f, const char *rompath); // 0x1b210 (p03)

// p02 render helpers (0x18dd4-0x19acc): busy animation while a background job runs. g_launchCmd
// doubles as the counter text buffer, g_titleFont/g_wifiEmptyColor/g_background are p01's globals.
extern "C" void SDL_Delay(unsigned int ms);
extern volatile int g_busy;                                         // 0x181908  animation runs while set
extern volatile int g_progressCur;                                  // 0x18190c
extern volatile int g_progressTotal;                                // 0x181910
extern SDL_Color g_progressColor;                            // 0x1822a0
int digitCount(int n);                                       // 0x18dd4
extern "C" void SDL_FreeSurface(SDL_Surface *s);
SDL_Surface *createSurface(int w, int h);                    // 0x1d720 (p03)
// Argument of the busy-screen thread (SDL_CreateThread from IconMenu::onSelect, createMenu, ...).
struct BusyData {
    const char *title;      // "sdata->title"
    int type;
};
int busyScreenThread(void *arg);                             // 0x193a8
int loadingScreenThread(void *arg);                          // 0x198a8

class AppAction : public Action {
public:
    AppAction(AppInfo *app, bool detail, int result) : Action(result) { this->app = app; this->detail = detail; } // 0x3984c
    virtual ~AppAction() {}
    virtual int execute(void *param);                        // 0x18c54
    virtual int progress(int *item, int *arg);               // 0x18c94, opens the detail window
    virtual int finished() { return 1; }                     // 0x39924
    AppInfo *app;       // +0x08
    bool detail;        // +0x0c
};
// Unnamed base (ctor 0x39c8c, vtable 0x1546e0) of the actions that reload a list afterwards.
class MenuAction : public Action {
public:
    MenuAction(ListMenu *menu, int result) : Action(result) { this->menu = menu; field0c = 0; }
    virtual ~MenuAction() {}
    ListMenu *menu;     // +0x08
    int field0c;        // +0x0c
};
class ClearFavoriteAction : public MenuAction {
public:
    ClearFavoriteAction(ListMenu *menu, int result) : MenuAction(menu, result) {} // 0x39e18
    virtual ~ClearFavoriteAction() {}
    virtual int execute(void *param);                        // 0x19f64
};
class ClearRecentAction : public MenuAction {
public:
    ClearRecentAction(ListMenu *menu, int result) : MenuAction(menu, result) {} // 0x39ed4
    virtual ~ClearRecentAction() {}
    virtual int execute(void *param);                        // 0x19fd8
};
// Unnamed base (ctor 0x39f90, vtable 0x1546c4) of the actions on one list item.
class ItemAction : public Action {
public:
    ItemAction(TextItem *item, int result) : Action(result) { this->item = item; }
    virtual ~ItemAction() {}
    TextItem *item;     // +0x08
};
class AddFavoriteAction : public ItemAction {
public:
    AddFavoriteAction(TextItem *item, int result) : ItemAction(item, result) {} // 0x3a054
    virtual ~AddFavoriteAction() {}
    virtual int execute(void *param);                        // 0x1a02c
};
class RemoveFavoriteAction : public ItemAction {
public:
    RemoveFavoriteAction(TextItem *item, int result) : ItemAction(item, result) {} // 0x3a110
    virtual ~RemoveFavoriteAction() {}
    virtual int execute(void *param);                        // 0x19acc
};
class SearchAction : public Action {
public:
    SearchAction(int arg, int id, void *emu) : Action(-1) { this->arg = arg; this->id = id; this->emu = emu; } // 0x3a24c
    virtual ~SearchAction() {}
    virtual int execute(void *param);                        // 0x19b04
    int arg;            // +0x08
    int id;             // +0x0c  menu id of the result window
    void *emu;          // +0x10
};
class GlobalSearchAction : public Action {
public:
    GlobalSearchAction() : Action(-1) {}                     // 0x3a32c
    virtual ~GlobalSearchAction() {}
    virtual int execute(void *param);                        // 0x19c18
};
class PopupSearchIME : public Action {
public:
    PopupSearchIME(int arg, int id, void *emu) : Action(-1) { this->arg = arg; this->id = id; this->emu = emu; } // 0x3a3dc
    virtual ~PopupSearchIME() {}
    virtual int execute(void *param);                        // 0x19d24
    int arg;            // +0x08
    int id;             // +0x0c
    void *emu;          // +0x10
};
class PopupDialogAction : public Action {
public:
    PopupDialogAction(int title, int text, bool flag, Action *action) : Action(-1) { this->title = title; this->text = text; this->flag = flag; this->action = action; } // 0x3a4bc
    virtual ~PopupDialogAction() {}
    virtual int execute(void *param);                        // 0x19dc0
    int title;          // +0x08
    int text;           // +0x0c
    bool flag;          // +0x10
    Action *action;     // +0x14  run by the dialog
};
class PopupProgressAction : public Action {
public:
    PopupProgressAction(int title, Action *action) : Action(-1) { this->title = title; this->action = action; } // 0x3a5a8
    virtual ~PopupProgressAction() {}
    virtual int execute(void *param);                        // 0x19e44
    int title;          // +0x08
    Action *action;     // +0x0c
};

// p09
extern "C" void SDL_FreeSurface(SDL_Surface *s);
extern char *g_themePath;                   // 0x181970
extern SDL_Surface *g_backBuffer;           // 0x181928
extern const char g_skinSubdir[];           // 0x151a48 (second argument of every skin load)
SDL_Surface *loadSkinImage(const char *theme, const char *sub, const char *file,
                           SDL_Surface *fmt, bool flags);   // 0x123af0
void freeSurfaceCallback(SDL_Surface *s);  // 0x2e318
