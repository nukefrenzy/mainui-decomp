// p09 0x2e424-0x3225c: MenuWindow, FloatWindow
// Functions in address order; see docs/plan-tu_015b20.md.

// 0x2e424-0x2e6d4: not written yet.

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
