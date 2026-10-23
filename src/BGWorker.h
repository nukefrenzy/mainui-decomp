// A background job for WorkerQueue (MenuHelpers.h): run() does the work on the worker thread,
// finished() follows on the same thread, then the queue deletes the job.
// Inline ctor/destructor order is shared by the UI and worker-queue units.
#pragma once
class BGWorker {
public:
    BGWorker() {}
    virtual ~BGWorker() {}
    virtual void run() = 0;         // slot 2, on the worker thread
    virtual void finished() = 0;    // slot 3
};
