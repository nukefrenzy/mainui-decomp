// FileBrowserMenu directory traversal and navigation cache, in binary address order.
// Proven 0x12ce28-0x1302cc: 4,780 body bytes plus 105 inline/template functions.
// The draft tu_12ef8c was part of this file's generated tail.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include "FileBrowserCache.h"
static_assert(sizeof(FileBrowserCache) == 0x1c && sizeof(DirectoryItems) == 28, "browser cache layouts");
static_assert(sizeof(FileAction) == 0x24 && sizeof(TextItem) == 0x74, "browser item layouts");
FileBrowserCache::FileBrowserCache(FileFilter callback)
{
    filter = callback;
}
FileBrowserCache::~FileBrowserCache()
{
    clear();
}
void FileBrowserCache::clear()
{
    for (int i = 0; i < directories.size(); i++) {
        freeItems(directories[i].items);
        delete directories[i].items;
        directories[i].items = NULL;
    }
    directories.clear();
}
int compareFileItems(TextItem *left, TextItem *right)
{
    if (right->action->result != left->action->result)
        return right->action->result - left->action->result;
    return strcmp(left->label.c_str(), right->label.c_str());
}
void insertFileItem(std::vector<void *> *items, TextItem *item)
{
    int i = 0;
    while (i < items->size()) {
        if (compareFileItems(item, (TextItem *)(*items)[i]) <= 0) break;
        i++;
    }
    if (i < items->size()) items->insert(items->begin() + i, (void *)item);
    else items->push_back((void *)item);
}
int countDirectory(std::string path, FileFilter filter, bool nested, char *buffer)
{
    int count = 0;
    int found = 0;
    DIR *dir;
    dirent *entry;
    struct stat info;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            sprintf(buffer, "%s/%s", path.c_str(), entry->d_name);
            if (stat(buffer, &info) == 0) {
                if (S_ISREG(info.st_mode)) {
                    if (filter) {
                        if (filter(entry->d_name, buffer)) count++;
                    } else count++;
                } else if (S_ISDIR(info.st_mode) && strncmp(entry->d_name, ".", 1) != 0) {
                    std::string next = path + "/" + entry->d_name;
                    found = countDirectory(next, filter, 1, buffer);
                    count += found;
                    if (found > 0) count++;
                }
            }
        }
        closedir(dir);
    }
    return count;
}
int countDirectoryItems(std::string path, FileFilter filter)
{
    int result = 0;
    char *buffer = (char *)malloc(path.length() + 256);
    if (!buffer) return -1;
    {
        result = countDirectory(path, filter, 0, buffer);
        free(buffer);
    }
    return result;
}
int loadDirectory(std::vector<void *> *items, const char *path, FileFilter filter, int limit, int parent)
{
    int count = 0;
    char *buffer = (char *)malloc(strlen(path) + 256);
    DIR *dir;
    dirent *entry;
    struct stat info;
    if (!buffer) return -1;
    if ((dir = opendir(path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            sprintf(buffer, "%s/%s", path, entry->d_name);
            if (stat(buffer, &info) == 0) {
                if (S_ISDIR(info.st_mode) && strlen(entry->d_name) != 0 &&
                    (entry->d_name[0] != '.' || (parent && strncmp(entry->d_name, "..", 2) == 0))) {
                    if (count >= limit) break;
                    insertFileItem(items, new TextItem(entry->d_name, 0, new FileAction(buffer, 0x66), 1));
                    count++;
                } else {
                    if (S_ISREG(info.st_mode)) {
                        if (filter(entry->d_name, buffer)) {
                            if (count >= limit) break;
                            insertFileItem(items, new TextItem(entry->d_name, 0, new FileAction(buffer, 0x65), 1));
                            count++;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    printf("loaddir %s %d\n", path, count);
    free(buffer);
    return count;
}
void FileBrowserCache::freeItems(std::vector<void *> *items)
{
    for (int i = 0; i < items->size(); i++) {
        TextItem *item = (TextItem *)(*items)[i];
        delete item;
    }
    items->clear();
}
int FileBrowserCache::totalItems()
{
    int result = 0;
    for (int i = 0; i < directories.size(); i++) result += directories[i].items->size();
    return result;
}
int FileBrowserCache::trim()
{
    if (directories.size() > 1) {
        int count = totalItems();
        DirectoryItems entry;
        while (60000 - count < 20000) {
            entry = directories.back();
            directories.pop_back();
            freeItems(entry.items);
            delete entry.items;
            count = totalItems();
        }
    }
    return 0;
}
std::vector<void *> *FileBrowserCache::find(const char *path)
{
    DirectoryItems entry;
    for (int i = 0; i < directories.size(); i++) {
        if (strcmp(path, directories[i].path.c_str()) == 0) {
            entry = directories[i];
            directories.erase(directories.begin() + i);
            directories.insert(directories.begin(), entry);
            printf("found menu for %s\n", path);
            return entry.items;
        }
    }
    return NULL;
}
std::vector<void *> *FileBrowserCache::open(const char *path)
{
    clear();
    paths.clear();
    paths.push_back(std::string(path));
    DirectoryItems entry;
    entry.path = path;
    entry.items = new std::vector<void *>;
    int count = loadDirectory(entry.items, path, filter, 20000, 0);
    directories.push_back(entry);
    return entry.items;
}
std::vector<void *> *FileBrowserCache::parent()
{
    if (paths.size() > 1) paths.pop_back();
    std::string path(paths.back());
    std::vector<void *> *items = find(path.c_str());
    if (items) return items;
    {
        trim();
        DirectoryItems entry;
        entry.path = path;
        entry.items = new std::vector<void *>;
        int count = loadDirectory(entry.items, path.c_str(), filter, 20000, paths.size() > 1);
        directories.insert(directories.begin(), entry);
        printf("parentdir %s return %p\n", path.c_str(), entry.items);
        return entry.items;
    }
    return items;
}
std::vector<void *> *FileBrowserCache::child(const char *path)
{
    paths.push_back(std::string(path));
    std::vector<void *> *items = find(path);
    if (items) return items;
    {
        trim();
        DirectoryItems entry;
        entry.path = path;
        entry.items = new std::vector<void *>;
        int count = loadDirectory(entry.items, path, filter, 20000, 1);
        directories.insert(directories.begin(), entry);
        printf("childdir %s return %p\n", path, entry.items);
        return entry.items;
    }
    return items;
}
