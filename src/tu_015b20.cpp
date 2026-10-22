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
std::vector<TrimusWindow *> *g_windowStack; // 0x17ff38
std::vector<TrimusWindow *> g_pendingWindows;

// Recovered POD storage. Original addresses describe the target, not this partial link.
unsigned char g_deviceId[5]; // 0x17ff48: consumed extent; original full allocation size unknown
char g_launchCmd[2048]; // 0x17ff54
char g_countBuffer[256]; // 0x180754
SDL_Thread *g_loadingThread; // 0x180854
// These TU-local objects follow iostream's local .comm initializer; making them
// public moved them ahead of it. Original target addresses are listed below.
static fb_var_screeninfo g_fbVar; // 0x18085c
static bool g_settingsDirty; // 0x1808fc
static int g_powerDownCount; // 0x180900
static bool g_powerSent; // 0x180904
static bool g_startHeld; // 0x180905
static bool g_selectHeld; // 0x180906
static bool g_statusIndicatorFlag; // 0x180907
static char g_cwdBuf[4096]; // 0x180908
static volatile int g_busy; // 0x181908
static volatile int g_progressCur; // 0x18190c
static volatile int g_progressTotal; // 0x181910
static unsigned char g_charging; // 0x181914
int g_fbFd = -1; // 0x17e488


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
