// Shared action ABI: result at +4; execute, progress, finished are slots 2-4.
#pragma once
class Action {
public:
    Action(int result) : result(result) {} // 0x39750
    virtual ~Action() {}  // 0x39794
    virtual int execute(void *param) = 0;
    virtual int progress(int *, int *) = 0;
    virtual int finished() = 0;
    int result;
};
