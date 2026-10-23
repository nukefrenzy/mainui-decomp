// The on-screen keyboard (ImeWindow.cpp): a window for typing a line of text (Wi-Fi password,
// hotspot name, search), which it hands to an Action. The window is 0xf0 bytes; each key
// is 100 bytes.
#pragma once
#include <string>
#include "CJsonApi.h"
#include <vector>
#include "TrimusWindow.h"

#include "SDLTypes.h"
#include "Action.h"

// Ids of the five wide keys in the bottom row; the 40 character keys have ids 2-38 and 40-42
// (see g_imeKeyMap and initKey).
enum ImeKeyId {
    IME_KEY_SHIFT = 0,      // L2
    IME_KEY_SYMBOLS = 1,    // R2: letters <-> symbols
    IME_KEY_SPACE = 39,     // X
    IME_KEY_DELETE = 43,    // Y
    IME_KEY_OK = 44,        // START
};
// One key: its rectangle, the character it types in each mode, and its images (button
// background focused/normal, and for the wide keys an icon plus the hint of its button).
struct ImeKey {
    int id;
    SDL_Rect rect;
    std::string lower, upper, symbol;
    SDL_Surface *focused, *normal, *icon, *hint;
};
class ImeWindow : public TrimusWindow
{
public:
    ImeWindow(int title, Action *callback, int label);
    virtual ~ImeWindow();
    void rowKeys(std::vector<int> &out, int key);
    void columnKeys(std::vector<int> &out, int key, bool up);
    void moveHorizontal(bool left);
    void moveVertical(bool up);
    virtual int handleInput();
    void drawKeys(SDL_Surface *dst, int x, int y);
    virtual int draw(SDL_Surface *dst);
    void setKeyIcons(int key, SDL_Surface *icon, SDL_Surface *hint);
    void initKey(ImeKey &key, int id, SDL_Surface *focused, SDL_Surface *normal,
                 SDL_Surface *icon, SDL_Surface *hint);
    void buildKeys();
    virtual void onHide();
    virtual void onShow();
    virtual void saveState(cJSON *obj);
    virtual int restoreState(cJSON *obj);
    virtual void update();

    SDL_Surface *btn1Focused, *btn1Normal, *btn2Focused, *btn2Normal; // +0x0c: small/wide keys
    SDL_Surface *abc, *character, *del, *l2, *ok, *r2; // +0x1c
    SDL_Surface *shift, *shiftActive, *space, *start, *xIcon, *yIcon; // +0x34
    SDL_Surface *inputImage; // +0x4c
    int selected;           // index into keys
    char text[128];         // the text typed so far
    int length;
    std::vector<ImeKey> keys; // +0xd8
    bool shifted, symbols; // +0xe4
    Action *callback; // +0xe8: runs with the text on OK; owned (deleted with the window)
    const char *label;      // the OK action's label (unused when drawing)
};
