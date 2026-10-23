// The two helpers behind the menus' thumbnails and previews (implemented in MenuHelpers.cpp):
// an LRU cache of loaded images keyed by path, and a queue of background jobs (BGWorker)
// that one worker thread runs, newest first. Formerly MenuHelper80/MenuHelper60 (their offsets
// in MenuWindow).
#pragma once
#include <string>
#include "BGWorker.h"
#include <list>
struct SDL_Surface;
struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
// Ctor 0x393dc (implicit), dtor 0x39410 and deleting dtor 0x39444 are inline in MainUI.cpp's tail.

struct CacheEntry {
    std::string key;
    void *value;
};
// Thread-safe cache: most recently used entries at the front; beyond 1000 entries the oldest
// are freed with freeFn. Values are SDL surfaces in practice.
struct LruCache { // 0x14 bytes
    LruCache(void (*freeFn)(SDL_Surface *));
    ~LruCache();
    void clear();
    int put(std::string key, void *value);
    void *get(std::string key);
    unsigned int count();
    std::list<CacheEntry> entries;
    void (*freeFn)(SDL_Surface *);
    SDL_mutex *mutex;
};
// A job queue with one worker thread. Holds at most `limit` waiting jobs (default 5): adding
// one more drops the oldest waiting job, so a fast-scrolling menu only loads what is current.
struct WorkerQueue { // 0x20 bytes
    WorkerQueue(int n);
    ~WorkerQueue();
    int addWorker(BGWorker *worker);
    BGWorker *takeWorker();
    int limit;                      // most waiting jobs kept
    SDL_mutex *mutex;               // guards workers and running
    SDL_cond *condition;            // signaled when a job arrives or at shutdown
    std::list<BGWorker *> workers;  // waiting jobs, newest first
    SDL_Thread *thread;
    bool running;
};
