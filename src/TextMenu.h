// Text-row menu shared with hardware tests; preserve inline definition order.
#pragma once
#include <stdio.h>
#include "TextMenuDrawer.h"
class TextItem;

class TextMenu : public TextMenuDrawer {
public:
    TextMenu(bool a, bool b, int c, const char *background);  // 0x20500
    virtual ~TextMenu();                                       // 0x20570 (D0 0x205c4)
    virtual void *getItem(int index);                          // 0x20714 slot 5, a TextItem *
    virtual int  count() { return items.size(); }              // 0x3a6dc, inline
    virtual void vfunc12(int);                                 // 0x2079c
    virtual void vfunc13();                                    // 0x206e0
    virtual void vfunc18();                                    // 0x205f8 slot 18 (new)
    void addItem(void *item);                              // 0x20898
    void insertSorted(void *item, int (*cmp)(TextItem *, TextItem *)); // 0x208d0
    bool addUnique(TextItem *item);                            // 0x20a3c  false if the label is listed
    std::vector<void *> *getItems()                        // 0x3a70c, inline
    {
        printf("TextMenu %p mItemList %p\n", this, &items);
        return &items;
    }
    std::vector<void *> items;  // +0xf8, TextItem / SettingMenuItem / DeviceInfoItem rows
};
