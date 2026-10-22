// Key mapping window and its legacy touch-input worker, in binary address order.
// Proven 0x1376fc-0x138c50: 5,288 body bytes plus five inline virtuals.
#include <iostream>
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

struct SDL_Thread;
struct KeyEvent { unsigned char type, which, state, padding; unsigned char scancode; int sym, mod; unsigned short unicode; };
struct UserEvent { unsigned char type; int code; void *data1, *data2; };
union InputEvent { unsigned char type; KeyEvent key; UserEvent user; char storage[20]; };
extern "C" {
int SDL_UpperBlit(SDL_Surface *, SDL_Rect *, SDL_Surface *, SDL_Rect *);
void SDL_FreeSurface(SDL_Surface *);
int SDL_WaitEvent(void *); int SDL_PushEvent(void *);
SDL_Thread *SDL_CreateThread(int (*)(void *), void *);
void SDL_WaitThread(SDL_Thread *, int *);
const char *getKeymap(); void setKeymap(const char *);
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

SDL_Thread *g_touchThread;
bool g_touchRunning, g_touchPressed;
int g_touchX = -1, g_touchY = -1;
static_assert(sizeof(KeySettingWindow) == 0x15c, "KeySettingWindow layout");
static_assert(sizeof(InputEvent) == 20, "SDL event layout");

extern const char g_keyEventReadFormat[44];

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
    name[79] = 0; physical[79] = 0; unique[79] = 0;
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
int touchInputThread(void *)
{
    int fd = openInputDevice("/dev/input/event0");
    if (fd >= 0) {
        int count = 1;
        pollfd *fds = (pollfd *)calloc(1, sizeof(pollfd));
        int result;
        InputEvent event;
        input_event input;
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        while (g_touchRunning) {
            result = poll(fds, count, 500);
            if (fds[0].revents & POLLIN) {
                result = read(fds[0].fd, &input, sizeof(input));
                if (result < (int)sizeof(input)) {
                    fprintf(stderr, "could not get event\n");
                    break;
                }
                if (input.type == EV_KEY && input.code == BTN_TOUCH) {
                    printf(g_keyEventReadFormat, input.type, input.code, input.value);
                    if (input.value == 1) g_touchPressed = true;
                    else if (input.value == 0) g_touchPressed = false;
                    g_touchX = g_touchY = -1;
                }
                if (input.type == EV_ABS && (input.code == 0 || input.code == 1) && g_touchPressed) {
                    printf(g_keyEventReadFormat, input.type, input.code, input.value);
                    if (input.code == 0) g_touchX = input.value;
                    else if (input.code == 1) g_touchY = input.value;
                    if (g_touchX >= 0 && g_touchY >= 0) {
                        if (g_touchX < 100 && g_touchY > 100 && g_touchY < 380) {
                            event.type = 24; event.user.code = 7; SDL_PushEvent(&event);
                        }
                        if (g_touchX > 540 && g_touchY > 100 && g_touchY < 380) {
                            event.type = 24; event.user.code = 8; SDL_PushEvent(&event);
                        }
                        if (g_touchY < 100 && g_touchX > 100 && g_touchX < 540) {
                            event.type = 24; event.user.code = 5; SDL_PushEvent(&event);
                        }
                        if (g_touchY > 380 && g_touchX > 100 && g_touchX < 540) {
                            event.type = 24; event.user.code = 6; SDL_PushEvent(&event);
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
int waitMappingKey()
{
    InputEvent event;
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        case 3:
            switch (event.key.sym) {
            case 32: return 1;
            case 306: return 2;
            case 304: return 3;
            case 308: return 4;
            case 101: return 12;
            case 116: return 14;
            case 9: return 13;
            case 8: return 15;
            case 27: return 11;
            default: break;
            }
            // Falls through to the shared default exit of the outer switch.
        default: break;
        }
    }
    return 0;
}
// Original diagnostic allocation at 0x162578 retains 44 bytes.
extern const char g_keyEventReadFormat[44] = "read event type=%d, code=%d, value=%d\n";

KeySettingWindow::KeySettingWindow() : TrimusWindow(0x6e, 0x19)
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
int KeySettingWindow::handleInput()
{
    int key = waitMappingKey();
    switch (key) {
    case 11:
        snprintf(keymap, 64, "%s,%s,%s,%s,%s,%s,%s,%s", keys[1].c_str(), keys[0].c_str(),
                 keys[3].c_str(), keys[2].c_str(), keys[4].c_str(), keys[5].c_str(), keys[6].c_str(), keys[7].c_str());
        setKeymap(keymap);
        saveSystemJson(g_keyShm, g_themePath, g_fontSize);
        sync();
        return 1;
    case 1: keys[selected] = "A"; break;
    case 2: keys[selected] = "B"; break;
    case 3: keys[selected] = "X"; break;
    case 4: keys[selected] = "Y"; break;
    case 12: keys[selected] = "L"; break;
    case 13: keys[selected] = "L2"; break;
    case 14: keys[selected] = "R"; break;
    case 15: keys[selected] = "R2"; break;
    }
    printf("set key[%d] to %s\n", selected, keys[selected].c_str());
    selected++;
    if (selected > 7) selected = 0;
    return -1;
}
void drawKeySettingFooter(SDL_Surface *dst, SDL_Surface *icon)
{
    int y = 420;
    int x = 20;
    if (g_keySettingFooter) {
        SDL_Rect dest; dest.x = 0; dest.y = y;
        SDL_Rect source; source.x = 0; source.y = 0;
        source.w = g_keySettingFooter->w; source.h = g_keySettingFooter->h;
        SDL_UpperBlit(g_keySettingFooter, &source, dst, &dest);
    }
    if (icon) drawIconLabel(dst, icon, x, y, 0, languageString(0x70));
}
int KeySettingWindow::draw(SDL_Surface *dst)
{
    if (background) SDL_UpperBlit(background, NULL, dst, NULL);
    else drawBackground(dst);
    drawTitleBar(dst, getTitle(), 1);
    drawKeySettingFooter(dst, menuIcon);
    SDL_UpperBlit(highlight, NULL, dst, &positions[selected]);
    for (int i = 0; i < 8; i++)
        drawText(dst, g_imeFont, positions[i], keys[i].c_str(), g_imeTextColor, 1, i == selected);
    return 0;
}
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
    std::string temp;
    temp = keys[0]; keys[0] = keys[1]; keys[1] = temp;
    temp = keys[2]; keys[2] = keys[3]; keys[3] = temp;
}
