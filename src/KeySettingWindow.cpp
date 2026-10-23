// KeySettingWindow.cpp (0x1376fc-0x138c50): the button-mapping screen, and a touch-panel input thread.
//
// KeySettingWindow lets the user choose which physical button reports as which (L, L2, R, R2, X,
// A, B, Y): each press assigns the pressed button's name to the highlighted slot, and MENU saves
// the result as the keymap (setKeymap/saveSystemJson in Settings.c), which the kernel's button
// driver applies. The touch thread turns taps near the screen edges into arrow-button events; it
// is leftover code: nothing in this build starts it (startTouchInput has no caller).
// Body 5,288 bytes plus five inline virtuals; functions in address order.
#include <iostream>
#include "SdlApi.h"
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <linux/input.h>
#include "KeySettingWindow.h"
#include "LangStrings.h"
#include "Input.h"
#include "WindowIds.h"

struct SDL_Thread;
extern "C" {
const char *getKeymap();
void setKeymap(const char *);
void saveSystemJson(void *, const char *, int);
}
extern char *g_themePath;
extern char g_keyShm[];
extern int g_fontSize;
extern SDL_Surface *g_backBuffer, *g_keySettingFooter;
extern void *g_imeFont;
extern SDL_Color g_imeTextColor;
SDL_Surface *loadSkinImage(const char *, const char *, const char *, SDL_Surface *, bool); // 0x123af0
const char *languageString(int);
void drawBackground(SDL_Surface *); // 0x17450
void drawTitleBar(SDL_Surface *, const char *, bool); // 0x16f68
int drawText(SDL_Surface *, void *, SDL_Rect, const char *, SDL_Color, int, bool);
int drawIconLabel(SDL_Surface *, SDL_Surface *, int, int, const char *, const char *); // 0x170f8

// Touch thread state: last touch position, -1 until both axes are known for the current touch.
SDL_Thread *g_touchThread;
bool g_touchRunning, g_touchPressed;
int g_touchX = -1, g_touchY = -1;
static_assert(sizeof(KeySettingWindow) == 0x15c, "KeySettingWindow layout");

extern const char g_keyEventReadFormat[44];

// Opens a Linux evdev input device and logs its identity (bus, vendor, name, ...).
// Returns the file descriptor, or -1 if it cannot be opened or is not an input device.
int openInputDevice(const char *path)
{
    int fd = open(path, O_RDWR);
    int version;
    char name[80], physical[80], unique[80];
    input_id identity;
    if (fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", path, strerror(errno));
        return -1;
    }
    if (ioctl(fd, EVIOCGVERSION, &version)) {
        fprintf(stderr, "could not get driver version for %s, %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    if (ioctl(fd, EVIOCGID, &identity)) {
        fprintf(stderr, "could not get driver id for %s, %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    name[79] = 0;
    physical[79] = 0;
    unique[79] = 0;
    if (ioctl(fd, EVIOCGNAME(79), name) < 1) name[0] = 0;
    if (ioctl(fd, EVIOCGPHYS(79), physical) < 1) physical[0] = 0;
    if (ioctl(fd, EVIOCGUNIQ(79), unique) < 1) unique[0] = 0;
    printf("  bus:      %04x\n  vendor    %04x\n  product   %04x\n  version   %04x\n",
           identity.bustype, identity.vendor, identity.product, identity.version);
    printf("  name:     \"%s\"\n", name);
    printf("  location: \"%s\"\n  id:       \"%s\"\n", physical, unique);
    printf("  version:  %d.%d.%d\n", version >> 16, (version >> 8) & 255, version & 255);
    return fd;
}
// Thread body: reads touch events from /dev/input/event0 until stopTouchInput, and posts an
// arrow button for a touch in one of the four edge bands of the 640x480 screen (left/right:
// x < 100 or x > 540; top/bottom: y < 100 or y > 380; corners excluded). The event goes out as a
// user event whose code is the button, which handleGlobalInput passes through as the button.
int touchInputThread(void *)
{
    int fd = openInputDevice("/dev/input/event0");
    if (fd >= 0) {
        int count = 1;
        pollfd *fds = (pollfd *)calloc(1, sizeof(pollfd));
        int result;
        SDL_Event event;
        input_event input;
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        while (g_touchRunning) {
            result = poll(fds, count, 500);   // wake twice a second to check g_touchRunning
            if (fds[0].revents & POLLIN) {
                result = read(fds[0].fd, &input, sizeof(input));
                if (result < (int)sizeof(input)) {
                    fprintf(stderr, "could not get event\n");
                    break;
                }
                if (input.type == EV_KEY && input.code == BTN_TOUCH) {
                    printf(g_keyEventReadFormat, input.type, input.code, input.value);
                    // Finger down or up; either way the next position starts over.
                    if (input.value == 1)
                        g_touchPressed = true;
                    else if (input.value == 0)
                        g_touchPressed = false;
                    g_touchX = g_touchY = -1;
                }
                if (input.type == EV_ABS && (input.code == ABS_X || input.code == ABS_Y) && g_touchPressed) {
                    printf(g_keyEventReadFormat, input.type, input.code, input.value);
                    if (input.code == ABS_X)
                        g_touchX = input.value;
                    else if (input.code == ABS_Y)
                        g_touchY = input.value;
                    if (g_touchX >= 0 && g_touchY >= 0) {
                        if (g_touchX < 100 && g_touchY > 100 && g_touchY < 380) {
                            event.type = SDL_USEREVENT;
                            event.user.code = BUTTON_LEFT;
                            SDL_PushEvent(&event);
                        }
                        if (g_touchX > 540 && g_touchY > 100 && g_touchY < 380) {
                            event.type = SDL_USEREVENT;
                            event.user.code = BUTTON_RIGHT;
                            SDL_PushEvent(&event);
                        }
                        if (g_touchY < 100 && g_touchX > 100 && g_touchX < 540) {
                            event.type = SDL_USEREVENT;
                            event.user.code = BUTTON_UP;
                            SDL_PushEvent(&event);
                        }
                        if (g_touchY > 380 && g_touchX > 100 && g_touchX < 540) {
                            event.type = SDL_USEREVENT;
                            event.user.code = BUTTON_DOWN;
                            SDL_PushEvent(&event);
                        }
                    }
                }
            }
        }
        close(fd);
        free(fds);
    }
    return 0;
}
// Starts and stops the touch thread (stop waits for it; it notices within half a second).
void startTouchInput()
{
    g_touchRunning = true;
    g_touchThread = SDL_CreateThread(touchInputThread, NULL);
}
void stopTouchInput()
{
    g_touchRunning = false;
    SDL_WaitThread(g_touchThread, NULL);
}
// Codes returned by waitMappingKey. A, B, X, Y and MENU are the usual button codes, but the
// shoulder buttons are numbered L1, L2, R1, R2 here, unlike BUTTON_HOTKEY1-4 (L2, R2, L1, R1).
enum MappingKey {
    MAPPING_L1 = 12,
    MAPPING_L2 = 13,
    MAPPING_R1 = 14,
    MAPPING_R2 = 15,
};

// Waits for a button release and returns its code (MappingKey or BUTTON_A/B/X/Y/MENU); other
// buttons and events are ignored. Returns 0 if SDL stops delivering events.
int waitMappingKey()
{
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case MIYOO_KEY_A: return BUTTON_A;
                    case MIYOO_KEY_B: return BUTTON_B;
                    case MIYOO_KEY_X: return BUTTON_X;
                    case MIYOO_KEY_Y: return BUTTON_Y;
                    case MIYOO_KEY_L1: return MAPPING_L1;
                    case MIYOO_KEY_R1: return MAPPING_R1;
                    case MIYOO_KEY_L2: return MAPPING_L2;
                    case MIYOO_KEY_R2: return MAPPING_R2;
                    case MIYOO_KEY_MENU: return BUTTON_MENU;
                    default: break;
                }
                // Falls through to the shared default exit of the outer switch.
            default: break;
        }
    }
    return 0;
}
// Defined after use to keep the original .rodata order; its original allocation (0x162578) is
// 44 bytes, longer than the text.
extern const char g_keyEventReadFormat[44] = "read event type=%d, code=%d, value=%d\n";

KeySettingWindow::KeySettingWindow() : TrimusWindow(STR_KEY_SETTING, WIN_TEST)
{
    menuIcon = NULL;
    highlight = NULL;
    background = NULL;
    selected = 0;
    loadResources();
}
KeySettingWindow::~KeySettingWindow()
{
    if (menuIcon) SDL_FreeSurface(menuIcon);
    if (highlight) SDL_FreeSurface(highlight);
    if (background) SDL_FreeSurface(background);
}
// One button press. MENU saves the mapping and closes the window (returns 1); any mapped button
// assigns its name to the selected slot and moves to the next slot (returns -1: stay open).
int KeySettingWindow::handleInput()
{
    int key = waitMappingKey();
    switch (key) {
        case BUTTON_MENU:
            // Back to driver order: the first two pairs are swapped on screen (see loadResources).
            snprintf(keymap, 64, "%s,%s,%s,%s,%s,%s,%s,%s", keys[1].c_str(), keys[0].c_str(),
                     keys[3].c_str(), keys[2].c_str(), keys[4].c_str(), keys[5].c_str(), keys[6].c_str(), keys[7].c_str());
            setKeymap(keymap);
            saveSystemJson(g_keyShm, g_themePath, g_fontSize);
            sync();
            return 1;
        case BUTTON_A: keys[selected] = "A"; break;
        case BUTTON_B: keys[selected] = "B"; break;
        case BUTTON_X: keys[selected] = "X"; break;
        case BUTTON_Y: keys[selected] = "Y"; break;
        case MAPPING_L1: keys[selected] = "L"; break;
        case MAPPING_L2: keys[selected] = "L2"; break;
        case MAPPING_R1: keys[selected] = "R"; break;
        case MAPPING_R2: keys[selected] = "R2"; break;
    }
    printf("set key[%d] to %s\n", selected, keys[selected].c_str());
    selected++;
    if (selected > 7) selected = 0;
    return -1;
}
// Footer bar at y 420 with the MENU icon and its label, "SAVE AND EXIT".
void drawKeySettingFooter(SDL_Surface *dst, SDL_Surface *icon)
{
    int y = 420;
    int x = 20;
    if (g_keySettingFooter) {
        SDL_Rect dest;
        dest.x = 0;
        dest.y = y;
        SDL_Rect source;
        source.x = 0;
        source.y = 0;
        source.w = g_keySettingFooter->w;
        source.h = g_keySettingFooter->h;
        SDL_UpperBlit(g_keySettingFooter, &source, dst, &dest);
    }
    if (icon) drawIconLabel(dst, icon, x, y, 0, languageString(STR_SAVE_AND_EXIT));
}
// Background, title, footer, the highlight behind the selected slot, then the eight button names.
int KeySettingWindow::draw(SDL_Surface *dst)
{
    if (background)
        SDL_UpperBlit(background, NULL, dst, NULL);
    else
        drawBackground(dst);
    drawTitleBar(dst, getTitle(), 1);
    drawKeySettingFooter(dst, menuIcon);
    SDL_UpperBlit(highlight, NULL, dst, &positions[selected]);
    for (int i = 0; i < 8; i++)
        drawText(dst, g_imeFont, positions[i], keys[i].c_str(), g_imeTextColor, 1, i == selected);
    return 0;
}
// Loads the skin images and the slot positions (two on the left of the screen, six on the right),
// and splits the current keymap into the eight slots.
void KeySettingWindow::loadResources()
{
    background = loadSkinImage(g_themePath, "./", "skin/bg-keysetting.png", g_backBuffer, 0);
    highlight = loadSkinImage(g_themePath, "./", "skin/bg-keysetting-f.png", g_backBuffer, 0);
    menuIcon = loadSkinImage(g_themePath, "./", "skin/ic-MENU.png", g_backBuffer, 0);
    positions[0] = {0, 64, 160, 60};
    positions[1] = {0, 124, 160, 60};
    positions[2] = {482, 64, 160, 60};
    positions[3] = {482, 124, 160, 60};
    positions[4] = {482, 184, 160, 60};
    positions[5] = {482, 244, 160, 60};
    positions[6] = {482, 312, 160, 60};
    positions[7] = {482, 372, 160, 60};
    std::string mapping(getKeymap());
    std::istringstream stream(mapping);
    std::string part;
    int i = 0;
    while (std::getline(stream, part, ',') && i < 8) keys[i++] = part;
    // The keymap lists L2,L,R2,R first; the screen shows L,L2,R,R2.
    std::string temp;
    temp = keys[0];
    keys[0] = keys[1];
    keys[1] = temp;
    temp = keys[2];
    keys[2] = keys[3];
    keys[3] = temp;
}
