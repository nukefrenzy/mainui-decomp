// tu_015b20 (0x15b20-0x49c3c), the first and largest file. The part files below are included
// textually in address order, so the whole compiles as one translation unit (docs/plan-tu_015b20.md).
//
// Header/inline definition order reproduces the 798-section tail; see the integration notes.
// <string> supplies the first char_traits helpers; <iostream> follows the UI declarations so
// its system-error string helpers do not precede the ListMenu/TextMenu vector instantiations.
// The static init (0x39228) constructs std::ios_base::Init and then the pending window queue.
#include <string>
#include <atomic>
#include <vector>
#include "tu_015b20.h"
#include <iostream>

// 0x17ff3c, the only global with a constructor (ctor 0x3b130, dtor 0x49be0 via __aeabi_atexit).
// Declared after the headers and before the body to preserve template first-use order.
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
