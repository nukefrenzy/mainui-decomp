// Directory contents and bounded navigation cache used by FileBrowserMenu.
#pragma once
#include <string>
#include <vector>
#include "Action.h"
struct SDL_Surface;
using FileFilter = bool (*)(const char *, const char *);
// vtable 0x153ed8: slots 3/4 are Action's; dtors 0x3a1cc/0x3a218 are inline in tu_015b20's tail.
class FileAction : public Action {
public:
    FileAction(const char *name, int kind) : Action(-1) { path = name; this->kind = kind; }
    virtual ~FileAction() {}
    virtual int execute(void *);    // 0x2d1bc (tu_015b20 p08)
    std::string path;
    int kind;
};
// One list row; sizeof == 0x74. Ctor 0x25510 and dtor 0x25620 are in tu_015b20 (p06).
class TextItem {
public:
    TextItem(const char *label, SDL_Surface *value, Action *action, bool flag);  // 0x25510
    ~TextItem();                                                        // 0x25620
    std::string label;
    Action *action;     // +0x18
    bool flag1c;        // +0x1c  ctor flag
    bool favorite;      // +0x1d  set by AddFavoriteAction, shown by the game detail window
    bool removed;       // +0x1e  set by RemoveFavoriteAction
    SDL_Surface *value; // +0x20  ctor value; TextMenuDrawer::vfunc7 draws it as the row icon (p04)
    bool flag24;        // +0x24  false
    std::string str28;  // +0x28
    std::string str40;  // +0x40
    std::string str58;  // +0x58
    SDL_Surface *field70; // +0x70  0; thumbnail loaded from str58 by TextMenuDrawer::vfunc7 (p04)
};
struct DirectoryItems {
    std::string path;
    std::vector<void *> *items;
};
class FileBrowserCache {
public:
    FileBrowserCache(FileFilter);
    ~FileBrowserCache();
    void clear();
    void freeItems(std::vector<void *> *);
    int totalItems();
    int trim();
    std::vector<void *> *find(const char *);
    std::vector<void *> *open(const char *);
    std::vector<void *> *parent();
    std::vector<void *> *child(const char *);
    std::vector<std::string> paths;
    std::vector<DirectoryItems> directories;
    FileFilter filter;
};
