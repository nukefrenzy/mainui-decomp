// Directory contents and bounded navigation cache used by FileBrowserMenu (tu_12ce28.cpp).
#pragma once
#include <string>
#include <vector>
#include "Action.h"
struct SDL_Surface;
using FileFilter = bool (*)(const char *, const char *);
// FileAction.kind: what a file-browser row stands for.
enum FileKind {
    FILE_KIND_FILE = 101,        // a file: launch it (or offer a TrimuiUpdate*.zip as an update)
    FILE_KIND_DIRECTORY = 102,   // a folder: open it (EVENT_OPEN_DIRECTORY)
};
// The action behind one file-browser row: its full path and kind.
// vtable 0x153ed8: slots 3/4 are Action's; dtors 0x3a1cc/0x3a218 are inline in tu_015b20's tail.
class FileAction : public Action
{
public:
    FileAction(const char *name, int kind) : Action(-1)
    {
        path = name;
        this->kind = kind;
    }
    virtual ~FileAction() {}
    virtual int execute(void *);    // 0x2d1bc (tu_015b20 p08)
    std::string path;
    int kind;
};
// One list row; sizeof == 0x74. Ctor 0x25510 and dtor 0x25620 are in tu_015b20 (p06).
class TextItem
{
public:
    TextItem(const char *label, SDL_Surface *value, Action *action, bool flag);  // 0x25510
    ~TextItem();                                                        // 0x25620
    std::string label;
    Action *action;     // +0x18
    bool ownsAction;        // +0x1c  ctor flag
    bool favorite;      // +0x1d  set by AddFavoriteAction, shown by the game detail window
    bool removed;       // +0x1e  set by RemoveFavoriteAction
    SDL_Surface *value; // +0x20  ctor value; TextMenuDrawer::drawList draws it as the row icon (p04)
    bool showThumbnail;        // +0x24  false
    std::string subtitle;  // +0x28
    std::string displayLabel;  // +0x40
    std::string thumbnailPath;  // +0x58
    SDL_Surface *thumbnailSurface; // +0x70  0; thumbnail loaded from thumbnailPath by TextMenuDrawer::drawList (p04)
};
// One directory's rows (TextItem*), as loaded.
struct DirectoryItems {
    std::string path;
    std::vector<void *> *items;
};
// The file browser's navigation: the path from the starting folder to the current one, and the
// rows of recently visited folders (most recent first) so going back is instant. When the cached
// rows pass 40,000, the least recently visited folders are dropped.
class FileBrowserCache
{
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
    std::vector<std::string> paths;             // the folders entered, from the starting one
    std::vector<DirectoryItems> directories;    // cached folders, most recently used first
    FileFilter filter;                          // which files to list (fileBrowserFilter)
};
