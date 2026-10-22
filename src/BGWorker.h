// Worker base; inline ctor/destructor order is shared by the UI and worker-queue units.
#pragma once
class BGWorker {
public:
    BGWorker() {}
    virtual ~BGWorker() {}
    virtual void run() = 0;         // slot 2, on the worker thread
    virtual void finished() = 0;    // slot 3
};
