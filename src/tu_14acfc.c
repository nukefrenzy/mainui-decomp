// tu_14acfc: device helpers: tinymix volume, backlight PWM, power key, AXP battery, USB sound
// card hotplug (0x14acfc-0x14b860). Ends before setupKeymap's second copy of "w" (0x163bac);
// the exact cut inside 0x14b198-0x14bb64 is by subsystem.
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
// Onion patches: the PWM period write in initBacklightPwm is a NOP, and the duty_cycle path
// is "/dev/null" (the strings below are the patched ones).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
cJSON *cJSON_Parse(const char *value);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
int cJSON_IsNumber(const cJSON *item);
void cJSON_Delete(cJSON *c);

typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_cond SDL_cond;
SDL_mutex *SDL_CreateMutex(void);
int SDL_mutexP(SDL_mutex *mutex);
int SDL_mutexV(SDL_mutex *mutex);
void SDL_DestroyMutex(SDL_mutex *mutex);
SDL_cond *SDL_CreateCond(void);
int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, unsigned int ms);
void SDL_DestroyCond(SDL_cond *cond);

// File-local statics go to .bss in declaration order (GCC 8 C, -O0); this is the original order.
static char g_pwrKeyBuf[64];    // 0x1824b8
static char g_axpBuf[64];       // 0x1824f8
static char g_tinymixCmd[64];   // 0x182538

struct FdList {                 // 0x182580
    int fd[12];
    int count;
};
static SDL_mutex *g_hotplugMutex;   // 0x182578
static SDL_cond *g_hotplugCond;     // 0x18257c
static struct FdList g_fds;         // 0x182580
static int g_hotplugRunning;        // 0x1825b4

int stub_4acfc()                // 0x14acfc
{
    return 0;
}

int stub_4ad18()                // 0x14ad18
{
    return 0;
}

int stub_4ad34()                // 0x14ad34
{
    return 0;
}

int setVolumeTinymix(int vol)   // 0x14ad50
{
    if (vol < 0)
        vol = 0;
    if (vol > 20)
        vol = 20;
    snprintf(g_tinymixCmd, 64, "tinymix set 6 %d", vol * 63 / 20 + 40);
    system(g_tinymixCmd);
    return 0;
}

int stub_4ade8()                // 0x14ade8
{
    return 2;
}

int writeFile(const char *path, const char *text)   // 0x14ae04
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        printf("Cannot open file: %s\n", path);
        return -1;
    }
    fputs(text, f);
    fclose(f);
    return 0;
}

// Original PWM literal slots. The patched duty-cycle path keeps its 40-byte allocation.
const char g_pwmZero[2] __attribute__((aligned(4))) = "0";
const char g_pwmExportPath[31] __attribute__((aligned(4))) = "/sys/class/pwm/pwmchip0/export";
const char g_pwmPeriod[4] __attribute__((aligned(4))) = "800";
const char g_pwmPeriodPath[36] __attribute__((aligned(4))) = "/sys/class/pwm/pwmchip0/pwm0/period";
const char g_pwmDuty[4] __attribute__((aligned(4))) = "100";
const char g_pwmDutyPath[40] __attribute__((aligned(4))) = "/dev/null";
const char g_pwmDisabledEnable[1] __attribute__((aligned(4))) = "";
const char g_pwmEnablePath[36] __attribute__((aligned(4))) = "/sys/class/pwm/pwmchip0/pwm0/enable";

// Original 0x14aea0 disables the PWM-period write. Preserve argument loads and
// the literal NOP, with no call relocation, as for the patched menu/drawing calls.
__asm__(
    ".set .Linit_pwm_nops, 0\n"
    ".macro bl target\n"
    "    .set .Lskip_patched_call, 0\n"
    "    .irp patch, 0x2c\n"
    "        .if (. - initBacklightPwm) == \\patch\n"
    "            .set .Lskip_patched_call, 1\n"
    "        .endif\n"
    "    .endr\n"
    "    .if .Lskip_patched_call\n"
    "        .inst 0xe320f000\n"
    "        .set .Linit_pwm_nops, .Linit_pwm_nops + 1\n"
    "    .else\n"
    "        .inst 0xebfffffe\n"
    "        .reloc . - 4, R_ARM_CALL, \\target\n"
    "    .endif\n"
    ".endm\n"
);
void initBacklightPwm()         // 0x14ae74
{
    writeFile(g_pwmExportPath, g_pwmZero);
    writeFile(g_pwmPeriodPath, g_pwmPeriod);
    writeFile(g_pwmDutyPath, g_pwmDuty);
    writeFile(g_pwmEnablePath, g_pwmDisabledEnable);
}

__asm__(
    ".purgem bl\n"
    ".if .Linit_pwm_nops != 1\n"
    "    .error \"target NOP patch count changed\"\n"
    ".endif\n"
);

int setBacklight(int level)     // 0x14aed4
{
    char buf[32] = {0};
    if (level < 0)
        level = 0;
    if (level > 10)
        level = 10;
    int duty = 0;
    switch (level) {
    case 0: duty = 6; break;
    case 1: duty = 10; break;
    case 2: duty = 20; break;
    case 3: duty = 30; break;
    case 4: duty = 40; break;
    case 5: duty = 50; break;
    case 6: duty = 60; break;
    case 7: duty = 70; break;
    case 8: duty = 80; break;
    case 9: duty = 90; break;
    default: duty = 100; break;
    }
    snprintf(buf, 32, "%d", duty);
    writeFile(g_pwmDutyPath, buf);
    return 0;
}

int backlightOff()              // 0x14b024
{
    writeFile(g_pwmDutyPath, g_pwmZero);
    return 0;
}

bool isBacklightOff()           // 0x14b04c
{
    char buf[32] = "1";
    FILE *f = fopen(g_pwmDutyPath, "r");
    if (f == NULL) {
        printf("Cannot open file: %s\n", g_pwmDutyPath);
        return false;
    }
    fread(buf, 1, 32, f);
    fclose(f);
    return atoi(buf) == 0;
}

bool isPowerKeyReleased()       // 0x14b0fc
{
    FILE *f = fopen("/sys/devices/soc0/soc/soc:gpio_keys/pwrkeyval", "r");
    if (f) {
        fgets(g_pwrKeyBuf, 64, f);
        int v = atoi(g_pwrKeyBuf);
        printf("read from pwr key file: %d, %s\n", v, g_pwrKeyBuf);
        fclose(f);
        return v != 0;
    }
    return true;
}

int readAxpStatus(int *battery, int *voltage, int *charging)   // 0x14b198
{
    int haveBattery = 0;
    int haveVoltage = 0;
    int haveCharging = 0;
    system("/customer/app/axp_test > /tmp/.axp_result");
    FILE *f = fopen("/tmp/.axp_result", "r");
    if (f) {
        cJSON *root = NULL;
        cJSON *item = NULL;
        if (fgets(g_axpBuf, 64, f)) {
            root = cJSON_Parse(g_axpBuf);
            if (root) {
                item = cJSON_GetObjectItem(root, "battery");
                if (item && cJSON_IsNumber(item)) {
                    *battery = item->valueint;
                    haveBattery = 1;
                }
                item = cJSON_GetObjectItem(root, "voltage");
                if (item && cJSON_IsNumber(item)) {
                    *voltage = item->valueint;
                    haveVoltage = 1;
                }
                item = cJSON_GetObjectItem(root, "charging");
                if (item && cJSON_IsNumber(item)) {
                    *charging = item->valueint;
                    haveCharging = 1;
                }
                cJSON_Delete(root);
            }
        }
        fclose(f);
    }
    return haveBattery && haveVoltage && haveCharging;
}

int fdList_find(int fd)         // 0x14b374
{
    for (int i = 0; i < g_fds.count; i++)
        if (fd == g_fds.fd[i])
            return i;
    return -1;
}

int fdList_add(int fd)          // 0x14b3ec
{
    if (g_fds.count < 12) {
        g_fds.fd[g_fds.count] = fd;
        g_fds.count++;
        return 0;
    }
    return -1;
}

int fdList_remove(int fd)       // 0x14b464
{
    int i = fdList_find(fd);
    if (i >= 0) {
        if (i + 1 < g_fds.count) {
            for (; i + 1 < g_fds.count; i++)
                g_fds.fd[i] = g_fds.fd[i + 1];
        } else {
            g_fds.fd[i] = 0;
        }
        g_fds.count--;
    }
    return 0;
}

int hasDsp1(int card)          // 0x14b53c
{
    return card == 2 && access("/dev/dsp1", R_OK) == 0;
}

// Polls for the USB sound card (/dev/dsp1) every 100 ms and reports plug/unplug to cb.
int hotplugLoop(void (*cb)(int, int))   // 0x14b58c
{
    int ret;
    g_fds.count = 0;
    memset(&g_fds, 0, 48);
    g_hotplugMutex = SDL_CreateMutex();
    if (g_hotplugMutex == NULL) {
        ret = -1;
        goto done;
    }
    g_hotplugCond = SDL_CreateCond();
    if (g_hotplugCond == NULL) {
        ret = -1;
        goto done;
    }
    SDL_mutexP(g_hotplugMutex);
    g_hotplugRunning = 1;
    SDL_mutexV(g_hotplugMutex);
    if (hasDsp1(2))
        fdList_add(2);
    while (1) {
        SDL_mutexP(g_hotplugMutex);
        if (!g_hotplugRunning) {
            SDL_mutexV(g_hotplugMutex);
            break;
        }
        SDL_CondWaitTimeout(g_hotplugCond, g_hotplugMutex, 100);
        if (!g_hotplugRunning) {
            SDL_mutexV(g_hotplugMutex);
            break;
        }
        SDL_mutexV(g_hotplugMutex);
        if (hasDsp1(2)) {
            if (fdList_find(2) < 0) {
                cb(2, 1);
                fdList_add(2);
            }
        } else {
            if (fdList_find(2) >= 0) {
                cb(2, 0);
                fdList_remove(2);
            }
        }
    }
done:
    if (g_hotplugMutex)
        SDL_DestroyMutex(g_hotplugMutex);
    if (g_hotplugCond)
        SDL_DestroyCond(g_hotplugCond);
    g_hotplugMutex = NULL;
    g_hotplugCond = NULL;
    return ret;
}

int stopHotplug()               // 0x14b814
{
    SDL_mutexP(g_hotplugMutex);
    g_hotplugRunning = 0;
    SDL_mutexV(g_hotplugMutex);
    return 1;
}
