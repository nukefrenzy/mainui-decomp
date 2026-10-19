// Two helpers shared by MenuWindow and the DB-backed text menus.
#pragma once
#include <string>

struct SDL_Surface;

// Background worker pool: MenuWindow builds it with 2, MultiDBCachedTextMenu with 10.
struct MenuHelper60 {                       // 32 bytes
    MenuHelper60(int n);                    // 0x1323e0
    ~MenuHelper60();                        // 0x13249c
    void addWorker(void *worker);           // 0x1325d0, queues a BGWorker (DBCacheWorker)
    char raw[0x20];
};

// Keyed cache; the callback frees an entry (SDL_FreeSurface wrapper, operator delete, free).
struct MenuHelper80 {                       // 0x14 bytes (MultiDBCachedTextMenu has two back to back)
    MenuHelper80(void (*freeFn)(SDL_Surface *));  // 0x131e34
    ~MenuHelper80();                        // 0x131e94
    void *get(std::string key);             // 0x1320dc, NULL until a worker has filled the entry
    char raw[0x14];
};
