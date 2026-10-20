// Directory contents and bounded navigation cache used by FileBrowserMenu.
#pragma once
#include <string>
#include <vector>
#include "Action.h"
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
    TextItem(const char *label, int value, Action *action, bool flag);  // 0x25510
    ~TextItem();                                                        // 0x25620
    std::string label;
    Action *action;     // +0x18
    bool flag1c;        // +0x1c  ctor flag
    bool favorite;      // +0x1d  set by AddFavoriteAction, shown by the game detail window
    bool removed;       // +0x1e  set by RemoveFavoriteAction
    int value;          // +0x20  ctor value
    bool flag24;        // +0x24  false
    std::string str28;  // +0x28
    std::string str40;  // +0x40
    std::string str58;  // +0x58
    int field70;        // +0x70  0
};
struct DirectoryItems {
    std::string path;
    std::vector<TextItem *> *items;
};
class FileBrowserCache {
public:
    FileBrowserCache(FileFilter);
    ~FileBrowserCache();
    void clear();
    void freeItems(std::vector<TextItem *> *);
    int totalItems();
    int trim();
    std::vector<TextItem *> *find(const char *);
    std::vector<TextItem *> *open(const char *);
    std::vector<TextItem *> *parent();
    std::vector<TextItem *> *child(const char *);
    std::vector<std::string> paths;
    std::vector<DirectoryItems> directories;
    FileFilter filter;
};
