// Shared declarations for tu_015b20 (0x15b20-0x49c3c): window and menu classes, actions, the window
// loop and createMenu. Each part file in src/tu_015b20/ adds only the declarations it needs here.
#pragma once
#include "PlatformApi.h"
#include "SdlApi.h"
#include "CJsonApi.h"
#include <atomic>
#include <stdio.h>
#include <unistd.h>
#include "BGWorker.h"
#include "TrimusWindow.h"
#include <list>
#include "ConfigTypes.h"
struct EmuInfo;
class TextItem;
#include "ListMenu.h"

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
#include "Input.h"
#include "ShmVar.h"
#include "WindowIds.h"
#include "ConfigTypes.h"
typedef void TTF_Font;
extern "C" {


SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg);


int GetKeyShm(void *shm, int key);
int SetKeyShm(void *shm, int key, int value);
int readAxpStatus(int *battery, int *voltage, int *charging);   // 0x14b198
int hasDsp1(int card);                                          // 0x14b53c
int wifiSignalLevel(int rssi);                                  // 0x14e118


int putenv(char *s);
int setVolumeTinymix(int vol);                                  // 0x14ad50
const char *getKeymap();                                        // 0x14bab8
void setupKeymap(const char *keymap);                           // 0x14bb64
void encrypt_path(const char *path, int len);
}
#define Mix_PlayChannel(channel, chunk, loops) Mix_PlayChannelTimed(channel, chunk, loops, -1)
const char *languageString(int id);         // 0x135150
void saveSystemJson(void *shm, const char *themePath, int fontSize);   // 0x14c5fc
extern int g_fontSize;                      // 0x17e764
unsigned long milliseconds();               // 0x1238ac
int swapSize();                             // 0x11ffa8
int addRecent(std::list<RecentItem> *list, const char *label, const char *launch,
              const char *rompath, const char *imgpath, int type, bool save);   // 0x1204c4
extern std::list<RecentItem> g_recentGames; // 0x18192c
extern std::vector<TrimusWindow *> *g_windowStack;   // 0x17ff38
extern char g_launchCmd[0x800];             // 0x17ff54, command written to /tmp/cmd_to_run.sh
extern int g_fbFd;                          // 0x17e488
extern void *g_themeMusic;                  // 0x182268
extern void *g_changeSound;                 // 0x18226c
// SDL 1.2 events as handleGlobalInput reads them.
struct SDL_keysym {
    unsigned char scancode;
    int sym;
    int mod;
    unsigned short unicode;
};
struct SDL_KeyboardEvent {
    unsigned char type, which, state;
    SDL_keysym keysym;
};
struct SDL_MouseButtonEvent {
    unsigned char type, which, button, state;
    unsigned short x, y;
};
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
#include "WifiStatus.h"
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
extern SDL_Surface *g_navItem;              // 0x18219c
extern SDL_Surface *g_keySettingFooter;     // 0x1821ac, footer background drawn by drawFooter
extern SDL_Surface *g_titleBar;             // 0x1821a8
extern SDL_Surface *g_miniLogo;             // 0x1821b0
extern SDL_Surface *g_listStripe;           // 0x1821c8
extern SDL_Surface *g_iconA, *g_iconB;      // 0x1821d4, 0x1821d8: A/B button hints
extern SDL_Surface *g_chargeIcon;           // 0x1821dc
extern SDL_Surface *g_battery0, *g_battery1, *g_battery2, *g_battery3, *g_battery4;   // 0x1821e0..
extern SDL_Surface *g_hotspotIcon;          // 0x182200
extern SDL_Surface *g_wifi4, *g_wifi3, *g_wifi2;                                     // 0x182204..
extern SDL_Surface *g_headphoneIcon;        // 0x182210
extern SDL_Surface *g_volumeIcons[21];    // 0x182214, indexed by key shm slot 0
extern void *g_enterSound;                  // 0x182270
extern SDL_Color g_wifiEmptyColor;          // 0x182274, title text
extern SDL_Color g_clearColor;             // 0x182278
extern SDL_Color g_labelColor;              // 0x18227c
extern SDL_Color g_selectedColor;           // 0x18229c
const char *keyEventName(int key);          // 0x15b20, unreferenced
void onUserActivity();                      // 0x1772c, empty
int powerOff(void *arg);                    // 0x1761c
void powerOffNow();                         // 0x176a8
void pushWindow(TrimusWindow *w);           // 0x36d88 (p11)
#include "Action.h"
class AppAction : public Action
{
public:
    AppAction(AppInfo *app, bool detail, int result) : Action(result)
    {
        this->app = app;
        this->detail = detail;
    } // 0x3984c
    virtual ~AppAction() {}
    virtual int execute(void *param);                        // 0x18c54
    virtual int progress(int *item, int index);               // 0x18c94, opens the detail window
    virtual int finished() { return 1; }                     // 0x39924
    AppInfo *app;       // +0x08
    bool detail;        // +0x0c
};

class EmuAction : public Action
{
public:
    EmuAction(EmuInfo *emu, const char *label, const char *rompath, const char *imgpath, int result,
              bool detail, int isDir)
        : Action(result), emu(emu), imgpath(imgpath), rompath(rompath), label(label), detail(detail),
          isDir(isDir) {}
    virtual ~EmuAction() {}
    virtual int execute(void *param);           // 0x1897c, param: the TextItem
    virtual int progress(int *item, int index);  // 0x18b00, opens the game detail window
    virtual int finished() { return isDir == 0; }
    EmuInfo *emu;           // +0x08
    std::string imgpath;    // +0x0c
    std::string rompath;    // +0x24
    std::string label;      // +0x3c
    bool detail;            // +0x54  has a detail window
    int isDir;              // +0x58  opens a ROM folder instead of launching
    std::string folderArg;  // +0x5c  handed to the folder window (MenuWindow::parentPath)
};

class GeneralAction : public Action
{
public:
    GeneralAction(int result, int (*fn)(void *), void *arg) : Action(result)
    {
        this->fn = fn;
        this->arg = arg;
        verbose = false;
    } // 0x39bac
    virtual ~GeneralAction() {}
    virtual int execute(void *param);                        // 0x19ecc
    int (*fn)(void *);  // +0x08
    void *arg;          // +0x0c
    bool verbose;       // +0x10
};

class MenuAction : public Action
{
public:
    MenuAction(ListMenu *menu, int result) : Action(result)
    {
        this->menu = menu;
        itemIndex = 0;
    }
    virtual ~MenuAction() {}
    ListMenu *menu;     // +0x08
    int itemIndex;        // +0x0c
};

class DeleteRomAction : public MenuAction
{
public:
    DeleteRomAction(ListMenu *menu, int result) : MenuAction(menu, result) {} // 0x39d5c
    virtual ~DeleteRomAction() {}
    virtual int execute(void *param);                        // 0x1a44c (p03)
};

class ClearFavoriteAction : public MenuAction
{
public:
    ClearFavoriteAction(ListMenu *menu, int result) : MenuAction(menu, result) {} // 0x39e18
    virtual ~ClearFavoriteAction() {}
    virtual int execute(void *param);                        // 0x19f64
};

class ClearRecentAction : public MenuAction
{
public:
    ClearRecentAction(ListMenu *menu, int result) : MenuAction(menu, result) {} // 0x39ed4
    virtual ~ClearRecentAction() {}
    virtual int execute(void *param);                        // 0x19fd8
};

class MenuItemAction : public Action
{
public:
    MenuItemAction(TextItem *item, int result) : Action(result) { this->item = item; }
    virtual ~MenuItemAction() {}
    TextItem *item;     // +0x08
};

class AddFavoriteAction : public MenuItemAction
{
public:
    AddFavoriteAction(TextItem *item, int result) : MenuItemAction(item, result) {} // 0x3a054
    virtual ~AddFavoriteAction() {}
    virtual int execute(void *param);                        // 0x1a02c
};

class RemoveFavoriteAction : public MenuItemAction
{
public:
    RemoveFavoriteAction(TextItem *item, int result) : MenuItemAction(item, result) {} // 0x3a110
    virtual ~RemoveFavoriteAction() {}
    virtual int execute(void *param);                        // 0x19acc
};

#include "FileBrowserCache.h"

class SearchAction : public Action
{
public:
    SearchAction(ListMenu *arg, int id, void *emu) : Action(-1)
    {
        this->arg = arg;
        this->id = id;
        this->emu = emu;
    } // 0x3a24c
    virtual ~SearchAction() {}
    virtual int execute(void *param);                        // 0x19b04
    ListMenu *arg;      // +0x08  originating menu
    int id;             // +0x0c  menu id of the result window
    void *emu;          // +0x10
};

class GlobalSearchAction : public Action
{
public:
    GlobalSearchAction() : Action(-1) {}                     // 0x3a32c
    virtual ~GlobalSearchAction() {}
    virtual int execute(void *param);                        // 0x19c18
};

class PopupSearchIME : public Action
{
public:
    PopupSearchIME(ListMenu *arg, int id, void *emu) : Action(-1)
    {
        this->arg = arg;
        this->id = id;
        this->emu = emu;
    } // 0x3a3dc
    virtual ~PopupSearchIME() {}
    virtual int execute(void *param);                        // 0x19d24
    ListMenu *arg;      // +0x08  originating menu
    int id;             // +0x0c
    void *emu;          // +0x10
};

class PopupDialogAction : public Action
{
public:
    PopupDialogAction(int title, int text, bool flag, Action *action) : Action(-1)
    {
        this->title = title;
        this->text = text;
        this->flag = flag;
        this->action = action;
    } // 0x3a4bc
    virtual ~PopupDialogAction() {}
    virtual int execute(void *param);                        // 0x19dc0
    int title;          // +0x08
    int text;           // +0x0c
    bool flag;          // +0x10
    Action *action;     // +0x14  run by the dialog
};

class PopupProgressAction : public Action
{
public:
    PopupProgressAction(int title, Action *action) : Action(-1)
    {
        this->title = title;
        this->action = action;
    } // 0x3a5a8
    virtual ~PopupProgressAction() {}
    virtual int execute(void *param);                        // 0x19e44
    int title;          // +0x08
    Action *action;     // +0x0c
};

#include "TextMenuDrawer.h"
#include "MultiDBCachedTextMenu.h"
#include "ImeWindow.h"


int enableSwapAction(void *arg);            // 0x38834 (p11)
// A ROM (or ROM folder) in a list; sizeof == 0x74. Ctor 0x39948, dtors 0x39ac4/0x39b34 and
// finished 0x39b6c are inline in the tail.

// The p10 windows p01 constructs (GameDetailWindow, ProgressDialog, DialogWindow, CountDownWindow)
// are declared with the other windows at the end of this file.
void writeCmdToRun(const char *cmd);        // 0x17fc0
void drawBackground(SDL_Surface *dst);      // 0x17450

// p02: actions. Inline ctors/dtors sit in the tail at 0x3984c-0x3a678, in this declaration order
// (EmuAction and GeneralAction are declared with p01 above; DeleteRomAction (p03) sits in tail order;
// FileAction is in FileBrowserCache.h).
// Every class here inherits Action's slots 3/4 (0x397fc returns -1, 0x39828 returns 0) except AppAction.
struct cJSON;
int clearRecent(std::list<RecentItem> *list);                // 0x120e20 (tu_11cdc8)
bool isFavourite(char *buf, FILE *f, const char *rompath);   // 0x1b210 (p03)

// p02 render helpers (0x18dd4-0x19acc): busy animation while a background job runs. g_launchCmd
// doubles as the counter text buffer, g_titleFont/g_wifiEmptyColor/g_background are p01's globals.

extern SDL_Color g_selectedTextColor;                            // 0x1822a0
int digitCount(int n);                                       // 0x18dd4

SDL_Surface *createSurface(int w, int h);                    // 0x1d720 (p03)
// Argument of the busy-screen thread (SDL_CreateThread from IconMenu::onSelect, createMenu, ...).
// Its constructor is the {0, -1, 0} one at 0x3adc4 (in the tail), which exactly these callers use.

int busyScreenThread(void *arg);                             // 0x193a8
int loadingScreenThread(void *arg);                          // 0x198a8


// Base (RTTI "MenuAction", ctor 0x39c8c, vtable 0x1546e0) of the actions that reload a list afterwards.


// Base (RTTI "MenuItemAction", ctor 0x39f90, vtable 0x1546c4) of the actions on one list item.


// p09

extern char *g_themePath;                   // 0x181970
extern SDL_Surface *g_backBuffer;           // 0x181928
extern const char g_skinSubdir[];           // 0x151a48 (second argument of every skin load)
SDL_Surface *loadSkinImage(const char *theme, const char *sub, const char *file,
                           SDL_Surface *fmt, bool flags);   // 0x123af0
void freeSurfaceCallback(SDL_Surface *s);  // 0x2e318

// ===========================================================================================
// Declarations shared by the part files p03-p11. Each class's out-of-line methods live in the
// part named next to it. Virtual slot order follows the original vtables and must not change.
// ===========================================================================================
#include "Theme.h"
#include "Language.h"

// Globals used by two or more parts (names as in the units that define them).
extern std::vector<AppInfo> g_appFolder;            // 0x181938  /mnt/SDCARD/App
extern std::vector<EmuInfo> g_emuFolders;          // 0x181944  /mnt/SDCARD/Emu
extern std::vector<EmuInfo> g_rappFolders;          // 0x181950  /mnt/SDCARD/RApp
extern std::vector<Theme> g_themeList;              // 0x18195c
extern std::vector<RomListPosition> g_romListPositions; // 0x181b00  /appconfigs/romwinidx.json
extern TTF_Font *g_iconFont;                        // 0x182180
extern TTF_Font *g_smallFont;                       // 0x182184
extern SDL_Surface *g_listNumber;                   // 0x1821b8
extern SDL_Surface *g_numberBg;                     // 0x1821bc
extern SDL_Color g_wifiSelectedColor;               // 0x182280
extern SDL_Color g_imeTextColor;                    // 0x182284
extern SDL_Color g_emptyColor;                      // 0x1822a4

class TextMenu;
class DBCachedTextMenu;

// libgamename import, C++ linkage (_Z11GetGameNamePKcS0_); used by p03 and p05.
char *GetGameName(const char *langFile, const char *name);

// Non-member functions called across parts (p01/p02 functions are defined before every later part).
// p03
void saveFavorites(const char *path, std::vector<void *> *items); // 0x1a4bc  writes a favourites file
void removeFavorite(const char *label, const char *path);   // 0x1a75c  removes a favourite
TextMenu *buildFavoriteList(const char *path);                    // 0x1a924  favourites list
TextMenu *buildRecentList(std::list<RecentItem> *list);         // 0x1b488  recent games
TextMenu *buildAppList(std::vector<AppInfo> *apps);          // 0x1bbfc  app list
int setLcdColor(int luma, int hue, int saturation, int contrast); // 0x1bea4  MI_DISP LCD params
void initAudioSettings(void *shm);                              // 0x1bf90  key shm
bool removeDeletedFavorites(void *menu, int id);                     // 0x1c0ac  deletes removed favourites (id 2)
int restoreWindowState(std::vector<TrimusWindow *> *stack);     // 0x1c1fc
void saveWindowState(std::vector<TrimusWindow *> *stack, const char *path); // 0x1c544
void setRomListPosition(std::vector<RomListPosition> *list, const std::string &path,
                        int cur, int start, int selected);      // 0x1c760  JSON keys pos, start, end
int getRomListPosition(std::vector<RomListPosition> *list, std::string path,
                       RomListPosition *out);                   // 0x1c8d0
int loadRomListPositions(std::vector<RomListPosition> *list);   // 0x1c994
void saveRomListPositions(std::vector<RomListPosition> *list); // 0x1cc90
SDL_Surface *loadThumbnail(const char *path, int w, int h);     // 0x1cee0
std::string makeThumbPath(std::string *game, const char *img);  // 0x1d318
// createSurface 0x1d720 and isFavourite 0x1b210 are declared with p02 above.
// p04
int drawListNumber(int n, SDL_Rect *rect, SDL_Surface *dst, TTF_Font *font); // 0x1e868
// p05
char *romFolderName(char *s);                           // 0x20b1c: last path component
int prepareRomCache(EmuInfo *emu, int arg, std::string dir, std::string search, char *buf,
                    int size);                              // 0x22d90: arg is the list's window id
DBCachedTextMenu *buildRomList(EmuInfo *emu, int arg, std::string *dir,
                               std::string *search);      // 0x23318
// p07
void applyPanelTiming();                                    // 0x2b5bc
// p08
int findEmuForFile(const char *name);                           // 0x2cfc0: Emu index or -1
// p09
int drawPageCount(SDL_Surface *dst, int current, int total); // 0x311f4
// freeSurfaceCallback 0x2e318 is declared with p09's globals above.
// p11
const char *windowTitle(int id);                                        // 0x374bc
ListMenu *createRomListMenu(EmuInfo *, int id, std::string *dir, std::string *search); // 0x37610
ListMenu *createMenu(int id, std::string path);                 // 0x37784  path by value (caller builds a temp)
void flipScreen();                                              // 0x36c20
void flushPendingWindows();                                     // 0x36cb8
void initFramebuffer();                                         // 0x36dbc
void runWindowLoop(std::vector<TrimusWindow *> *, std::atomic<bool> *); // 0x36ee8
void createRootWindow(std::vector<TrimusWindow *> *);          // 0x373c4
void startBackgroundThread();                                   // 0x38868
// pushWindow 0x36d88 and enableSwapAction 0x38834 are declared with p01 above.
// Inline in the tail (0x3ad44), called by MenuWindow::onShow/draw: menu ids with a ROM/app list.


// --- Menus. Bases are in ListMenu.h / TextMenuDrawer.h (p03 ctor/setRect, p04 the rest).

// p05. vtable 0x153da8; sizeof 0x104.
#include "TextMenu.h"

// p06. vtable 0x153d40; sizeof 0x430 (layout parallels MultiDBCachedTextMenu).
class DBCacheWorker : public BGWorker
{
public:
    DBCacheWorker(sqlite3 *db, const std::string &dbName, const std::string &table, int offset, int len,
                  int rowWindowId, LruCache *items, LruCache *data, EmuInfo *emu, std::string *dir,
                  std::string *search, bool flag)                // 0x3a74c
    {
        this->db = db;
        this->dbName = dbName;
        this->table = table;
        this->offset = offset;
        this->len = len;
        this->rowWindowId = rowWindowId;
        this->items = items;
        this->data = data;
        this->emu = emu;
        this->dir = *dir;
        this->search = *search;
        this->flag = flag;
    }
    virtual ~DBCacheWorker() {}                                // 0x3a8d8 (D0 0x3a954)
    virtual void run();                                        // 0x23548
    virtual void finished();                                   // 0x2407c
    char sqlBuffer[0x200];     // +0x04  SQL/cache-key formatting buffer
    sqlite3 *db;               // +0x204
    std::string dbName;           // +0x208
    std::string table;            // +0x220
    std::string dir;              // +0x238
    std::string search;           // +0x250
    bool flag;             // +0x268
    int offset;           // +0x26c
    int len;              // +0x270
    int rowWindowId;                // +0x274
    LruCache *items;            // +0x278
    LruCache *data;             // +0x27c
    EmuInfo *emu;              // +0x280
};

class DBCachedTextMenu : public TextMenuDrawer
{
public:
    DBCachedTextMenu(const char *dbName, EmuInfo *emu, int listWindowId, std::string search); // 0x25240
    virtual ~DBCachedTextMenu();                               // 0x2540c (D0 0x254dc)
    virtual void restorePosition(int savedCur, int savedStart, int savedEnd); // 0x246f8
    virtual void *getItem(int index);                          // 0x24410
    virtual void *getItemData(int index);                           // 0x24584
    virtual int count() { return total; }                     // 0x3a988, inline
    virtual int moveDown();                                     // 0x24128
    virtual int moveUp();                                    // 0x24164
    virtual void removeItem(int);                                 // 0x24a14
    virtual void setSearch(const char *);                         // 0x24d98
    virtual int pageUp();                                    // 0x240b4
    virtual int pageDown();                                    // 0x240ec
    void prefetchNext();                                      // 0x241a0
    void prefetchPrevious();                                  // 0x242dc
    void setTotal(int n) { total = n; }                        // 0x3a9b0, inline
    int open(const char *path, const char *table, const char *folder); // 0x24834
    WorkerQueue workers;           // +0xf8   3 threads
    LruCache items;             // +0x118  rows (free 0x24d28)
    LruCache data;              // +0x12c  (free 0x24d6c)
    sqlite3 *cacheDb;          // +0x140  database connection
    std::string name;              // +0x144
    std::string tablePrefix;            // +0x15c
    int listWindowId;          // +0x174  window id given to each row's action
    int total;             // +0x178  0
    std::string dbName;            // +0x17c
    char keyBuffer[0x280];     // +0x194  SQL/cache-key formatting buffer
    EmuInfo *emuInfo;          // +0x414  emulator record (constructor argument)
    std::string dir;               // +0x418
};

// p06. Not polymorphic; sizeof 0x80.
class IconMenuItem
{
public:
    IconMenuItem(int labelId, const char *icon, const char *iconSelected,
                 SDL_Surface *background, SDL_Surface *backgroundSelected, int menuId, void *param, bool flag);                         // 0x256f0
    IconMenuItem(const char *label, const char *icon, const char *iconSelected,
                 SDL_Surface *background, SDL_Surface *backgroundSelected, int menuId, void *param, bool flag);             // 0x258d4 (labelId -1)
    ~IconMenuItem();                                           // 0x25a8c
    void reload();                                            // 0x25b5c
    SDL_Surface *iconSurface;       // +0x00
    SDL_Surface *iconSelectedSurface; // +0x04
    SDL_Surface *background, *backgroundSelected; // +0x08, +0x0c, borrowed surfaces
    std::string label;             // +0x10
    std::string verboseLabel;             // +0x28
    int labelId;           // +0x40
    void *param;             // +0x44
    bool flag;              // +0x48
    int menuId;            // +0x4c  createMenu id opened by IconMenu::onSelect
    std::string icon;              // +0x50
    std::string iconSelected;      // +0x68
};

// p06. vtable 0x153cf0; sizeof 0xa0.
class IconMenu : public ListMenu
{
public:
    IconMenu(int rows, int columns, bool stripes, bool dots, int marginX, int marginY, bool themeOffsets); // 0x25c8c
    virtual ~IconMenu();                                       // 0x25e48 (D0 0x25ec8)
    virtual int handleKey();                                     // 0x27bc4
    virtual int onSelect();                                     // 0x27724 onSelect
    virtual void *getItem(int index);                          // 0x26fa0
    virtual void drawList(SDL_Surface *dst);                     // 0x25efc draw
    virtual int count() { return items.size(); }              // 0x3a9e0, inline
    virtual void reloadTheme();                                    // 0x27a68
    void freeItems();                                          // 0x2797c
    void addItem(void *item);                          // 0x27a30
    void setIconCropSize(int a, int b)
    {
        iconCropWidth = a;
        iconCropHeight = b;
    } // 0x3aa10, inline
    void setFontFn(TTF_Font *(*fn)()) { fontFn = fn; }         // 0x3aa50, inline
    void setLabelOffsetY(int v) { labelOffsetY = v; }                    // 0x3aa80, inline
    std::vector<void *> items; // +0x48, IconMenuItem rows; shared specialization with TextMenu
    int gridColumns;           // +0x54
    int gridRows;           // +0x58
    bool showStripes;            // +0x5c
    bool showPageDots;            // +0x5d
    int gridMarginX;           // +0x60
    int gridMarginY;           // +0x64
    int iconCropWidth;           // +0x68  0
    int iconCropHeight;           // +0x6c  0
    TTF_Font *(*fontFn)();         // +0x70  0x25c68 (returns g_iconFont); createMenu sets 0x37760 (g_smallFont)
    int labelOffsetY;           // +0x74  0
    std::string loadedThemePath;             // +0x78
    int iconOffsetX, iconOffsetY, textOffsetY; // +0x90  copied from the Theme's gridIconOffsetX/Y and gridTextOffsetY
    bool useThemeOffsets;            // +0x9c
};

// p06. Not polymorphic; sizeof 0x24.
// What a settings row does (SettingMenuItem::handler): A opens or confirms, left/right adjust
// the value. Rows 1, 4, 9 and 20 exist in the code but createMenu never adds them.
enum SettingHandler {
    SETTING_KEYMAP = 0,          // KeySettingWindow
    SETTING_VOLUME = 1,          // 0-20, previewed with a sound
    SETTING_BGM_VOLUME = 2,      // menu music 0-20
    SETTING_BRIGHTNESS = 3,      // backlight 1-10
    SETTING_REFRESH_ROMS = 4,    // delete the ROM caches
    SETTING_FACTORY_RESET = 5,
    SETTING_DEVICE_INFO = 7,
    SETTING_IO_TEST = 8,         // KeyTestWindow2 (labelled "Hardware Test")
    SETTING_ADC_INFO = 9,
    SETTING_LANGUAGE = 11,
    SETTING_HIBERNATE = 12,      // sleep after 5/15/30 min or never
    SETTING_COLOR = 13,
    SETTING_UNUSED_14 = 14,      // drawn as adjustable, but nothing adjusts or adds it
    SETTING_THEME = 15,
    SETTING_FIXES = 16,          // the "Fixes" submenu (WIN_FIXES)
    SETTING_AUDIOFIX = 17,       // on/off; also toggles /tmp/no_audioserver
    SETTING_POWER_OFF = 18,
    SETTING_WIFI = 19,
    SETTING_HOTSPOT = 20,
};
class SettingMenuItem
{
public:
    SettingMenuItem(int labelId, const char *icon, int handler); // 0x282d0, integer settings selector
    ~SettingMenuItem();                                        // 0x283b8
    void reload();                                             // 0x28408, called by SettingMenu::reloadTheme
    SDL_Surface *iconSurface;       // +0x00
    int labelId;           // +0x04
    int handler;           // +0x08  settings selector, switched on in p07 (not a pointer)
    std::string icon;              // +0x0c
};

// p07. vtable 0x153c9c; sizeof 0x128.
class SettingMenu : public TextMenu
{
public:
    SettingMenu();                                             // 0x2849c  TextMenu(1, 1, 6, 0)
    virtual ~SettingMenu();                                    // 0x28614 (D0 0x28748)
    virtual int handleKey();                                     // 0x2a5d4
    virtual void drawList(SDL_Surface *dst);                     // 0x28994
    virtual void reloadTheme();                                    // 0x2877c
    virtual void freeListItem();                                    // 0x2a6ec
    void decrease();                                          // 0x295a8
    void increase();                                          // 0x298ec
    void select();                                            // 0x29dc8
    void *volumePreviewTimer;          // +0x104  delayed volume-preview timer
    SDL_Surface *settingLeftArrow;        // +0x108  left-arrow icon
    SDL_Surface *rightArrow;        // +0x10c  right-arrow icon (separate from selectedBackground)
    std::string themePath;         // +0x110
};

// p07. vtable 0x153c48; sizeof 0x120.
class DeviceInfoMenu : public TextMenu
{
public:
    DeviceInfoMenu();                                          // 0x2a83c  TextMenu(1, 1, 6, 0)
    virtual ~DeviceInfoMenu();                                 // 0x2a934 (D0 0x2a9d4)
    virtual int onSelect() { return 0; }                        // 0x3aab0, inline
    virtual void drawList(SDL_Surface *dst);                     // 0x2aae4
    virtual void reloadTheme();                                    // 0x2aa08
    virtual void freeListItem();                                    // 0x2b504
    void loadInfo();                                           // 0x2b0f8, populate the seven device-information rows
    char *firmwareVersion;          // +0x104  loaded firmware-version buffer
    std::string themePath;         // +0x108
};

// p08. vtable 0x153bf4; sizeof 0x10c.
class ColorMenu : public TextMenu
{
public:
    ColorMenu();                                               // 0x2b788  TextMenu(0, 1, 6, 0)
    virtual ~ColorMenu();                                      // 0x2b8bc (D0 0x2b96c)
    virtual int handleKey();                                     // 0x2c274
    virtual int onSelect();                                     // 0x2c73c
    virtual void drawList(SDL_Surface *dst);                     // 0x2b9a0
    virtual void freeListItem();                                    // 0x2c760
    void loadItems();                                          // 0x2c824  four LCD controls
    SDL_Surface *leftArrow;        // +0x104
    SDL_Surface *colorRightArrow;        // +0x108  right arrow (selection background is +0x4c)
};

// p08. vtable 0x153ba0; sizeof 0x110.
class LanguageMenu : public TextMenu
{
public:
    LanguageMenu(std::vector<LanguageChoice> languages); // 0x2c950
    virtual ~LanguageMenu();                                   // 0x2caa8 (D0 0x2caf4)
    virtual int onSelect();                                     // 0x2cb28
    std::vector<LanguageChoice> languages; // +0x104  filename/name pairs
};

// p08. vtable 0x153b4c; sizeof 0x104 (lists g_themeList).
class ThemeMenu : public TextMenu
{
public:
    ThemeMenu();                                               // 0x2cc14  TextMenu(0, 1, 6, 0)
    virtual ~ThemeMenu();                                      // 0x2cd80 (D0 0x2cdbc)
    virtual int onSelect();                                     // 0x2cdf0
};

// p08. vtable 0x153afc; sizeof 0x80.
class FileBrowserMenu : public ListMenu
{
public:
    FileBrowserMenu(const char *path);                         // 0x2d700  ListMenu(false)
    virtual ~FileBrowserMenu();                                // 0x2d908 (D0 0x2da58)
    virtual int handleKey();                                     // 0x2e0f0
    virtual int onSelect();                                     // 0x2dad4
    virtual void *getItem(int index);                          // 0x2db8c
    virtual void drawList(SDL_Surface *dst);                     // 0x2dcbc
    virtual int count();                                      // 0x2da8c
    SDL_Surface *itemIcon(TextItem *item);                     // 0x2dc14
    FileBrowserCache cache;         // +0x48  filter 0x2d574
    std::vector<void *> *dirItems; // +0x64  cache.open(path)
    std::vector<SDL_Surface *> icons;  // +0x68  file and folder icons
    SDL_Surface *fileIcon;         // +0x74
    SDL_Surface *selectionBar;         // +0x78
    bool highlightSelected;            // +0x7c  true
};

// --- Background workers (BGWorker is in MenuHelpers.h).

// p06 (run/finished); ctor and dtors inline in the tail (0x3a74c-0x3a954). sizeof 0x284.


// p08 (run/finished); created by MenuWindow (p09, 0x2f6c4). Ctor and dtors inline (0x3af38-0x3b01c).
// sizeof 0x24.


// --- Windows (TrimusWindow.h; MenuWindow.h is p09's).

// p09. vtable 0x153a94; sizeof 0x10. Ctor 0x32040: TrimusWindow(title, -1).
#include "MenuWindow.h"

class FloatWindow : public TrimusWindow
{
public:
    FloatWindow(int titleLabelId);                             // 0x32040
    virtual ~FloatWindow();                                    // 0x32094 (D0 0x320f0)
    virtual int handleInput() { return -1; }                  // 0x3ab64, inline
    virtual int draw(SDL_Surface *dst);                       // 0x32124
    virtual void onHide() {}                                   // 0x3ab88, inline
    virtual void onShow();                                     // 0x32160
    virtual void saveState(cJSON *obj) {}                      // 0x3aba8, inline
    virtual int restoreState(cJSON *obj) { return 0; }        // 0x3abcc, inline
    virtual void update() {}                                   // 0x3abf4, inline
    SDL_Surface *backdrop;         // +0x0c  dimmed copy of the screen behind the pop-up
};

// p10. vtable 0x153a60; sizeof 0x18. Created by MenuWindow::handleInput.
class PopUpWindow : public FloatWindow
{
public:
    PopUpWindow(int titleLabelId, TextMenu *menu, const char *background); // 0x3225c
    virtual ~PopUpWindow();                                    // 0x3247c (D0 0x32534)
    virtual int handleInput();                                // 0x32784
    virtual int draw(SDL_Surface *dst);                       // 0x32588
    virtual void onHide();                                     // 0x32568
    TextMenu *menu;                 // +0x10
    SDL_Surface *background;       // +0x14
};

// p10. vtable 0x153a2c; sizeof 0x3c.
class NotifyWindow : public FloatWindow
{
public:
    NotifyWindow(const char *text, const char *buttonLabel, const char *buttonImage, void (*callback)()); // 0x327e4back
    virtual ~NotifyWindow();                                   // 0x32920 (D0 0x329cc)
    virtual int handleInput();                                // 0x32d0c
    virtual int draw(SDL_Surface *dst);                       // 0x32d74
    virtual void onShow();                                     // 0x32aa0
    void *notifyTimer;           // +0x10  SDL timer
    std::string text;              // +0x14
    SDL_Surface *notifyBackground;         // +0x2c
    SDL_Surface *notifyButton;         // +0x30
    const char *buttonLabel;           // +0x34  borrowed button-label text
    void (*callback)();       // +0x38
};

// p10. vtable 0x1539f8; sizeof 0xcc. NotifyWindow(<0x151c98>, ?, "skin/bg-button-f.png", onTimeout).
class CountDownWindow : public NotifyWindow
{
public:
    CountDownWindow(int title, void (*onTimeout)(), void (*onCancel)());   // 0x33060
    virtual ~CountDownWindow();                                // 0x330fc (D0 0x33158)
    virtual int handleInput();                                // 0x332d8
    virtual void onShow();                                     // 0x331ec
    int title;             // +0x3c
    void *tickTimer;           // +0x40  countdown tick timer
    int seconds;           // +0x44  10
    void (*onCancel)();       // +0x48
    char countdownText[0xcc - 0x4c]; // +0x4c  formatted countdown text
};

// p10. vtable 0x1539c4; sizeof 0x34.
class DialogWindow : public FloatWindow
{
public:
    DialogWindow(int title, int text, bool flag, Action *action);   // 0x333ec
    virtual ~DialogWindow();                                   // 0x33570 (D0 0x33630)
    virtual int handleInput();                                // 0x3400c
    virtual int draw(SDL_Surface *dst);                       // 0x33bac
    virtual void onShow();                                     // 0x33664
    int title;             // +0x10
    int text;              // +0x14
    int drawFallback(SDL_Surface *);                           // 0x33934
    int selected;          // +0x18  button: 0 Cancel, 1 OK (default)
    SDL_Surface *popupBackground;         // +0x1c
    SDL_Surface *buttonHighlight;         // +0x20
    SDL_Surface *iconA;             // +0x24  g_iconA
    SDL_Surface *iconB;             // +0x28  g_iconB
    Action *action;            // +0x2c
    bool flag;              // +0x30
    bool noPopupSkin;            // +0x31
};

// p10. vtable 0x153990; sizeof 0x3c.
class ProgressDialog : public FloatWindow
{
public:
    ProgressDialog(int title, Action *action);                 // 0x34184
    virtual ~ProgressDialog();                                 // 0x342ac (D0 0x3437c)
    virtual int handleInput();                                // 0x349f0
    virtual int draw(SDL_Surface *dst);                       // 0x34468
    virtual void onHide();                                     // 0x34438
    virtual void onShow();                                     // 0x343b0
    void runAction();                                          // 0x343f4
    int title;             // +0x10
    std::string text;              // +0x14
    SDL_Surface *progressButton;         // +0x2c
    SDL_Surface *progressBackground;         // +0x30
    Action *action;            // +0x34
    SDL_Thread *actionThread;           // +0x38  action worker
};

// p10. vtable 0x15395c; sizeof 0x9c.
class GameDetailWindow : public TrimusWindow
{
public:
    GameDetailWindow(int title, int id);                       // 0x34a14
    GameDetailWindow(int title, int id, const char *name, const char *item, const char *launch,
                     const char *icon, int listIndex, int type, bool favorite); // 0x34b1c
    virtual ~GameDetailWindow();                               // 0x34d0c (D0 0x34d98)
    virtual int handleInput();                                // 0x35bbc
    virtual int draw(SDL_Surface *dst);                       // 0x356e4
    virtual void onHide();                                     // 0x34dcc
    virtual void onShow();                                     // 0x34e6c
    virtual void saveState(cJSON *obj);                        // 0x35fe4
    virtual int restoreState(cJSON *obj);                     // 0x36140
    virtual void update();                                     // 0x34f84
    virtual void animateIn(SDL_Surface *dst, FlipFn flip);     // 0x36288
    virtual void animateOut(SDL_Surface *dst, FlipFn flip);    // 0x36534
    void loadInfo();                                           // 0x35080, wrap title lines
    void resetDetails(const char *, const char *, const char *, const char *, int, int, bool); // 0x353a0
    void resetFromItem(TextItem *, int);                        // 0x35538
    void captureBackground(SDL_Surface *);                     // 0x3675c
    std::string name;              // +0x0c
    std::string item;              // +0x24
    std::string thumbPath;         // +0x3c  makeThumbPath(launch, icon)
    std::string launch;            // +0x54
    std::vector<std::string> nameLines; // +0x6c  wrapped title lines
    int listIndex;             // +0x78  item index
    int type;              // +0x7c
    bool favorite;          // +0x80
    SDL_Surface *thumbnailImage, *favoriteImage, *animationCapture; // thumbnail, favourite mark, animation capture
    int openedIndex;           // +0x90  list index when opened (the slide plays only for that game)
    bool animatePending;            // +0x94  true
    TextMenuDrawer *parentMenu;        // +0x98  parent text/ROM menu
};

// p11. vtable 0x153928; sizeof 0x10.
class ADCInfoWindow : public TrimusWindow
{
public:
    ADCInfoWindow(int title, int id);                          // 0x36838
    virtual ~ADCInfoWindow();                                  // 0x368b8 (D0 0x36914)
    virtual int handleInput();                                // 0x36948
    virtual int draw(SDL_Surface *dst);                       // 0x36990
    virtual void onHide() {}                                   // 0x3ac14, inline
    virtual void onShow() {}                                   // 0x3ac34, inline
    virtual void saveState(cJSON *obj) {}                      // 0x3ac54, inline
    virtual int restoreState(cJSON *obj) { return 0; }        // 0x3ac78, inline
    virtual void update() {}                                   // 0x3aca0, inline
    void *timer;                    // +0x0c  SDL timer
};

// Preserve RapidXml's original assertion diagnostic path, independent of the checkout path.
#include <cstdlib>
#include <cassert>
#include <new>
#define RAPIDXML_NO_STDLIB // Standard declarations above; avoid re-including unguarded assert.h.
#pragma push_macro("__assert_fail")
#define __assert_fail(condition, file, line, function) __assert_fail(condition, "/home/alfchen/TheData/ProjectData2/miyoo_sigmastar/ssd202_miyoo354/sdk_v008/app/UI/../../../project/release/include/cust_inc/rapidxml.hpp", line, function)
#include <rapidxml.hpp>
#pragma pop_macro("__assert_fail")
#undef RAPIDXML_NO_STDLIB

// Windows that show a list with a "current/total" count and the header line.
inline bool isListMenuId(int id)
{
    return id == 5 || id == 17 || id == 2 || id == 10 || id == 3 || id == 4 || id == 24;
}

struct BusyData {
    BusyData()
    {
        title = 0;
        type = -1;
        unused8 = 0;
    }        // 0x3adc4, inline
    const char *title;      // "sdata->title"
    int type;               // window id shown in the frame
    int unused8;             // never read
};

class LoadSurfaceWorker : public BGWorker
{
public:
    inline LoadSurfaceWorker(std::string path, LruCache *cache); // 0x3af38
    inline virtual ~LoadSurfaceWorker();                           // 0x3afd0
    virtual void run();                                        // 0x2e344
    virtual void finished();                                   // 0x2e3ec
    std::string path;             // +0x04
    LruCache *cache;            // +0x1c
    int unused20;          // +0x20  never read
};

// Sizes from operator new / operator delete, offsets from the constructors.
#include <stddef.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(sizeof(ListMenu) == 0x48 && sizeof(TextMenuDrawer) == 0xf8 && sizeof(TextMenu) == 0x104, "menu bases");
static_assert(offsetof(TextMenuDrawer, textRowHeight) == 0x58 && offsetof(TextMenuDrawer, textScratch) == 0xa0 &&
                  offsetof(TextMenuDrawer, drawStripes) == 0xc1 && offsetof(TextMenuDrawer, background) == 0xe0,
              "TextMenuDrawer");
static_assert(sizeof(DBCachedTextMenu) == 0x430 && offsetof(DBCachedTextMenu, total) == 0x178 &&
                  offsetof(DBCachedTextMenu, dir) == 0x418,
              "DBCachedTextMenu");
static_assert(sizeof(IconMenuItem) == 0x80 && offsetof(IconMenuItem, menuId) == 0x4c, "IconMenuItem");
static_assert(sizeof(IconMenu) == 0xa0 && offsetof(IconMenu, loadedThemePath) == 0x78 && offsetof(IconMenu, useThemeOffsets) == 0x9c, "IconMenu");
static_assert(sizeof(SettingMenuItem) == 0x24 && sizeof(SettingMenu) == 0x128 && sizeof(DeviceInfoMenu) == 0x120, "settings");
static_assert(sizeof(ColorMenu) == 0x10c && sizeof(LanguageMenu) == 0x110 && sizeof(ThemeMenu) == 0x104, "p08 menus");
static_assert(sizeof(FileBrowserMenu) == 0x80 && offsetof(FileBrowserMenu, dirItems) == 0x64, "FileBrowserMenu");
static_assert(sizeof(DBCacheWorker) == 0x284 && offsetof(DBCacheWorker, flag) == 0x268, "DBCacheWorker");
static_assert(sizeof(LoadSurfaceWorker) == 0x24, "LoadSurfaceWorker");
static_assert(sizeof(FloatWindow) == 0x10 && sizeof(PopUpWindow) == 0x18 && sizeof(NotifyWindow) == 0x3c, "float windows");
static_assert(sizeof(CountDownWindow) == 0xcc && sizeof(DialogWindow) == 0x34 && sizeof(ProgressDialog) == 0x3c, "dialogs");
static_assert(sizeof(GameDetailWindow) == 0x9c && offsetof(GameDetailWindow, animatePending) == 0x94, "GameDetailWindow");
static_assert(sizeof(ADCInfoWindow) == 0x10 && sizeof(MenuWindow) == 0x11c && sizeof(TextItem) == 0x74, "windows, items");
static_assert(sizeof(EmuAction) == 0x74 && sizeof(AppAction) == 0x10 && sizeof(GeneralAction) == 0x14 &&
                  sizeof(DeleteRomAction) == 0x10 && sizeof(AddFavoriteAction) == 0x0c && sizeof(SearchAction) == 0x14 &&
                  sizeof(PopupDialogAction) == 0x18 && sizeof(PopupProgressAction) == 0x10 && sizeof(FileAction) == 0x24,
              "actions");
#pragma GCC diagnostic pop
