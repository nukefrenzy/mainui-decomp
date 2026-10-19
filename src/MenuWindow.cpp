#include "MenuWindow.h"

extern "C" void SDL_FreeSurface(SDL_Surface *s);
extern char *g_themePath;                   // 0x181970
extern SDL_Surface *g_backBuffer;           // 0x181928
extern const char g_skinSubdir[];           // 0x151a48 (second argument of every skin load)
SDL_Surface *loadSkinImage(const char *theme, const char *sub, const char *file,
                           SDL_Surface *fmt, int flags);   // 0x23af0
void freeSurfaceCallback(SDL_Surface *s);  // 0x2e318

MenuWindow::MenuWindow(int titleLabelId, int id, ListMenu *menu)
    : TrimusWindow(titleLabelId, id), helper60(2), helper80(freeSurfaceCallback)
{
    this->menu = menu;
    flag1c = false;
    flag1d = false;
    field38 = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    flag1c = false;
    flag1d = false;
    field54 = true;
    surface58 = 0;
    emuEntry = 0;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    surfaceFC = 0;
    ownsSurfaceFC = false;
}

MenuWindow::~MenuWindow()
{
    if (surface58 != 0) {
        SDL_FreeSurface(surface58);
    }
    if (iconTF != 0) {
        SDL_FreeSurface(iconTF);
    }
    if (ownsSurfaceFC && surfaceFC != 0) {
        SDL_FreeSurface(surfaceFC);
    }
}
