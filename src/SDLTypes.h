// SDL 1.2 structures, declared with only the members MainUI uses.
#pragma once
struct SDL_PixelFormat {
    void *palette;
    unsigned char bitsPerPixel, bytesPerPixel, rLoss, gLoss, bLoss, aLoss, rShift, gShift, bShift, aShift;
    unsigned rMask, gMask, bMask, aMask;
};
// SDL 1.2 prefix used through pointers here; surfaces are owned/allocated by SDL.
struct SDL_Surface {
    unsigned int flags;
    SDL_PixelFormat *format;
    int w, h;
    unsigned short pitch;
    void *pixels;
};
struct SDL_Rect {
    short x, y;
    unsigned short w, h;
};
struct SDL_Color {
    unsigned char r, g, b, unused;
};

// SDL 1.2 events: the members MainUI reads (keyboard, touch/mouse button, user events).
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
// A user event (SDL_USEREVENT). Timers and threads push just this 16-byte part, as the original does.
struct SDL_UserEvent {
    unsigned char type;
    int code;
    void *data1, *data2;
};
union SDL_Event {
    unsigned char type;
    SDL_KeyboardEvent key;
    SDL_MouseButtonEvent button;
    SDL_UserEvent user;
};
static_assert(sizeof(SDL_UserEvent) == 16 && sizeof(SDL_Event) == 20, "SDL 1.2 event layouts");
