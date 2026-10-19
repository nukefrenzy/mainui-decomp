// On-screen keyboard. Recovered layout: 0xf0 bytes; each key is 100 bytes.
#pragma once
#include <string>
#include <vector>
#include "TrimusWindow.h"

struct SDL_PixelFormat;
// SDL 1.2 prefix used through pointers here; surfaces are owned/allocated by SDL.
struct SDL_Surface {
    unsigned int flags;
    SDL_PixelFormat *format;
    int w, h;
};
struct SDL_Rect {
    short x, y;
    unsigned short w, h;
};
struct SDL_Color { unsigned char r, g, b, unused; };
struct ImeKey {
    int id;
    SDL_Rect rect;
    std::string lower, upper, symbol;
    SDL_Surface *focused, *normal, *icon, *hint;
};
class ImeCallback {
public:
    virtual ~ImeCallback();
    virtual int execute(char *text) = 0;
};
class ImeWindow : public TrimusWindow {
public:
    ImeWindow(int title, ImeCallback *callback, int label);
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

    SDL_Surface *btn1Focused, *btn1Normal, *btn2Focused, *btn2Normal; // +0x0c
    SDL_Surface *abc, *character, *del, *l2, *ok, *r2; // +0x1c
    SDL_Surface *shift, *shiftActive, *space, *start, *xIcon, *yIcon; // +0x34
    SDL_Surface *inputImage; // +0x4c
    int selected;
    char text[128];
    int length;
    std::vector<ImeKey> keys; // +0xd8
    bool shifted, symbols; // +0xe4
    ImeCallback *callback; // +0xe8
    const char *label;
};
