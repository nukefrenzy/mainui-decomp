// ListMenu / TextMenuDrawer: the list inside a MenuWindow. Only what derived menus touch is laid out;
// slot names are signposts until the base bodies are read.
#pragma once
#include <string>

class ListMenu {
public:
    virtual ~ListMenu();                                       // slots 0-1
    virtual void restorePosition(int a, int cur, int c);       // slot 2
    virtual void vfunc3();
    virtual void vfunc4();
    virtual void *getItem(int index);                          // slot 5
    virtual void *vfunc6(int index);                           // slot 6
    virtual void vfunc7();
    virtual int  count();                                      // slot 8
    virtual int  vfunc9();
    virtual int  vfunc10();
    virtual void vfunc11();
    virtual void vfunc12();
    virtual void vfunc13();
    virtual void vfunc14();
    virtual void vfunc15();
    virtual int  vfunc16();
    virtual int  vfunc17();

    int         field04;
    int         field08;
    int         cur;            // +0x0c  current item
    int         pageStart;      // +0x10
    int         pageEnd;        // +0x14
    std::string str18;          // +0x18  search string for the DB menus
    char        raw30[0x28];
    int         field58;        // +0x58
    char        raw5c[0xf8 - 0x5c];
};

class TextMenuDrawer : public ListMenu {
public:
    TextMenuDrawer(int a, int b, int c, const char *background);  // 0x1e128
    virtual ~TextMenuDrawer();                                     // 0x1e3f8
    virtual void restorePosition(int a, int cur, int c);           // 0x1d9d8
    virtual int  vfunc9();                                         // 0x1dae8
    virtual int  vfunc10();                                        // 0x1db6c
    virtual int  vfunc16();                                        // 0x1dbdc
    virtual int  vfunc17();                                        // 0x1dc30
};
