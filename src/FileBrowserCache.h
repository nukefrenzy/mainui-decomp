// Directory contents and bounded navigation cache used by FileBrowserMenu.
#pragma once
#include <string>
#include <vector>
#include "Action.h"
using FileFilter = bool (*)(const char *, const char *);
class FileAction : public Action {
public:
    FileAction(const char *name, int kind) : Action(-1) { path = name; this->kind = kind; }
    virtual ~FileAction();
    virtual int execute(void *);
    virtual int progress(int *, int *);
    virtual int finished();
    std::string path;
    int kind;
};
// Only the prefix needed for sorting is named; remaining fields are handled by its external methods.
class TextItem {
public:
    TextItem(const char *, int, Action *, bool);
    ~TextItem();
    std::string label;
    Action *action;
    char raw[0x74 - 0x1c];
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
