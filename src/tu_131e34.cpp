// tu_131e34 (0x131e34-0x134080): LruCache and WorkerQueue (declared in MenuHelpers.h).
//
// Menus load thumbnails and previews in the background: they queue a job on a WorkerQueue,
// whose thread runs it, and the job stores the image in an LruCache keyed by its path, where
// drawing code looks it up. Body 2,348 bytes plus 110 generated functions.
#include "MenuHelpers.h"
#include "SdlApi.h"
static_assert(sizeof(LruCache) == 0x14 && sizeof(WorkerQueue) == 0x20, "helper layouts");
LruCache::LruCache(void (*callback)(SDL_Surface *))
{
    freeFn = callback;
    mutex = SDL_CreateMutex();
}
LruCache::~LruCache()
{
    clear();
    SDL_DestroyMutex(mutex);
}
// Frees every value (with freeFn, if set) and empties the cache.
// Original quirk, kept as-is: the list itself is cleared outside the lock.
void LruCache::clear()
{
    if (freeFn) {
        SDL_mutexP(mutex);
        for (auto it = entries.begin(); it != entries.end(); it++) freeFn((SDL_Surface *)it->value);
        SDL_mutexV(mutex);
    }
    entries.clear();
}
// Adds an entry at the front, first freeing the oldest while there are more than 1000.
// Does not check for an existing entry with the same key.
int LruCache::put(std::string key, void *value)
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
// The value for key (moving its entry to the front), or NULL.
void *LruCache::get(std::string key)
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
unsigned int LruCache::count()
{
    return entries.size();
}
// The worker thread: sleeps until a job is queued, runs it (run, finished, delete) and repeats
// until the queue is destroyed.
int workerThread(void *data)
{
    WorkerQueue *queue = (WorkerQueue *)data;
    BGWorker *worker = NULL;
    if (queue) {
        while (true) {
            SDL_mutexP(queue->mutex);
            if (!queue->running) {
                SDL_mutexV(queue->mutex);
                break;
            }
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
// Starts the worker thread; n is the most waiting jobs to keep (5 if n < 1).
WorkerQueue::WorkerQueue(int n)
{
    running = true;
    mutex = SDL_CreateMutex();
    condition = SDL_CreateCond();
    thread = SDL_CreateThread(workerThread, this);
    limit = n;
    if (limit < 1) limit = 5;
}
// Stops the thread (after its current job) and deletes the jobs still waiting.
WorkerQueue::~WorkerQueue()
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
// Queues a job at the front; if the queue is full, the oldest waiting job is deleted unrun.
int WorkerQueue::addWorker(BGWorker *worker)
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
// Takes the newest waiting job, or NULL.
BGWorker *WorkerQueue::takeWorker()
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
