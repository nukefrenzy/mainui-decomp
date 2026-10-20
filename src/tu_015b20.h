// Shared declarations for tu_015b20 (0x15b20-0x49c3c): window and menu classes, actions, the window
// loop and createMenu. Each part file in src/tu_015b20/ adds only the declarations it needs here.
#pragma once
#include <stdio.h>
#include "TrimusWindow.h"
#include "MenuWindow.h"

// p01
const char *keyEventName(int key);          // 0x15b20, unreferenced
void writeCmdToRun(const char *cmd);        // 0x17fc0

// p09
extern "C" void SDL_FreeSurface(SDL_Surface *s);
extern char *g_themePath;                   // 0x181970
extern SDL_Surface *g_backBuffer;           // 0x181928
extern const char g_skinSubdir[];           // 0x151a48 (second argument of every skin load)
SDL_Surface *loadSkinImage(const char *theme, const char *sub, const char *file,
                           SDL_Surface *fmt, bool flags);   // 0x123af0
void freeSurfaceCallback(SDL_Surface *s);  // 0x2e318
