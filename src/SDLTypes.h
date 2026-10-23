// SDL 1.2 layouts used by the recovered UI code.
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
