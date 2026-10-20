// TrimusWindow: base of every window on MainUI's window stack.
// Layout and virtual order verified byte-exact against MainUI-354-clean (see MATCHES.txt).
#pragma once

struct SDL_Surface;
typedef void (*FlipFn)();
struct cJSON;

class TrimusWindow {
public:
    TrimusWindow(int titleLabelId, int id);           // 0x1d7dc
    virtual ~TrimusWindow();                          // slots 0-1, body 0x1d830
    virtual int  handleInput() = 0;                   // slot 2: result code (-1 none, 1/4 close, 3 close+exit, 2 quit)
    virtual int  draw(SDL_Surface *dst) = 0;          // slot 3
    virtual void onHide() = 0;                        // slot 4: before the window is deleted
    virtual void onShow() = 0;                        // slot 5: when it becomes the top window
    virtual void saveState(cJSON *obj) = 0;           // slot 6: called by saveWindowState (/tmp/state.json)
    virtual int  restoreState(cJSON *obj) = 0;        // slot 7: called by restoreWindowState
    virtual void update() = 0;                        // slot 8: every frame, before draw
    // Inline: both sit in tu_015b20's template/inline tail, not in its body.
    virtual void animateIn(SDL_Surface *dst, FlipFn flip)    // slot 9, default empty (0x39478)
    {
    }
    virtual void animateOut(SDL_Surface *dst, FlipFn flip)   // slot 10, default empty (0x394a0)
    {
    }

    const char *getTitle();                            // 0x394f0, languageString(titleLabelId)
    int getId();                                      // 0x394c8, inline in the tail; see TrimusWindow.cpp
    void dump();                                      // 0x1d898 printf("title %s, type %d"); name unverified

    // Public: saveWindowState (0x1c544) reads both fields directly.
    int titleLabelId;   // +0x04  language string id for the title (e.g. 0x83 = Wi-Fi)
    int id;             // +0x08  window/menu id (MenuWindow: the createMenu id)
};
