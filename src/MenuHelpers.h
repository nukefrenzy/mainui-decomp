// Cache and background worker queue shared by the menu classes.
#pragma once
#include <string>
#include <list>
struct SDL_Surface;
struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
// Ctor 0x393dc (implicit), dtor 0x39410 and deleting dtor 0x39444 are inline in tu_015b20's tail.
class BGWorker {
public:
    virtual ~BGWorker() {}
    virtual void run() = 0;         // slot 2, on the worker thread
    virtual void finished() = 0;    // slot 3
};
struct CacheEntry {
    std::string key;
    void *value;
};
struct MenuHelper80 { // 0x14 bytes
    MenuHelper80(void (*freeFn)(SDL_Surface *));
    ~MenuHelper80();
    void clear();
    int put(std::string key, void *value);
    void *get(std::string key);
    unsigned int count();
    std::list<CacheEntry> entries;
    void (*freeFn)(SDL_Surface *);
    SDL_mutex *mutex;
};
struct MenuHelper60 { // 0x20 bytes
    MenuHelper60(int n);
    ~MenuHelper60();
    int addWorker(BGWorker *worker);
    BGWorker *takeWorker();
    int limit;
    SDL_mutex *mutex;
    SDL_cond *condition;
    std::list<BGWorker *> workers;
    SDL_Thread *thread;
    bool running;
};
