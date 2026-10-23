// tu_04cb68 (0x04cb68-0x051b5c): the factory hardware tests (Settings > Hardware Test).
//
// HardwareTestMenu lists six tests, each with a PASS/FAIL/UNTEST result: keys (press each button
// in turn within 12 s), LCD (color bars), sound (music at three volumes), USB host (type on a
// USB keyboard), SD card (write and read back 2 MiB, with speeds) and a stress test (run a
// bundled arcade ROM; the menu shows how long it has run). Results are saved together with the
// device serial and only shown again on the same device. KeyTestWindow2 ("IO Test") lights up
// each button on a picture of the device as it is pressed. Much of this comes from TrimUI's
// firmware (paths under /usr/trimui, "Usb Host Test").
// Body 19,208 bytes, 27 inline tail functions; four zero linker-padding bytes precede cJSON at
// 0x051b60.
#include <cstdio>
#include "SdlApi.h"
#include "CJsonApi.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include "HardwareTestWindows.h"
#include "LangStrings.h"
#include "Input.h"
#include "ShmVar.h"
#include "WindowIds.h"

// Button codes in the tests: the original numbering (keyEventName's "key_*" names), which is
// MainButton's except that START is 9 and SELECT 10, and L2/R2 are 12/13, labeled L and R.
// 14-17 only occur in the IO test.
enum TestKey {
    TEST_A = 1,
    TEST_B = 2,
    TEST_X = 3,
    TEST_Y = 4,
    TEST_UP = 5,
    TEST_DOWN = 6,
    TEST_LEFT = 7,
    TEST_RIGHT = 8,
    TEST_START = 9,
    TEST_SELECT = 10,
    TEST_MENU = 11,
    TEST_L = 12,
    TEST_R = 13,                       // L2 and R2 (SDL TAB and BACKSPACE)
    TEST_L1 = 14,
    TEST_R1 = 15,
    TEST_VOLUME_UP = 16,
    TEST_VOLUME_DOWN = 17,     // SDL LSUPER / RSUPER (mask-add/mask-minus)
};
// g_testResults slots and values.
enum {
    RESULT_KEYS,
    RESULT_LCD,
    RESULT_SOUND,
    RESULT_USB,
    RESULT_SDCARD
};
enum {
    TEST_UNTESTED = 0,
    TEST_PASS = 1,
    TEST_FAIL = -1
};
extern "C" {
SDL_Surface *TTF_RenderUTF8_Blended(void *, const char *, SDL_Color);
}

extern char *g_themePath;
extern SDL_Surface *g_backBuffer;
extern void *g_imeFont;
extern SDL_Color g_imeTextColor;
// 0x17e778
TestColor g_testGray = {
    0x404040};
TestColor g_testGreen = {
              0xff00},
          g_testRed = {0xff}, g_testBlue = {0xff0000}, g_testWhite = {0xffffff};
int g_testResults[5];   // per test (RESULT_*): TEST_PASS, TEST_FAIL or TEST_UNTESTED
TestColor g_testBlack = {0};
// 0x181b7c
void drawBackground(SDL_Surface *);
int drawText(SDL_Surface *, void *, SDL_Rect, const char *, SDL_Color, int, bool);
SDL_Surface *loadSkinImage(const char *, const char *, const char *, SDL_Surface *, bool);
int saveTestResults(int *);
// Waits for a button release and returns its TestKey; user events come back as their code
// (EVENT_REDRAW, EVENT_TEST_TIMEOUT). Other buttons fall through to the user-event case and
// return the event's (meaningless) code field, as in the original.
int readTestInput()
{
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case MIYOO_KEY_UP: return TEST_UP;
                    case MIYOO_KEY_DOWN: return TEST_DOWN;
                    case MIYOO_KEY_LEFT: return TEST_LEFT;
                    case MIYOO_KEY_RIGHT: return TEST_RIGHT;
                    case MIYOO_KEY_A: return TEST_A;
                    case MIYOO_KEY_B: return TEST_B;
                    case MIYOO_KEY_X: return TEST_X;
                    case MIYOO_KEY_Y: return TEST_Y;
                    case MIYOO_KEY_L2: return TEST_L;
                    case MIYOO_KEY_R2: return TEST_R;
                    case MIYOO_KEY_SELECT: return TEST_SELECT;
                    case MIYOO_KEY_MENU: return TEST_MENU;
                    case MIYOO_KEY_START: return TEST_START;
                }
            case SDL_USEREVENT: return event.user.code;
            default: break;
        }
    }
    return 0;
}

// A "press these keys in order" test: sequence holds the keys, input reads one, and the result
// goes to g_testResults[index]. Each key must come within 12 s (a 1 s timer counts down).
SDLKeyTestWindow::SDLKeyTestWindow(const int *sequence, int count, int (*input)(), const char *unused, int index) : TrimusWindow(-1, WIN_TEST)
{
    readInput = input;
    keys = sequence;
    keyCount = count;
    testIndex = index;
    current = 0;
    seconds = 12;
    timer = 0;
}

// Next key: the expected one advances (and restarts the countdown); the last one passes the
// test; EVENT_TEST_TIMEOUT fails it. Returns 1 to close the window.
int SDLKeyTestWindow::handleInput()
{
    int input = readInput();
    if (input > 0) {
        if (current < keyCount) {
            if (input == EVENT_TEST_TIMEOUT) {
                g_testResults[testIndex] = -1;
                saveTestResults(g_testResults);
                return 1;
            }
            if (input == keys[current]) {
                current++;
                seconds = 12;
                restartTimer();
            }
            if (current == keyCount) {
                g_testResults[testIndex] = 1;
                saveTestResults(g_testResults);
                return 1;
            } else
                return -1;
        }
        return 1;
    }
    return -1;
}

const char *SDLKeyTestWindow::keyName(int key)
{
    switch (key) {
        case 273: return "UP";
        case 274: return "DOWN";
        case 276: return "LEFT";
        case 275: return "RIGHT";
        case 'u': return "u";
        case 's': return "s";
        case 'b': return "b";
        default: return "";
    }
}

// Draws text aligned in bounds (align 0 left, 1 center, 2 right) with the bytes
// [begin, begin+length) in red, e.g. the key to press. Returns the width.
int drawHighlightedText(SDL_Surface *dst, void *font, SDL_Rect bounds, const char *text, SDL_Color color, int align, int begin, int length)
{
    if (!font) return 0;
    SDL_Rect dest;
    if (begin < 0 || begin >= strlen(text) || length < 0 || length > strlen(text)) {
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
        if (align == 1) {
            dest.x = bounds.x + (bounds.w - surface->w) / 2;
            dest.y = bounds.y + (bounds.h - surface->h) / 2;
        } else if (align == 0) {
            dest.x = bounds.x;
            dest.y = bounds.y;
        } else {
            dest.x = bounds.x + bounds.w - surface->w;
            dest.y = bounds.y;
        }
        dest.w = surface->w;
        dest.h = surface->h;
        SDL_UpperBlit(surface, NULL, dst, &dest);
        int width = surface->w;
        SDL_FreeSurface(surface);
        return width;
    } else {
        std::string value(text);
        std::string left = value.substr(0, begin), middle = value.substr(begin, length), right = value.substr(begin + length);
        SDL_Surface *a = TTF_RenderUTF8_Blended(font, left.c_str(), color);
        SDL_Surface *b = TTF_RenderUTF8_Blended(font, middle.c_str(), g_testRed.bytes);
        SDL_Surface *c = TTF_RenderUTF8_Blended(font, right.c_str(), color);
        int width = a->w + 2 + b->w + 2 + c->w;
        if (align == 1) {
            dest.x = bounds.x + (bounds.w - width) / 2;
            dest.y = bounds.y + (bounds.h - a->h) / 2;
        } else if (align == 0) {
            dest.x = bounds.x;
            dest.y = bounds.y;
        } else {
            dest.x = bounds.x + bounds.w - width;
            dest.y = bounds.y;
        }
        dest.w = a->w;
        dest.h = a->h;
        SDL_UpperBlit(a, NULL, dst, &dest);
        dest.y += 1;
        dest.x += a->w + 2;
        dest.w = b->w;
        dest.h = b->h;
        SDL_UpperBlit(b, NULL, dst, &dest);
        dest.y -= 1;
        dest.x += b->w + 2;
        dest.w = c->w;
        dest.h = c->h;
        SDL_UpperBlit(c, NULL, dst, &dest);
        SDL_FreeSurface(a);
        SDL_FreeSurface(b);
        SDL_FreeSurface(c);
        return width;
    }
}

int SDLKeyTestWindow::draw(SDL_Surface *dst)
{
    drawBackground(dst);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 80;
    rect.w = 640;
    rect.h = 40;
    drawText(dst, g_imeFont, rect, "Please plug in usb keyboard", g_imeTextColor, 1, false);
    rect.x = 0;
    rect.y = 120;
    rect.w = 640;
    rect.h = 40;
    snprintf(message, 64, "and press %s in %d seconds", keyName(keys[current]), seconds);
    drawHighlightedText(dst, g_imeFont, rect, message, g_imeTextColor, 1, 10, strlen(keyName(keys[current])));
    return 0;
}

// Once a second: count down; at zero post EVENT_TEST_TIMEOUT, otherwise EVENT_REDRAW.
unsigned testKeyTimer(unsigned interval, void *data)
{
    SDLKeyTestWindow *next = (SDLKeyTestWindow *)data;
    next->seconds--;
    if (next->seconds <= 0) {
        SDL_UserEvent event;
        event.type = SDL_USEREVENT;
        event.code = EVENT_TEST_TIMEOUT;
        if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    } else {
        SDL_UserEvent event;
        event.type = SDL_USEREVENT;
        event.code = EVENT_REDRAW;
        if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
        next->restartTimer();
    }
    return 0;
}

void SDLKeyTestWindow::restartTimer()
{
    if (timer) SDL_RemoveTimer(timer);
    timer = SDL_AddTimer(1000, reinterpret_cast<unsigned (*)(unsigned, void *)>(testKeyTimer), this);
}

void SDLKeyTestWindow::onHide()
{
    if (timer) {
        SDL_RemoveTimer(timer);
        timer = 0;
    }
}

void SDLKeyTestWindow::onShow()
{
    restartTimer();
}

// A key picture in its normal and highlighted look.
TestKeyImage::TestKeyImage(const char *normalFile, const char *focusedFile)
{
    normal = loadSkinImage(g_themePath, "./", normalFile, g_backBuffer, false);
    focused = loadSkinImage(g_themePath, "./", focusedFile, g_backBuffer, false);
}

TestKeyImage::~TestKeyImage()
{
    if (normal) SDL_FreeSurface(normal);
    if (focused) SDL_FreeSurface(focused);
}

// The device key test: SDLKeyTestWindow with a drawing of the buttons.
KeyTestWindow::KeyTestWindow(const int *sequence, int count, int (*input)(), const char *unused, int index) : SDLKeyTestWindow(sequence, count, input, unused, index),
                                                                                                              left("skin/left.png", "skin/left_focus.png"), right("skin/right.png", "skin/right_focus.png"), up("skin/up.png", "skin/up_focus.png"), down("skin/down.png", "skin/down_focus.png"), circle("skin/circle.png", "skin/circle_focus.png"), rectangle("skin/rect.png", "skin/rect_focus.png")
{
}

const char *KeyTestWindow::keyName(int key)
{
    switch (key) {
        case TEST_UP: return "UP";
        case TEST_DOWN: return "DOWN";
        case TEST_LEFT: return "LEFT";
        case TEST_RIGHT: return "RIGHT";
        case TEST_A: return "A";
        case TEST_B: return "B";
        case TEST_X: return "X";
        case TEST_Y: return "Y";
        case TEST_L: return "L";
        case TEST_R: return "R";
        case TEST_SELECT: return "SELECT";
        case TEST_MENU: return "MENU";
        case TEST_START: return "START";
        default: return "";
    }
}

bool KeyTestWindow::expecting(int key)
{
    return key == keys[current];
}

int KeyTestWindow::draw(SDL_Surface *dst)
{
    drawBackground(dst);
    SDL_Rect rect;
    rect.x = 65;
    rect.y = 10;
    SDL_UpperBlit(expecting(TEST_L) ? rectangle.focused : rectangle.normal, NULL, dst, &rect);
    rect.x = 225;
    rect.y = 10;
    SDL_UpperBlit(expecting(TEST_R) ? rectangle.focused : rectangle.normal, NULL, dst, &rect);
    rect.x = 65;
    rect.y = 45;
    SDL_UpperBlit(expecting(TEST_UP) ? up.focused : up.normal, NULL, dst, &rect);
    rect.x = 65;
    rect.y = 105;
    SDL_UpperBlit(expecting(TEST_DOWN) ? down.focused : down.normal, NULL, dst, &rect);
    rect.x = 40;
    rect.y = 75;
    SDL_UpperBlit(expecting(TEST_LEFT) ? left.focused : left.normal, NULL, dst, &rect);
    rect.x = 90;
    rect.y = 75;
    SDL_UpperBlit(expecting(TEST_RIGHT) ? right.focused : right.normal, NULL, dst, &rect);
    rect.x = 250;
    rect.y = 75;
    SDL_UpperBlit(expecting(TEST_A) ? circle.focused : circle.normal, NULL, dst, &rect);
    rect.x = 225;
    rect.y = 105;
    SDL_UpperBlit(expecting(TEST_B) ? circle.focused : circle.normal, NULL, dst, &rect);
    rect.x = 225;
    rect.y = 45;
    SDL_UpperBlit(expecting(TEST_X) ? circle.focused : circle.normal, NULL, dst, &rect);
    rect.x = 200;
    rect.y = 75;
    SDL_UpperBlit(expecting(TEST_Y) ? circle.focused : circle.normal, NULL, dst, &rect);
    rect.x = 105;
    rect.y = 130;
    SDL_UpperBlit(expecting(TEST_SELECT) ? rectangle.focused : rectangle.normal, NULL, dst, &rect);
    rect.x = 145;
    rect.y = 130;
    SDL_UpperBlit(expecting(TEST_MENU) ? rectangle.focused : rectangle.normal, NULL, dst, &rect);
    rect.x = 185;
    rect.y = 130;
    SDL_UpperBlit(expecting(TEST_START) ? rectangle.focused : rectangle.normal, NULL, dst, &rect);
    snprintf(message, 64, "Please press %s in %d seconds", keyName(keys[current]), seconds);
    rect.x = 0;
    rect.y = 160;
    rect.w = 640;
    rect.h = 40;
    drawHighlightedText(dst, g_imeFont, rect, message, g_imeTextColor, 1, 13, strlen(keyName(keys[current])));
    return 0;
}

// The "IO Test": shows which buttons are held. MENU + A leaves.
KeyTestWindow2::KeyTestWindow2() : TrimusWindow(STR_IO_TEST, WIN_TEST)
{
    loadMasks();
    menuHeld = false;
}

KeyTestWindow2::~KeyTestWindow2()
{
    if (maskCross) SDL_FreeSurface(maskCross);
    if (maskStart) SDL_FreeSurface(maskStart);
    if (maskMenu) SDL_FreeSurface(maskMenu);
    if (maskLR) SDL_FreeSurface(maskLR);
    if (maskABXY) SDL_FreeSurface(maskABXY);
    if (maskAdd) SDL_FreeSurface(maskAdd);
    if (maskMinus) SDL_FreeSurface(maskMinus);
    if (background) SDL_FreeSurface(background);
}

void KeyTestWindow2::loadMasks()
{
    maskCross = loadSkinImage(g_themePath, "./", "skin/mask-cross-key.png", g_backBuffer, false);
    maskStart = loadSkinImage(g_themePath, "./", "skin/mask-insert&start.png", g_backBuffer, false);
    maskMenu = loadSkinImage(g_themePath, "./", "skin/mask-menu.png", g_backBuffer, false);
    maskLR = loadSkinImage(g_themePath, "./", "skin/mask-LR.png", g_backBuffer, false);
    maskABXY = loadSkinImage(g_themePath, "./", "skin/mask-ABXY.png", g_backBuffer, false);
    maskAdd = loadSkinImage(g_themePath, "./", "skin/mask-add.png", g_backBuffer, false);
    maskMinus = loadSkinImage(g_themePath, "./", "skin/mask-minus.png", g_backBuffer, false);
    background = loadSkinImage(g_themePath, "./", "skin/bg-io-testing.png", g_backBuffer, false);
}

int KeyTestWindow2::mapKey(SDLKey key)
{
    switch (key) {
        case MIYOO_KEY_UP: return TEST_UP;
        case MIYOO_KEY_DOWN: return TEST_DOWN;
        case MIYOO_KEY_LEFT: return TEST_LEFT;
        case MIYOO_KEY_RIGHT: return TEST_RIGHT;
        case MIYOO_KEY_A: return TEST_A;
        case MIYOO_KEY_B: return TEST_B;
        case MIYOO_KEY_X: return TEST_X;
        case MIYOO_KEY_Y: return TEST_Y;
        case MIYOO_KEY_L2: return TEST_L;
        case MIYOO_KEY_R2: return TEST_R;
        case MIYOO_KEY_SELECT: return TEST_SELECT;
        case MIYOO_KEY_MENU: return TEST_MENU;
        case MIYOO_KEY_START: return TEST_START;
        case MIYOO_KEY_L1: return TEST_L1;
        case MIYOO_KEY_R1: return TEST_R1;
        case MIYOO_KEY_LSUPER: return TEST_VOLUME_UP;
        case MIYOO_KEY_RSUPER: return TEST_VOLUME_DOWN;
    }
    return -1;
}

// Records presses and releases for draw; A released while MENU is held closes the window.
int KeyTestWindow2::handleInput()
{
    int key = -1;
    SDL_Event event;
    if (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case 2:
                key = mapKey((SDLKey)event.key.keysym.sym);
                if (key > 0 && key < 18) {
                    pressed[key] = 1;
                    if (key == 11) menuHeld = true;
                    return -1;
                }
            case 3:
                if (event.key.keysym.sym == 32 && menuHeld) return 1;
                key = mapKey((SDLKey)event.key.keysym.sym);
                if (key > 0 && key < 18) {
                    pressed[key] = 0;
                    released[key] = 1;
                    menuHeld = false;
                }
                return -1;
            default: return -1;
        }
    }
    return -1;
}

// The highlight image and its position on the device picture, per key.
SDL_Surface *keyMask(KeyTestWindow2 *window, int key)
{
    switch (key) {
        case TEST_UP:
        case TEST_DOWN:
        case TEST_LEFT:
        case TEST_RIGHT: return window->maskCross;
        case TEST_A:
        case TEST_B:
        case TEST_X:
        case TEST_Y: return window->maskABXY;
        case TEST_L:
        case TEST_R:
        case TEST_L1:
        case TEST_R1: return window->maskLR;
        case TEST_START:
        case TEST_SELECT: return window->maskStart;
        case TEST_MENU: return window->maskMenu;
        case TEST_VOLUME_UP: return window->maskAdd;
        case TEST_VOLUME_DOWN: return window->maskMinus;
        default: return NULL;
    }
}

int keyMaskRect(KeyTestWindow2 *window, int key, SDL_Rect *rect)
{
    switch (key) {
        case TEST_UP:
            rect->x = 136;
            rect->y = 278;
            rect->w = 28;
            rect->h = 28;
            return 0;
        case TEST_DOWN:
            rect->x = 136;
            rect->y = 328;
            rect->w = 28;
            rect->h = 28;
            return 0;
        case TEST_LEFT:
            rect->x = 104;
            rect->y = 302;
            rect->w = 28;
            rect->h = 28;
            return 0;
        case TEST_RIGHT:
            rect->x = 163;
            rect->y = 302;
            rect->w = 28;
            rect->h = 28;
            return 0;
        case TEST_A:
            rect->x = 286;
            rect->y = 301;
            rect->w = 31;
            rect->h = 31;
            return 0;
        case TEST_B:
            rect->x = 256;
            rect->y = 327;
            rect->w = 31;
            rect->h = 31;
            return 0;
        case TEST_X:
            rect->x = 256;
            rect->y = 272;
            rect->w = 31;
            rect->h = 31;
            return 0;
        case TEST_Y:
            rect->x = 230;
            rect->y = 301;
            rect->w = 31;
            rect->h = 31;
            return 0;
        case TEST_L:
            rect->x = 517;
            rect->y = 161;
            rect->w = 38;
            rect->h = 41;
            return 0;
        case TEST_R:
            rect->x = 414;
            rect->y = 161;
            rect->w = 38;
            rect->h = 41;
            return 0;
        case TEST_L1:
            rect->x = 558;
            rect->y = 161;
            rect->w = 38;
            rect->h = 41;
            return 0;
        case TEST_R1:
            rect->x = 375;
            rect->y = 161;
            rect->w = 38;
            rect->h = 41;
            return 0;
        case TEST_SELECT:
            rect->x = 170;
            rect->y = 365;
            rect->w = 28;
            rect->h = 24;
            return 0;
        case TEST_START:
            rect->x = 203;
            rect->y = 365;
            rect->w = 28;
            rect->h = 24;
            return 0;
        case TEST_MENU:
            rect->x = 199;
            rect->y = 269;
            rect->w = 28;
            rect->h = 24;
            return 0;
        case TEST_VOLUME_UP:
            rect->x = 39;
            rect->y = 103;
            rect->w = 22;
            rect->h = 40;
            return 0;
        case TEST_VOLUME_DOWN:
            rect->x = 39;
            rect->y = 143;
            rect->w = 22;
            rect->h = 40;
            return 0;
        default: return -1;
    }
}

extern SDL_Surface *g_keySettingFooter;
const char *languageString(int);
void drawTitleBar(SDL_Surface *, const char *, bool);
int drawIconLabel(SDL_Surface *, SDL_Surface *, int, int, const char *, const char *);
void drawKeyTestFooter(SDL_Surface *dst)
{
    SDL_Surface *icon = NULL;
    int y = 420, x = 20;
    if (g_keySettingFooter) {
        SDL_Rect dest, src;
        dest.x = 0;
        dest.y = y;
        src.x = 0;
        src.y = 0;
        src.w = g_keySettingFooter->w;
        src.h = g_keySettingFooter->h;
        SDL_UpperBlit(g_keySettingFooter, &src, dst, &dest);
    }
    icon = loadSkinImage(g_themePath, "./", "skin/ic-MENU+A.png", g_backBuffer, false);
    if (icon) {
        drawIconLabel(dst, icon, x, y, 0, languageString(STR_EXIT));
        SDL_FreeSurface(icon);
    }
}

int KeyTestWindow2::draw(SDL_Surface *dst)
{
    if (background)
        SDL_UpperBlit(background, NULL, dst, NULL);
    else
        drawBackground(dst);
    drawTitleBar(dst, getTitle(), 1);
    drawKeyTestFooter(dst);
    SDL_Surface *mask = NULL;
    int result = -1;
    SDL_Rect rect;
    for (int i = 1; i < 18; i++) {
        if (pressed[i] == 1) {
            mask = keyMask(this, i);
            result = keyMaskRect(this, i, &rect);
            if (mask && result >= 0) SDL_UpperBlit(mask, NULL, dst, &rect);
        }
    }
    for (int i = 1; i < 18; i++) {
        if (released[i] == 1) {
            mask = keyMask(this, i);
            result = keyMaskRect(this, i, &rect);
            if (mask && result >= 0) SDL_UpperBlit(mask, &rect, dst, &rect);
            released[i] = 0;
        }
    }
    return 0;
}

// The LCD test: five vertical color bars (up/down switches to plain red, left/right rotates
// the colors); A passes, B fails.
LCDTestWindow::LCDTestWindow() : TrimusWindow(-1, WIN_TEST)
{
    mode = 0;
    colorIndex = 0;
    colors[0] = g_testRed;
    colors[1] = g_testGreen;
    colors[2] = g_testBlue;
    colors[3] = g_testWhite;
    colors[4] = g_testBlack;
}

int LCDTestWindow::handleInput()
{
    int key = readTestInput();
    switch (key) {
        case 5:
        case 6:
            mode++;
            break;
        case 7:
            if (colorIndex < 5)
                colorIndex++;
            else
                colorIndex = 0;
            break;
        case 8:
            if (colorIndex > 0)
                colorIndex--;
            else
                colorIndex = 4;
            break;
        case 1:
            g_testResults[1] = 1;
            saveTestResults(g_testResults);
            return 1;
        case 2:
            g_testResults[1] = -1;
            saveTestResults(g_testResults);
            return 1;
    }
    return -1;
}


int LCDTestWindow::draw(SDL_Surface *dst)
{
    SDL_Rect rect;
    SDL_Color value;
    if (mode % 2 == 0) {
        int color = colorIndex;
        rect.x = 0;
        rect.y = 0;
        rect.w = 128;
        rect.h = 480;
        for (unsigned i = 0; (int)i < 5;) {
            rect.x = i * 128;
            value = colors[color % 5].bytes;
            SDL_FillRect(dst, &rect, SDL_MapRGB(dst->format, value.r, value.g, value.b));
            i++;
            color++;
        }
    } else
        SDL_FillRect(dst, NULL, SDL_MapRGB(dst->format, 255, 0, 0));
    return 0;
}

void LCDTestWindow::onHide()
{
}

void LCDTestWindow::onShow()
{
}

// The sound test: plays the theme music at 1/3, 2/3 and full volume, six seconds each; A passes
// once all three have played, B fails.
SoundTestWindow::SoundTestWindow() : TrimusWindow(-1, WIN_TEST)
{
    phase = 0;
    timer = 0;
}

int SoundTestWindow::handleInput()
{
    int key = readTestInput();
    switch (key) {
        case 2:
            g_testResults[2] = -1;
            saveTestResults(g_testResults);
            return 1;
        case 1:
            if (phase > 2) {
                g_testResults[2] = 1;
                saveTestResults(g_testResults);
                return 1;
            }
            break;
        default: break;
    }
    return -1;
}

int SoundTestWindow::draw(SDL_Surface *dst)
{
    drawBackground(dst);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 40;
    if (phase % 3 == 0) {
        drawText(dst, g_imeFont, rect, "Play Music in 1/3 of max volume", g_imeTextColor, 1, false);
    } else if (phase % 3 == 1) {
        drawText(dst, g_imeFont, rect, "Play Music in 2/3 of max volume", g_imeTextColor, 1, false);
    } else {
        drawText(dst, g_imeFont, rect, "Play Music in max volume", g_imeTextColor, 1, false);
    }
    rect.y = 100;
    drawHighlightedText(dst, g_imeFont, rect, "Press B when fail", g_imeTextColor, 1, 6, 1);
    if (phase > 2) {
        rect.y = 140;
        drawHighlightedText(dst, g_imeFont, rect, "Press A when pass", g_imeTextColor, 1, 6, 1);
    }
    return 0;
}

unsigned soundTestTimer(unsigned interval, void *data)
{
    SoundTestWindow *window = (SoundTestWindow *)data;
    window->playNext();
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_REDRAW;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return 0;
}


extern "C" int GetKeyShm(void *, int);
extern char g_keyShm[];
extern void *g_themeMusic;
void SoundTestWindow::playNext()
{
    phase++;
    Mix_VolumeMusic(((phase % 3 + 1) * 128) / 3);
    printf("play music vollume %d\n", ((phase % 3 + 1) * 128) / 3);
    if (timer) SDL_RemoveTimer(timer);
    timer = SDL_AddTimer(6000, soundTestTimer, this);
}

void SoundTestWindow::onHide()
{
    Mix_PlayMusic(g_themeMusic, -1);
    Mix_VolumeMusic((GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME) << 7) / 20);
    printf("restore volume %d\n", (GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME) << 7) / 20);
    if (timer) SDL_RemoveTimer(timer);
}

void SoundTestWindow::onShow()
{
    Mix_PauseMusic();
    Mix_PlayMusic(g_themeMusic, -1);
    Mix_VolumeMusic(((phase % 3 + 1) * 128) / 3);
    printf("play music vollume %d\n", ((phase % 3 + 1) * 128) / 3);
    if (timer) SDL_RemoveTimer(timer);
    timer = SDL_AddTimer(6000, soundTestTimer, this);
}

// The USB keyboard test's input: the raw SDL key of each release (or a user event's code).
int readUsbTestInput()
{
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case 3: return event.key.keysym.sym;
            case 24: return event.user.code;
            default: break;
        }
    }
    return 0;
}

// The SD card test: a thread writes 2 MiB to /mnt/SDCARD/testfile and reads it back, checking
// every byte; the window shows progress and speeds. Any key afterwards records the result.
SDCardTestWindow::SDCardTestWindow() : TrimusWindow(-1, WIN_TEST)
{
    thread = NULL;
    writeMS = 0;
    readMS = 0;
    readBytes = 0;
    writeBytes = 0;
    readDone = false;
    writeDone = false;
    lastUpdate = 0;
}

int SDCardTestWindow::handleInput()
{
    int key = readTestInput();
    if (readDone && writeDone) {
        if (readBytes == 0x200000 && writeBytes == 0x200000) {
            g_testResults[4] = 1;
            saveTestResults(g_testResults);
            return 1;
        } else {
            g_testResults[4] = -1;
            saveTestResults(g_testResults);
            return 1;
        }
    }
    return -1;
}

int SDCardTestWindow::draw(SDL_Surface *dst)
{
    drawBackground(dst);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 80;
    rect.w = 640;
    rect.h = 40;
    if (writeMS <= 0)
        snprintf(message, 64, "writed %d bytes", writeBytes);
    else {
        printf("mWriteMS is %d, speed %.2f\n", writeMS, (float)(writeBytes * 1000) / (writeMS << 20));
        snprintf(message, 64, "write speed %.2f MB/s", (float)(writeBytes * 1000) / (writeMS << 20));
    }
    drawText(dst, g_imeFont, rect, message, g_imeTextColor, 1, false);
    rect.x = 0;
    rect.y = 120;
    rect.w = 640;
    rect.h = 40;
    if (readMS <= 0)
        snprintf(message, 64, "readed %d bytes", readBytes);
    else {
        printf("mReadMS is %d, speed %.2f\n", readMS, (float)(readBytes * 1000) / (readMS << 20));
        snprintf(message, 64, "read speed %.2f MB/s", (float)(readBytes * 1000) / (readMS << 20));
    }
    drawText(dst, g_imeFont, rect, message, g_imeTextColor, 1, false);
    return 0;
}
#include <sys/time.h>
void SDCardTestWindow::testWrite()
{
    int i;
    FILE *f = fopen("/mnt/SDCARD/testfile", "w");
    void *buf;
    int count;
    int elapsed;
    timeval start, end, now;
    SDL_UserEvent event;
    gettimeofday(&start, NULL);
    writeBytes = 0;
    event.type = SDL_USEREVENT;
    event.code = EVENT_REDRAW;
    lastUpdate = 0;
    if (f) {
        buf = malloc(1024);
        if (buf) {
            memset(buf, 'b', 1024);
            for (i = 0; i < 2048; i++) {
                count = fwrite(buf, 1, 1024, f);
                if (count != 1024) break;
                writeBytes += count;
                // Preserve the original use of end before its final gettimeofday call.
                gettimeofday(&now, NULL);
                elapsed = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
                if (elapsed - lastUpdate > 100) {
                    SDL_PushEvent(&event);
                    lastUpdate = elapsed;
                }
            }
            free(buf);
        }
        fflush(f);
        fclose(f);
    }
    gettimeofday(&end, NULL);
    writeMS = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
    writeDone = true;
    SDL_PushEvent(&event);
    printf("write %d bytes take %d ms\n", writeBytes, writeMS);
}

void SDCardTestWindow::testRead()
{
    bool bad;
    int i, j;
    FILE *f = fopen("/mnt/SDCARD/testfile", "r");
    char *buf;
    int count;
    int elapsed;
    timeval start, end, now;
    SDL_UserEvent event;
    gettimeofday(&start, NULL);
    readBytes = 0;
    event.type = SDL_USEREVENT;
    event.code = EVENT_REDRAW;
    lastUpdate = 0;
    if (f) {
        buf = (char *)malloc(1024);
        if (buf) {
            for (i = 0; i < 2048; i++) {
                count = fread(buf, 1, 1024, f);
                bad = true;
                if (count == 1024) {
                    j = 0;
                    for (j = 0; j < count; j++) {
                        if (buf[j] != 'b') break;
                    }
                    bad = j < count;
                }
                readBytes += count;
                // Preserve the original use of end before its final gettimeofday call.
                gettimeofday(&now, NULL);
                elapsed = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
                if (elapsed - lastUpdate > 100) {
                    SDL_PushEvent(&event);
                    lastUpdate = elapsed;
                }
                if (bad) break;
            }
            free(buf);
        }
        fflush(f);
        fclose(f);
    }
    gettimeofday(&end, NULL);
    readMS = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
    readDone = true;
    SDL_PushEvent(&event);
    printf("read %d bytes take %d ms\n", readBytes, readMS);
}


void SDCardTestWindow::onHide()
{
    if (thread) SDL_WaitThread(thread, NULL);
}

int sdCardTestThread(void *data)
{
    SDCardTestWindow *window = (SDCardTestWindow *)data;
    puts("test_thread");
    window->testWrite();
    window->testRead();
    return 0;
}

void SDCardTestWindow::onShow()
{
    thread = SDL_CreateThread(sdCardTestThread, this);
}

TestMenuItem::TestMenuItem(const char *text, const char *path)
{
    if (path)
        icon = loadSkinImage(g_themePath, "./", path, g_backBuffer, false);
    else
        icon = NULL;
    label = text;
}

TestMenuItem::~TestMenuItem()
{
    if (icon) SDL_FreeSurface(icon);
}

// Reads how long the stress test has been running (its start time is in /tmp/.stress_start_time).
HardwareTestMenu::HardwareTestMenu() : TextMenu(0, 1, 6, NULL)
{
    initializeTests();
    stressSeconds = 0;
    puts("HardwareTestMenu");
    FILE *f = fopen("/tmp/.stress_start_time", "rb");
    if (f) {
        int ms;
        size_t count = fread(&ms, 4, 1, f);
        if (count == 1) {
            timeval tv;
            gettimeofday(&tv, NULL);
            stressSeconds = tv.tv_sec - ms;
            if (stressSeconds < 0) stressSeconds = 0;
            printf("tv_sec=%ld, ms=%d, mStressRunSeconds=%d\n", tv.tv_sec, ms, stressSeconds);
        } else
            printf("readed %d bytes data %d\n", count, ms);
        fclose(f);
    } else
        puts("open stress fail");
}

HardwareTestMenu::~HardwareTestMenu()
{
    freeListItem();
}

extern std::vector<TrimusWindow *> *g_windowStack;
extern SDL_Surface *g_screen;
extern void *g_titleFont;
extern SDL_Color g_wifiEmptyColor;
extern char g_launchCmd[2048];
void showLaunchScreen(const char *);
// 0x18018

bool isSpinand();
bool sdCardMounted();
int g_usbTestKeys[7] = {   // the USB keyboard test: SDL keys up, down, left, right, u, s, b
                        273, 274, 276, 275, 117, 115, 98};
int g_deviceTestKeys[13] = {   // the key test's order
                            TEST_UP, TEST_DOWN, TEST_LEFT, TEST_RIGHT, TEST_A, TEST_B, TEST_X, TEST_Y, TEST_L, TEST_R, TEST_SELECT, TEST_MENU, TEST_START};
// Opens the selected test. The SD card test needs a card when the system runs from SPI NAND;
// the stress test launches /mnt/SDCARD/test/dino.zip in mame4allx (returning 3 to launch).
int HardwareTestMenu::onSelect()
{
    switch (cur) {
        case 0: {
            KeyTestWindow *window = new KeyTestWindow(g_deviceTestKeys, 13, readTestInput, "KeyTest", 0);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            break;
        }
        case 1: {
            LCDTestWindow *window = new LCDTestWindow;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            break;
        }
        case 2: {
            SoundTestWindow *window = new SoundTestWindow;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            break;
        }
        case 3: {
            SDLKeyTestWindow *window = new SDLKeyTestWindow(g_usbTestKeys, 7, readUsbTestInput, "USBKeyTest", 3);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            break;
        }
        case 4: {
            if (!(isSpinand() == true && sdCardMounted() == false)) {
                SDCardTestWindow *window = new SDCardTestWindow;
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            }
            break;
        }
        case 5: {
            if (access("/mnt/SDCARD/test/dino.zip", 0) == 0) {
                SDL_Rect rect;
                rect.x = 0;
                rect.y = 0;
                rect.w = 640;
                rect.h = 480;
                drawBackground(g_backBuffer);
                drawText(g_backBuffer, g_titleFont, rect, "LOADING", g_wifiEmptyColor, 1, false);
                SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
                SDL_Flip(g_screen);
                snprintf(g_launchCmd, 2048, "cd /usr/trimui/bin/;HOME=/mnt/SDCARD/test/  /usr/trimui/bin/mame4allx /mnt/SDCARD/test/dino.zip");
                printf("Load test rom %s\n", g_launchCmd);
                showLaunchScreen(g_launchCmd);
                FILE *f = fopen("/tmp/.stress_start_time", "wb");
                if (f) {
                    timeval tv;
                    gettimeofday(&tv, NULL);
                    time_t seconds = tv.tv_sec;
                    fwrite(&seconds, 4, 1, f);
                    fclose(f);
                }
                return 3;
            }
            break;
        }
        default: break;
    }
    return -1;
}

const char *testResultLabel(int result)
{
    switch (result) {
        case 1: return "PASS";
        case -1: return "FAIL";
        default: return "UNTEST";
    }
}

SDL_Color testResultColor(int result)
{
    switch (result) {
        case 1: return g_testGreen.bytes;
        case -1: return g_testRed.bytes;
        default: return g_testGray.bytes;
    }
}

extern SDL_Color g_wifiSelectedColor;

// Draws the six rows: name and result (or the stress test's running time).
void HardwareTestMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0};
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        TestMenuItem *item = (TestMenuItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->icon) {
            area.y = (60 - item->icon->h) / 2 + y;
            SDL_UpperBlit(item->icon, NULL, dst, &area);
            area.x = area.x + item->icon->w + 5;
        }
        area.y = (60 - height) / 2 + y;
        area.w = rect.w - 100;
        area.h = 60;
        if (i == 4 && isSpinand() && sdCardMounted() != true) {
            printf("systemOnSPINand %d, isSDCardMounted %d\n", isSpinand(), sdCardMounted());
            color = g_testGray.bytes;
        }
        drawText(dst, g_imeFont, area, item->label.c_str(), color, 0, i == cur);
        area.x = rect.w + x - 80;
        area.w = 60;
        if (i != 5) {
            int result = g_testResults[i];
            color = testResultColor(result);
            drawText(dst, g_imeFont, area, testResultLabel(result), color, 1, false);
        } else {
            snprintf(text, 32, "run %ds", stressSeconds);
            drawText(dst, g_imeFont, area, text, color, 1, false);
        }
        i++;
        row++;
    }
}

void HardwareTestMenu::freeListItem()
{
    puts("HardwareTestMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        TestMenuItem *item = (TestMenuItem *)items[i];
        delete item;
    }
    items.clear();
}

bool loadTestResults(int *, char *);
int getSerial(char *);
// Loads the saved results, discarding them if they were saved on another device (serial).
void HardwareTestMenu::initializeTests()
{
    char expected[16], serial[16];
    memset(g_testResults, 0, 20);
    loadTestResults(g_testResults, serial);
    getSerial(expected);
    if (strncmp(expected, serial, 16) != 0) memset(g_testResults, 0, 20);
    addItem(new TestMenuItem("Key Test", NULL));
    addItem(new TestMenuItem("LCD Test", NULL));
    addItem(new TestMenuItem("Sound Test", NULL));
    addItem(new TestMenuItem("Usb Host Test", NULL));
    addItem(new TestMenuItem("SD Card Test", NULL));
    addItem(new TestMenuItem("Stress Test", NULL));
}
