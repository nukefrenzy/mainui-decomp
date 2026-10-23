// Action: what a list row does when chosen. `result` is at +4; execute, progress and finished
// are virtual slots 2-4.
#pragma once
class Action
{
public:
    Action(int result) : result(result) {} // 0x39750
    virtual ~Action() {}  // 0x39794
    virtual int execute(void *param) = 0;
    virtual int progress(int *, int) { return -1; } // 0x397fc, shared by most of tu_015b20's actions
    virtual int finished() { return 0; }              // 0x39828
    int result;
};
