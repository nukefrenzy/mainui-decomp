// tu_015b20 (0x15b20-0x49c3c), the first and largest file. The part files below are included
// textually in address order, so the whole compiles as one translation unit (docs/plan-tu_015b20.md).
//
// Include order decides the tail's inline-method order; the tail starts with <atomic> helpers
// (std::operator&, atomic<bool>::load), then a small polymorphic class, then TrimusWindow's inline
// methods. See the plan for the full sequence. The static init (0x39228) constructs
// std::ios_base::Init (so <iostream> is included) and then the window queue below.
#include <iostream>
#include <atomic>
#include <vector>
#include "tu_015b20.h"

// 0x17ff3c, the only global with a constructor (ctor 0x3b130, dtor 0x49be0 via __aeabi_atexit).
// Its exact position among the functions is not known yet; it decides where the vector templates
// land in the tail.
std::vector<TrimusWindow *> g_pendingWindows;

#include "tu_015b20/p01.inc"
#include "tu_015b20/p02.inc"
#include "tu_015b20/p03.inc"
#include "tu_015b20/p04.inc"
#include "tu_015b20/p05.inc"
#include "tu_015b20/p06.inc"
#include "tu_015b20/p07.inc"
#include "tu_015b20/p08.inc"
#include "tu_015b20/p09.inc"
#include "tu_015b20/p10.inc"
#include "tu_015b20/p11.inc"
