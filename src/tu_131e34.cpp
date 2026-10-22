// Mutex-protected keyed cache and single-threaded background worker queue.
// Proven 0x131e34-0x134080: 2,348 body bytes plus 110 generated functions.
#include "MenuHelpers.h"
extern "C" {
SDL_mutex *SDL_CreateMutex(); void SDL_DestroyMutex(SDL_mutex *);
int SDL_mutexP(SDL_mutex *); int SDL_mutexV(SDL_mutex *);
SDL_cond *SDL_CreateCond(); void SDL_DestroyCond(SDL_cond *);
int SDL_CondSignal(SDL_cond *); int SDL_CondWait(SDL_cond *, SDL_mutex *);
SDL_Thread *SDL_CreateThread(int (*)(void *), void *);
void SDL_WaitThread(SDL_Thread *, int *);
}
static_assert(sizeof(MenuHelper80) == 0x14 && sizeof(MenuHelper60) == 0x20, "helper layouts");
MenuHelper80::MenuHelper80(void (*callback)(SDL_Surface *))
{
    freeFn = callback;
    mutex = SDL_CreateMutex();
}
MenuHelper80::~MenuHelper80()
{
    clear();
    SDL_DestroyMutex(mutex);
}
void MenuHelper80::clear()
{
    if (freeFn) {
        SDL_mutexP(mutex);
        for (auto it = entries.begin(); it != entries.end(); it++) freeFn((SDL_Surface *)it->value);
        SDL_mutexV(mutex);
    }
    entries.clear();
}
int MenuHelper80::put(std::string key, void *value)
{
    CacheEntry entry;
    SDL_mutexP(mutex);
    int size = entries.size();
    while (size > 1000) {
        entry = entries.back();
        entries.pop_back();
        if (freeFn) freeFn((SDL_Surface *)entry.value);
        size = entries.size();
    }
    entry.key = key;
    entry.value = value;
    entries.push_front(entry);
    SDL_mutexV(mutex);
    return 0;
}
void *MenuHelper80::get(std::string key)
{
    CacheEntry entry;
    SDL_mutexP(mutex);
    for (auto it = entries.begin(); it != entries.end(); it++) {
        if (it->key.compare(key) == 0) {
            entry.key = key;
            entry.value = it->value;
            entries.erase(it);
            entries.push_front(entry);
            SDL_mutexV(mutex);
            return entry.value;
        }
    }
    SDL_mutexV(mutex);
    return NULL;
}
unsigned int MenuHelper80::count()
{
    return entries.size();
}
int workerThread(void *data)
{
    MenuHelper60 *queue = (MenuHelper60 *)data;
    BGWorker *worker = NULL;
    if (queue) {
        while (true) {
            SDL_mutexP(queue->mutex);
            if (!queue->running) { SDL_mutexV(queue->mutex); break; }
            if (queue->workers.size() == 0) {
                SDL_CondWait(queue->condition, queue->mutex);
                SDL_mutexV(queue->mutex);
                continue;
            } else {
                SDL_mutexV(queue->mutex);
                worker = queue->takeWorker();
                if (worker) {
                    worker->run();
                    worker->finished();
                    delete worker;
                }
            }
        }
    }
    return 0;
}
MenuHelper60::MenuHelper60(int n)
{
    running = true;
    mutex = SDL_CreateMutex();
    condition = SDL_CreateCond();
    thread = SDL_CreateThread(workerThread, this);
    limit = n;
    if (limit < 1) limit = 5;
}
MenuHelper60::~MenuHelper60()
{
    SDL_mutexP(mutex);
    running = false;
    SDL_CondSignal(condition);
    SDL_mutexV(mutex);
    SDL_WaitThread(thread, NULL);
    SDL_DestroyMutex(mutex);
    SDL_DestroyCond(condition);
    for (auto it = workers.begin(); it != workers.end(); it++) delete *it;
}
int MenuHelper60::addWorker(BGWorker *worker)
{
    SDL_mutexP(mutex);
    if (workers.size() >= limit) {
        BGWorker *old = workers.back();
        delete old;
        workers.pop_back();
    }
    workers.push_front(worker);
    SDL_CondSignal(condition);
    SDL_mutexV(mutex);
    return 0;
}
BGWorker *MenuHelper60::takeWorker()
{
    BGWorker *worker = NULL;
    SDL_mutexP(mutex);
    if (workers.size() != 0) {
        worker = workers.front();
        workers.pop_front();
    }
    SDL_mutexV(mutex);
    return worker;
}
