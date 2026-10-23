// tu_13edfc (0x13edfc-0x143720): ImeWindow, the on-screen keyboard.
//
// Four rows of ten character keys (digits, then QWERTY) and a bottom row of five wide keys:
// shift, symbols, space, delete and OK. The D-pad moves between keys and A presses one; the
// wide keys also have shortcut buttons (L2 shift, R2 symbols, X space, Y delete, START OK), and
// B cancels. OK passes the text to the window's Action (connect to Wi-Fi, save a hotspot
// setting, ...). Body 12,936 bytes, then 67 generated tail functions. Case order and true/false
// branch order below preserve GCC 8 -O0 layout.
#include <stdio.h>
#include <string.h>
#include "ImeWindow.h"
#include "Input.h"
#include "WindowIds.h"

extern "C" {
void SDL_FreeSurface(SDL_Surface *);
int SDL_UpperBlit(SDL_Surface *, SDL_Rect *, SDL_Surface *, SDL_Rect *);
unsigned int SDL_MapRGB(SDL_PixelFormat *, unsigned char, unsigned char, unsigned char);
int SDL_FillRect(SDL_Surface *, SDL_Rect *, unsigned int);
}
extern char *g_themePath;
// Key ids by column (10) and row (5): rows 0-3 are the character rows, row 4 the wide key below
// each column. Used to lay the keys out and to move up and down.
int g_imeKeyMap[10][5] = {
    {6, 16, 26, 35, 0},
    {7, 17, 27, 36, 0},
    {8, 18, 28, 37, 1},
    {9, 19, 29, 38, 1},
    {10, 20, 30, 40, 39},
    {11, 21, 31, 41, 39},
    {12, 22, 32, 42, 43},
    {13, 23, 33, 2, 43},
    {14, 24, 34, 3, 44},
    {15, 25, 4, 5, 44},
}; // 0x17fbd4
extern void *g_imeFont; // 0x18217c
extern SDL_Color g_imeTextColor; // 0x182284
SDL_Color g_imeInputColor; // 0x182498, original zero-initialized BSS: the typed text is black
SDL_Surface *loadImeImage(const char *, const char *, const char *); // 0x12a140
const char *languageString(int); // 0x135150
union SDL_Event;
int handleGlobalInput(SDL_Event *);
void drawScreenFrame(SDL_Surface *, const char *, int, bool); // 0x17568
int drawText(SDL_Surface *, void *, SDL_Rect, const char *, SDL_Color, int, bool); // 0x15d24
int drawTextLines(SDL_Surface *, void *, SDL_Rect *, const char *, const char *, SDL_Color *, int, bool); // 0x16018

static_assert(sizeof(ImeKey) == 100, "ImeKey layout");
static_assert(sizeof(ImeWindow) == 0xf0, "ImeWindow layout");

// title and label are language string ids (LangStrings.h).
ImeWindow::ImeWindow(int title, Action *callback, int label) : TrimusWindow(title, WIN_KEYBOARD)
{
    btn1Focused = loadImeImage(g_themePath, "./", "skin/bg-btn-01-f.png");
    btn1Normal = loadImeImage(g_themePath, "./", "skin/bg-btn-01-n.png");
    btn2Focused = loadImeImage(g_themePath, "./", "skin/bg-btn-02-f.png");
    btn2Normal = loadImeImage(g_themePath, "./", "skin/bg-btn-02-n.png");
    abc = loadImeImage(g_themePath, "./", "skin/icon-abc.png");
    character = loadImeImage(g_themePath, "./", "skin/icon-character.png");
    del = loadImeImage(g_themePath, "./", "skin/icon-delete.png");
    l2 = loadImeImage(g_themePath, "./", "skin/icon-L2.png");
    ok = loadImeImage(g_themePath, "./", "skin/icon-OK.png");
    r2 = loadImeImage(g_themePath, "./", "skin/icon-R2.png");
    shift = loadImeImage(g_themePath, "./", "skin/icon-shift.png");
    shiftActive = loadImeImage(g_themePath, "./", "skin/icon-shift-active.png");
    space = loadImeImage(g_themePath, "./", "skin/icon-space.png");
    start = loadImeImage(g_themePath, "./", "skin/icon-START.png");
    xIcon = loadImeImage(g_themePath, "./", "skin/icon-x.png");
    yIcon = loadImeImage(g_themePath, "./", "skin/icon-y.png");
    shifted = false;
    symbols = false;
    inputImage = loadImeImage(g_themePath, "./", "skin/input.png");
    selected = 0;
    memset(text, 0, 128);
    length = 0;
    this->callback = callback;
    this->label = languageString(label);
    buildKeys();
}
ImeWindow::~ImeWindow()
{
    if (btn1Focused) SDL_FreeSurface(btn1Focused);
    if (btn1Normal) SDL_FreeSurface(btn1Normal);
    if (btn2Focused) SDL_FreeSurface(btn2Focused);
    if (btn2Normal) SDL_FreeSurface(btn2Normal);
    if (abc) SDL_FreeSurface(abc);
    if (character) SDL_FreeSurface(character);
    if (del) SDL_FreeSurface(del);
    if (l2) SDL_FreeSurface(l2);
    if (ok) SDL_FreeSurface(ok);
    if (r2) SDL_FreeSurface(r2);
    if (shift) SDL_FreeSurface(shift);
    if (shiftActive) SDL_FreeSurface(shiftActive);
    if (space) SDL_FreeSurface(space);
    if (start) SDL_FreeSurface(start);
    if (xIcon) SDL_FreeSurface(xIcon);
    if (yIcon) SDL_FreeSurface(yIcon);
    if (inputImage) SDL_FreeSurface(inputImage);
    if (callback) delete callback;
}

// The ids of the keys in the same row as key (same y), left to right.
void ImeWindow::rowKeys(std::vector<int> &out, int key)
{
    int index = -1;
    out.clear();
    for (int i = 0; keys.size() > i; i++) {
        if (key == keys[i].id) {
            index = i;
            break;
        }
    }
    if (index >= 0 && keys.size() > index) {
        for (int i = 0; keys.size() > i; i++) {
            if (keys[i].rect.y == keys[index].rect.y) out.push_back(keys[i].id);
        }
    }
}
// The ids of key's column, top to bottom, from g_imeKeyMap; OK is appended when moving up (or in
// the last column), so moving up from the top row can wrap to it.
void ImeWindow::columnKeys(std::vector<int> &out, int key, bool up)
{
    int col = -1;
    out.clear();
    for (int i = 0; i < 10; i++) {
        col = -1;
        for (int j = 0; j < 5; j++) {
            if (key == g_imeKeyMap[i][j]) {
                col = i;
                break;
            }
        }
        if (col >= 0) break;
    }
    if (col < 0 && key == IME_KEY_OK) col = 9;
    if (col >= 0 && col < 10) {
        for (int i = 0; i < 5; i++) out.push_back(g_imeKeyMap[col][i]);
        if (col == 9 || up) out.push_back(IME_KEY_OK);
    }
}
// Moves the selection left or right within its row, wrapping around.
void ImeWindow::moveHorizontal(bool left)
{
    std::vector<int> row;
    int key = keys[selected].id;
    rowKeys(row, key);
    int index = -1;
    for (int i = 0; row.size() > i; i++) {
        if (key == row[i]) { index = i; break; }
    }
    if (index >= 0 && row.size() > index) {
        if (left) {
            index--;
            if (index < 0) index = row.size() - 1;
        } else {
            index++;
            if (index > row.size() - 1) index = 0;
        }
        int next = row[index];
        for (int i = 0; keys.size() > i; i++) {
            if (next == keys[i].id) { selected = i; break; }
        }
    }
}
// Moves the selection up or down within its column, wrapping around.
void ImeWindow::moveVertical(bool up)
{
    std::vector<int> column;
    int key = keys[selected].id;
    columnKeys(column, key, up);
    int index = -1;
    for (int i = 0; column.size() > i; i++) {
        if (key == column[i]) { index = i; break; }
    }
    if (index >= 0 && column.size() > index) {
        if (up) {
            index--;
            if (index < 0) index = column.size() - 1;
        } else {
            index++;
            if (index > column.size() - 1) index = 0;
        }
        int next = column[index];
        for (int i = 0; keys.size() > i; i++) {
            if (next == keys[i].id) { selected = i; break; }
        }
    }
}

// One button: returns the Action's result on OK, 1 (close) on B, -1 to stay open.
// Original quirks, kept as-is: X adds a space without checking the 128-byte limit, and A on a
// key returns 0 rather than -1.
int ImeWindow::handleInput()
{
    int action = handleGlobalInput(0);
    switch (action) {
    case BUTTON_START:
        printf("ImeWindow action %s\n", text);
        if (callback) return callback->execute(text);
        break;
    case BUTTON_Y:
        if ((unsigned int)length < 128) {
            if (length > 0) length--;
            text[length] = 0;
        }
        break;
    case BUTTON_X: text[length] = ' '; length++; break;
    case BUTTON_HOTKEY1:
        shifted = !shifted;
        if (shifted) setKeyIcons(IME_KEY_SHIFT, shiftActive, l2);
        else setKeyIcons(IME_KEY_SHIFT, shift, l2);
        break;
    case BUTTON_HOTKEY2:
        symbols = !symbols;
        if (symbols) setKeyIcons(IME_KEY_SYMBOLS, character, r2);
        else setKeyIcons(IME_KEY_SYMBOLS, abc, r2);
        break;
    case BUTTON_A:
        if ((unsigned int)length < 128) {
            if (keys[selected].id == IME_KEY_DELETE) {
                if (length > 0) length--;
                text[length] = 0;
            } else if (keys[selected].id == IME_KEY_SPACE) {
                text[length] = ' ';
                length++;
            } else if (keys[selected].id == IME_KEY_SHIFT) {
                shifted = !shifted;
                if (shifted) setKeyIcons(IME_KEY_SHIFT, shiftActive, l2);
                else setKeyIcons(IME_KEY_SHIFT, shift, l2);
            } else if (keys[selected].id == IME_KEY_SYMBOLS) {
                symbols = !symbols;
                if (symbols) setKeyIcons(IME_KEY_SYMBOLS, character, r2);
                else setKeyIcons(IME_KEY_SYMBOLS, abc, r2);
            } else if (keys[selected].id == IME_KEY_OK) {
                printf("ImeWindow action %s\n", text);
                if (callback) return callback->execute(text);
            } else if (symbols) {
                text[length] = keys[selected].symbol[0]; length++;
            } else if (shifted) {
                text[length] = keys[selected].upper[0]; length++;
            } else { text[length] = keys[selected].lower[0]; length++; }
        }
        return 0;
    case BUTTON_B: return 1;
    case BUTTON_LEFT: moveHorizontal(true); break;
    case BUTTON_RIGHT: moveHorizontal(false); break;
    case BUTTON_UP: moveVertical(true); break;
    case BUTTON_DOWN: moveVertical(false); break;
    default: break;
    }
    return -1;
}

// Draws every key in its current mode: background (focused for the selection), the wide keys'
// icon and button hint, and the character.
void ImeWindow::drawKeys(SDL_Surface *dst, int x, int y)
{
    const char *str = NULL;
    for (int i = 0; keys.size() > i; i++) {
        if (symbols) str = keys[i].symbol.c_str();
        else if (shifted) str = keys[i].upper.c_str();
        else str = keys[i].lower.c_str();
        if (i != selected) SDL_UpperBlit(keys[i].normal, NULL, dst, &keys[i].rect);
        else SDL_UpperBlit(keys[i].focused, NULL, dst, &keys[i].rect);
        if (keys[i].icon && keys[i].hint) {
            int width = 0;
            SDL_Rect rect;
            if (keys[i].hint) {
                if (keys[i].hint->w > 40) {
                    width = keys[i].hint->w + 6;
                    rect.x = keys[i].rect.x + keys[i].rect.w - width;
                } else {
                    width = keys[i].hint->w + 13;
                    rect.x = keys[i].rect.x + keys[i].rect.w - width;
                }
                rect.y = keys[i].rect.y + (keys[i].rect.h - keys[i].hint->h) / 2;
                SDL_UpperBlit(keys[i].hint, NULL, dst, &rect);
            }
            if (keys[i].icon) {
                rect.x = keys[i].rect.x + (keys[i].rect.w - width - keys[i].icon->w) / 2;
                rect.y = keys[i].rect.y + (keys[i].rect.h - keys[i].icon->h) / 2;
                SDL_UpperBlit(keys[i].icon, NULL, dst, &rect);
            }
        }
        if (keys[i].lower.size() != 0)
            drawText(dst, g_imeFont, keys[i].rect, str, g_imeTextColor, 1, false);
    }
}
// Title frame, a black panel, the input field with the text, and the keys.
int ImeWindow::draw(SDL_Surface *dst)
{
    drawScreenFrame(dst, languageString(titleLabelId), id, true);
    SDL_Rect rect;
    rect.x = 0; rect.y = 62; rect.w = 640; rect.h = 358;
    SDL_FillRect(dst, &rect, SDL_MapRGB(dst->format, 0, 0, 0));
    rect.x = 24; rect.y = 64; rect.w = 592; rect.h = 52;
    if (inputImage) SDL_UpperBlit(inputImage, NULL, dst, &rect);
    rect.x = 40;
    drawTextLines(dst, g_imeFont, &rect, text, "\n", &g_imeInputColor, 3, false);
    drawKeys(dst, 0, 114);
    return 0;
}
// Changes a key's icon and hint (shift and symbols show their state this way).
void ImeWindow::setKeyIcons(int key, SDL_Surface *icon, SDL_Surface *hint)
{
    for (int i = 0; keys.size() > i; i++) {
        if (key == keys[i].id) {
            keys[i].icon = icon;
            keys[i].hint = hint;
        }
    }
}
// Original case order is revealed by the read-only label pool, not numeric id order. The
// empty-label cases share one "" literal, so the pool cannot place them; the jump table at
// 0x140db0 (body addresses) puts 39, 43 and 44 right after 0 and 1.
void ImeWindow::initKey(ImeKey &key, int id, SDL_Surface *focused, SDL_Surface *normal,
                        SDL_Surface *icon, SDL_Surface *hint)
{
    key.id = id;
    key.focused = focused;
    key.normal = normal;
    key.icon = icon;
    key.hint = hint;
    switch (id) {
    case IME_KEY_SHIFT: key.lower = ""; key.upper = ""; key.symbol = ""; break;
    case IME_KEY_SYMBOLS: key.lower = ""; key.upper = ""; key.symbol = ""; break;
    case IME_KEY_SPACE: key.lower = ""; key.upper = ""; key.symbol = ""; break;
    case IME_KEY_DELETE: key.lower = ""; key.upper = ""; key.symbol = ""; break;
    case IME_KEY_OK: key.lower = ""; key.upper = ""; key.symbol = ""; break;
    case 6: key.lower = "1"; key.upper = "1"; key.symbol = "1"; break;
    case 7: key.lower = "2"; key.upper = "2"; key.symbol = "2"; break;
    case 8: key.lower = "3"; key.upper = "3"; key.symbol = "3"; break;
    case 9: key.lower = "4"; key.upper = "4"; key.symbol = "4"; break;
    case 10: key.lower = "5"; key.upper = "5"; key.symbol = "5"; break;
    case 11: key.lower = "6"; key.upper = "6"; key.symbol = "6"; break;
    case 12: key.lower = "7"; key.upper = "7"; key.symbol = "7"; break;
    case 13: key.lower = "8"; key.upper = "8"; key.symbol = "8"; break;
    case 14: key.lower = "9"; key.upper = "9"; key.symbol = "9"; break;
    case 15: key.lower = "0"; key.upper = "0"; key.symbol = "0"; break;
    case 16: key.lower = "q"; key.upper = "Q"; key.symbol = "!"; break;
    case 17: key.lower = "w"; key.upper = "W"; key.symbol = "@"; break;
    case 18: key.lower = "e"; key.upper = "E"; key.symbol = "#"; break;
    case 19: key.lower = "r"; key.upper = "R"; key.symbol = "$"; break;
    case 20: key.lower = "t"; key.upper = "T"; key.symbol = "%"; break;
    case 21: key.lower = "y"; key.upper = "Y"; key.symbol = "^"; break;
    case 22: key.lower = "u"; key.upper = "U"; key.symbol = "&"; break;
    case 23: key.lower = "i"; key.upper = "I"; key.symbol = "*"; break;
    case 24: key.lower = "o"; key.upper = "O"; key.symbol = "/"; break;
    case 25: key.lower = "p"; key.upper = "P"; key.symbol = "?"; break;
    case 26: key.lower = "a"; key.upper = "A"; key.symbol = "~"; break;
    case 27: key.lower = "s"; key.upper = "S"; key.symbol = "_"; break;
    case 28: key.lower = "d"; key.upper = "D"; key.symbol = "+"; break;
    case 29: key.lower = "f"; key.upper = "F"; key.symbol = "{"; break;
    case 30: key.lower = "g"; key.upper = "G"; key.symbol = "}"; break;
    case 31: key.lower = "h"; key.upper = "H"; key.symbol = "|"; break;
    case 32: key.lower = "j"; key.upper = "J"; key.symbol = ":"; break;
    case 33: key.lower = "k"; key.upper = "K"; key.symbol = "\""; break;
    case 34: key.lower = "l"; key.upper = "L"; key.symbol = "<"; break;
    case 4: key.lower = "-"; key.upper = "-"; key.symbol = ">"; break;
    case 35: key.lower = "z"; key.upper = "Z"; key.symbol = "`"; break;
    case 36: key.lower = "x"; key.upper = "X"; key.symbol = "-"; break;
    case 37: key.lower = "c"; key.upper = "C"; key.symbol = "="; break;
    case 38: key.lower = "v"; key.upper = "V"; key.symbol = "["; break;
    case 40: key.lower = "b"; key.upper = "B"; key.symbol = "]"; break;
    case 41: key.lower = "n"; key.upper = "N"; key.symbol = "\\"; break;
    case 42: key.lower = "m"; key.upper = "M"; key.symbol = ";"; break;
    case 2: key.lower = "("; key.upper = "("; key.symbol = "'"; break;
    case 3: key.lower = ")"; key.upper = ")"; key.symbol = ","; break;
    case 5: key.lower = "/"; key.upper = "/"; key.symbol = "."; break;
    }
}
// Lays out the 40 character keys (52 px squares, 8 px apart) and the five wide keys below.
void ImeWindow::buildKeys()
{
    keys.clear();
    ImeKey key;
    int height = 52;
    int width = 52;
    int x = 24;
    int y = 126;
    int xgap = 8;
    int ygap = 8;
    key.rect.w = width;
    key.rect.h = height;
    for (int row = 0; row < 4; row++) {
        key.rect.y = (height + ygap) * row + y;
        for (int col = 0; col < 10; col++) {
            key.rect.x = (width + xgap) * col + x;
            initKey(key, g_imeKeyMap[col][row], btn1Focused, btn1Normal, NULL, NULL);
            keys.push_back(key);
        }
    }
    width = 112;
    height = 52;
    key.rect.w = width;
    key.rect.h = height;
    key.rect.x = 24; key.rect.y = 362;
    initKey(key, IME_KEY_SHIFT, btn2Focused, btn2Normal, shift, l2); keys.push_back(key);
    key.rect.x = 144; key.rect.y = 362;
    initKey(key, IME_KEY_SYMBOLS, btn2Focused, btn2Normal, abc, r2); keys.push_back(key);
    key.rect.x = 264; key.rect.y = 362;
    initKey(key, IME_KEY_SPACE, btn2Focused, btn2Normal, space, xIcon); keys.push_back(key);
    key.rect.x = 384; key.rect.y = 362;
    initKey(key, IME_KEY_DELETE, btn2Focused, btn2Normal, del, yIcon); keys.push_back(key);
    key.rect.x = 504; key.rect.y = 362;
    initKey(key, IME_KEY_OK, btn2Focused, btn2Normal, ok, start); keys.push_back(key);
}
void ImeWindow::onHide() {}
void ImeWindow::onShow() {}
void ImeWindow::saveState(cJSON *) {}
int ImeWindow::restoreState(cJSON *) { return 0; }
void ImeWindow::update() {}
