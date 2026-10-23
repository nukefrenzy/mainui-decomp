// MultiDBCachedTextMenu: global search across every emulator's ROM cache database.
// sizeof == 0x400 (operator delete size in the deleting destructor).
#pragma once
#include <string>
#include <vector>
#include "TextMenuDrawer.h"
#include "MenuHelpers.h"

struct sqlite3;

// One ROM source (emulator or app list entry). sizeof == 0xdc (std::vector<EmuInfo>::size at 0x3b3bc).
struct EmuInfo {
    std::string label, labelVerbose, icon, iconSelected, launch;
    std::string path;           // +0x78  ROM folder
    std::string imagePath, gameList;
    int useSwap;               // +0xc0
    int hasCpinyin;            // +0xc4  JSON shortname; search uses the cpinyin column
    int hideBios;              // +0xc8
    std::vector<std::string> extensions; // +0xcc
    int noAudioFix;            // +0xd8
};

// One opened cache database that has hits for the search string.
struct DBEntry {                // 0x24 bytes
    sqlite3 *db;
    std::string name;           // last path component of EmuInfo::path, also the table prefix
    int count;          // matching rows
    EmuInfo *emu;
};

class MultiDBCachedTextMenu : public TextMenuDrawer
{
public:
    MultiDBCachedTextMenu(std::string search);    // 0x1485fc, from MenuWindow::onShow
    virtual ~MultiDBCachedTextMenu();                    // 0x14879c (D2), 0x14889c (D0)
    virtual void restorePosition(int savedCur, int savedStart, int savedEnd); // 0x14909c
    virtual void *getItem(int index);                    // 0x149514
    virtual void *getItemData(int index);                     // 0x149824
    virtual int count() { return total; }               // 0x149b54
    virtual int moveDown();                               // 0x149024
    virtual int moveUp();                              // 0x149060
    virtual int pageUp();                              // 0x148fac
    virtual int pageDown();                              // 0x148fe8

    int openCache(EmuInfo &emu, char *buf, size_t size);   // 0x1488d0
    void prepare();                                      // 0x148bc4
    void fetchForward();                                 // 0x148eac
    void fetchBackward();                                // 0x148f30
    void prepareBGWorkers(int index, int len);           // 0x149138

    WorkerQueue workers;     // +0xf8   loads result rows in the background (10 waiting)
    LruCache items;       // +0x118  loaded rows (TextItem), freed with operator delete
    LruCache data;        // +0x12c  per-row data, freed with free()
    int total;       // +0x140  sum of DBEntry::count
    std::string name;        // +0x144  "global_search", table prefix for workers
    char key[0x280];  // +0x15c
    std::string dir;         // +0x3dc  "."
    std::vector<DBEntry> dbs;         // +0x3f4
};
