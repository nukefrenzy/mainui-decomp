// tu_049c3c (0x049c3c-0x04c418): MainUI's main program and its two background threads.
//
// main initializes the device and SDL, loads settings, theme, language and the Emu/App/RApp
// folders, restores the windows from the last run (or creates the root menu) and runs the window
// loop until a launch or SIGTERM ends it. Meanwhile:
// - the status thread wakes every second and posts user events (Input.h) when the battery or
//   charger state, the Wi-Fi state or the volume changes, so the status bar can redraw;
// - the hotplug thread posts EVENT_HOTPLUG when a second sound card comes or goes;
// - the Wi-Fi monitor (tu_14e0cc.c) calls wifiStatusCallback to start DHCP when needed.
// Body 4,740 bytes (10 functions and an init pair) plus 93 inline/template functions.
#include "PlatformApi.h"
#include <atomic>
#include <list>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include "ConfigTypes.h"
#include "Theme.h"
#include "MultiDBCachedTextMenu.h"
#include "WifiMenus.h"
#include "Input.h"
#include "ShmVar.h"
static_assert(sizeof(RecentItem)==100 && sizeof(AppInfo)==128, "configuration records");
static_assert(sizeof(RomListPosition)==36, "ROM position record");
struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
// SDL 1.2 SDL_Event as MainUI posts it (SDL_USEREVENT with a code).
union MainEvent {
    unsigned char type;
    struct {
        unsigned char type;
        int code;
        void *data1,*data2;
    }
    user;
    char storage[20];
};
extern "C" {
    void Mix_CloseAudio();
    void TTF_Quit();
    void SDL_Quit();
    int GetKeyShm(void*,int);
    int SetKeyShm(void*,int,int);
    int readAxpStatus(int*,int*,int*);
    int isBacklightOff();
    int bootedByPowerKey();
    int setBacklight(int);
    int hasDsp1(int);
    int hotplugLoop(void(*)(int,int));
    int stopHotplug();
    int SDL_PushEvent(void*);
    int SDL_mutexP(SDL_mutex*);
    int SDL_mutexV(SDL_mutex*);
    int SDL_CondWaitTimeout(SDL_cond*,SDL_mutex*,unsigned);
    int SDL_CondSignal(SDL_cond*);
    SDL_mutex*SDL_CreateMutex();
    SDL_cond*SDL_CreateCond();
    void SDL_DestroyMutex(SDL_mutex*);
    void SDL_DestroyCond(SDL_cond*);
    SDL_Thread*SDL_CreateThread(int(*)(void*),void*);
    void SDL_WaitThread(SDL_Thread*,int*);
    void startWifiMonitor(void(*)(WifiStatus*));
    void stopWifiMonitor();
    int getWifiOn();
    int getHotspotOn();
    WifiStatus*getWifiStatus();
    int SDL_Init(unsigned);
    const char*SDL_GetError();
    int TTF_Init();
    void InitKeyShm(void*);
    void UninitKeyShm(void*);
    int Mix_OpenAudio(int,unsigned short,int,int);
    int Mix_AllocateChannels(int);
    int SDL_ShowCursor(int);
    int SDL_EnableKeyRepeat(int,int);
    SDL_Surface*SDL_SetVideoMode(int,int,int,unsigned);
    int SDL_Flip(SDL_Surface*);
    SDL_Surface*SDL_CreateRGBSurface(unsigned,int,int,int,unsigned,unsigned,unsigned,unsigned);
    void SDL_FreeSurface(SDL_Surface*);
    void loadSystemJson(void*,char**,int*);
    void initBacklightPwm();
    int exitDeviceHook();
    void encrypter_init();
    void encrypter_deinit();
}

unsigned long milliseconds();
bool moduleLoaded(const char*);
int findProcess(const char*,int);
int wifiMode();
void swicth2ApMode();
int loadThemeFolder(std::vector<Theme>*,const char*);
Theme*findTheme(std::vector<Theme>*,const char*);
void loadLanguages(const char*);
void initFramebuffer();
void chargingScreenLoop();
void startBackgroundThread();
void ensureSwap(void*);
void stopBackgroundThread();
void loadPinyinTable();
int loadRecentList(std::list<RecentItem>*);
int saveRecentList(std::list<RecentItem>*);
int loadEmuFolder(std::vector<EmuInfo>*,const char*);
int loadAppFolder(std::vector<AppInfo>*,const char*);
void initAudioSettings(void*);
int restoreWindowState(std::vector<TrimusWindow*>*);
void createRootWindow(std::vector<TrimusWindow*>*);
int loadRomListPositions(std::vector<RomListPosition>*);
void runWindowLoop(std::vector<TrimusWindow*>*,std::atomic<bool>*);
void clearWindows(std::vector<TrimusWindow*>*);
void closeFramebuffer();
unsigned char waitForProbe();
int g_statusThreadRun;          // 1 while the status thread should keep polling
SDL_mutex*g_statusMutex;        // with g_statusCond: the status thread's 1 s wait, cut short at exit
SDL_cond*g_statusCond;
SDL_Surface*g_screen,*g_backBuffer;
std::list<RecentItem> g_recentGames;
std::vector<AppInfo> g_appFolder;
std::vector<EmuInfo> g_romSourcesA,g_romSourcesB;
std::vector<Theme> g_themeList;
char g_keyShm[8];               // libshmvar handle for the settings shared with keymon (ShmVar.h)
char*g_themePath;
Theme g_skin;
std::vector<RomListPosition> g_romListPositions;
std::atomic<bool> g_running;    // the window loop runs while true; SIGTERM clears it
int g_lastBattery;              // last battery level reported (status thread)
unsigned long g_lastVolumeTime; // time of the last volume change, in ms
unsigned char g_unrefBss_181b18[96]; // 0x181b18: unreferenced storage at this address in the Onion and stock builds; size and position proven, name, type and partition unknown
int g_fontSize=24;
bool g_audioRun=true;
int g_lastChargerConnected=-1;  // last charger state reported (-1 = none yet)
unsigned g_lastWifiState=-1;    // last wifiMode() reported
int g_lastReportedWifiStatus=-1;
static int g_loggedWifi;        // the Wi-Fi flags have been logged once

// SIGTERM: leave the window loop and exit cleanly.
void onSIGTERM(int signal) {
    g_running=false;
}

// Shuts down audio, fonts and SDL; exits the process when code is nonzero.
void appExit(int code) {
    putenv((char*)"trimui_show=no");
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    g_audioRun=0;
    if (code)exit(code);
}

// Posts EVENT_BATTERY when the battery level (from keymon) or the charger state changes.
// The second check can never fire: g_lastChargerConnected was just set to `plug`. (If it could,
// unplugging the charger would turn a darkened backlight back on.)
void pollBatteryStatus() {
    int battery=GetKeyShm(g_keyShm,MONITOR_ADC_VALUE);
    MainEvent event;
    int bat,voltage,plug;
    readAxpStatus(&bat,&voltage,&plug);
    plug=plug==2||plug==3;   // charger connected
    if (battery!=g_lastBattery||g_lastChargerConnected!=plug) {
        if (battery>0)g_lastBattery=battery;
        g_lastChargerConnected=plug;
        event.type=SDL_USEREVENT;
        event.user.code=EVENT_BATTERY;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
    if (g_lastChargerConnected!=plug&&plug==0) {
        if (isBacklightOff())setBacklight(GetKeyShm(g_keyShm,MONITOR_BRIGHTNESS));
    }
}

// Posts EVENT_WIFI_STATUS when the Wi-Fi mode changes or, in station mode, when the connection state
// differs from g_lastReportedWifiStatus. (Nothing ever updates that variable, so in mode 2 the
// event repeats every second.)
void pollWifiStatus() {
    bool changed=false;
    int state=wifiMode();
    changed=g_lastWifiState!=state;
    if (state==WIFI_MODE_STATION&&!changed) {
        WifiStatus*status=getWifiStatus();
        if (status)changed=status->state!=g_lastReportedWifiStatus;
    }
    if (changed) {
        g_lastWifiState=state;
        MainEvent event;
        event.type=SDL_USEREVENT;
        event.user.code=EVENT_WIFI_STATUS;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
}

// keymon flags volume-key use in shared memory: post EVENT_VOLUME_SHOWN (and clear the flag).
// More than 3 s after the last change, post EVENT_VOLUME_HIDDEN (every second).
void pollVolumeChange() {
    unsigned long now=milliseconds();
    if (GetKeyShm(g_keyShm,MONITOR_VOLUME_CHANGED)!=0) {
        g_lastVolumeTime=now;
        printf("volume change %ld\n",g_lastVolumeTime);
        SetKeyShm(g_keyShm,MONITOR_VOLUME_CHANGED,0);
        MainEvent event;
        event.type=SDL_USEREVENT;
        event.user.code=EVENT_VOLUME_SHOWN;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    } else if (now-g_lastVolumeTime>3000) {
        MainEvent event;
        event.type=SDL_USEREVENT;
        event.user.code=EVENT_VOLUME_HIDDEN;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
}

// Status thread: every second (or at once when main signals exit), poll the three states above.
int statusPollThread(void*arg) {
    int count=0; // Retain the unused counter: each poll increments it at -O0.
    while (g_statusThreadRun==1) {
        SDL_mutexP(g_statusMutex);
        SDL_CondWaitTimeout(g_statusCond,g_statusMutex,1000);
        SDL_mutexV(g_statusMutex);
        if (g_statusThreadRun) {
            pollBatteryStatus();
            pollWifiStatus();
            pollVolumeChange();
            count++;
        }
    }
    return 0;
}

// hotplugLoop callback: posts EVENT_HOTPLUG when sound card 2 (/dev/dsp1) is plugged or unplugged.
void pushHotplugEvent(int card,int plugged) {
    MainEvent event;
    event.type=SDL_USEREVENT;
    event.user.code=EVENT_HOTPLUG;
    if (card==2) {
        if (SDL_PushEvent(&event)<0)puts("Failed to push hotplug SDL event");
    }
}

// Hotplug thread: runs hotplugLoop (tu_14acfc.c) until stopHotplug.
int hotplugThread(void*arg) {
    return hotplugLoop(reinterpret_cast<void(*)(int,int)>(pushHotplugEvent));
}

// Called by the Wi-Fi monitor every 3 s. Starts the DHCP client once connected (if it is not
// already running), switches to access-point mode when the hotspot is on, and
// saves the Wi-Fi MAC address to /tmp/wifimac. Touch /tmp/wifi_verbose to log every status.
void wifiStatusCallback(WifiStatus*status) {
    if (access("/tmp/wifi_verbose",0)==0)printf("status SSID [%s]: IP:[%s] RSSI:%d\n",status->ssid,status->ip,status->rssi);
    if (status->state==WIFI_STATE_COMPLETED) {
        if (findProcess("udhcpc",0)<0)system("udhcpc -i wlan0 -s /etc/init.d/udhcpc.script &");
    }
    int wifi=getWifiOn(),hotspot=getHotspotOn();
    if (g_loggedWifi==0) {
        printf("wififlag %d, hostapflag %d\n",wifi,hotspot);
        g_loggedWifi=1;
    }
    if (hotspot) {
        if (findProcess("wpa_supplicant",0)>0) {
            swicth2ApMode();
            system("ifconfig | grep HWaddr | sed 's/[ ]\\+/ /g' | cut -d' ' -f5 > /tmp/wifimac");
        }
    } else {
        if (wifi)system("ifconfig | grep HWaddr | sed 's/[ ]\\+/ /g' | cut -d' ' -f5 > /tmp/wifimac");
    }
}

// Start-up order matters: the device check comes first, the charging screen (when the device was
// started by plugging in a charger) runs before any menu, and the threads start before the
// folders are scanned. A launch ends runWindowLoop after writing /tmp/cmd_to_run.sh; the shell
// script around MainUI then runs the command and starts MainUI again.
int main(int argc,char**argv) {
    printf("%d ui start\n",milliseconds());
    system("echo ondemand > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    system("touch /tmp/stay_awake");
    if (waitForProbe()!=0)return 2;   // device authentication (tu_14f730.c)
    // Load the RTL8188FU Wi-Fi driver from the SD card if it is not loaded yet.
    if (moduleLoaded("8188fu.ko")!=true) {
        if (access("/mnt/SDCARD/8188fu.ko",0)==0) {
            int rc=system("insmod /mnt/SDCARD/8188fu.ko");
        }
    }
    g_running=true;
    struct sigaction action;
    memset(&action,0,sizeof action);
    action.sa_handler=onSIGTERM;
    sigaction(SIGTERM,&action,NULL);
    startWifiMonitor(wifiStatusCallback);
    if (SDL_Init(0x21)<0) {   // SDL_INIT_TIMER | SDL_INIT_VIDEO
        fprintf(stderr,"Couldn't initialize SDL: %s\n",SDL_GetError());
        return 2;
    }
    printf("%d ttf inited\n",milliseconds());
    if (TTF_Init()<0) {
        fprintf(stderr,"Couldn't initialize TTF: %s\n",SDL_GetError());
        SDL_Quit();
        return 2;
    }
    putenv((char*)"trimui_show=yes");
    if (hasDsp1(2)>0)putenv((char*)"AUDIODEV=/dev/dsp1");
    else putenv((char*)"AUDIODEV=/dev/dsp");
    // Started by plugging in a charger (not by the power key)? Then only the charging screen runs.
    bool charging=false;
    if (access("/tmp/no_charging_ui",0)!=0) {
        int battery,voltage,plug;
        readAxpStatus(&battery,&voltage,&plug);
        charging=!bootedByPowerKey()&&(plug==2||plug==3);
        printf("bat=%d,voltage=%d,plugin=%d\n",battery,voltage,plug);
    }
    InitKeyShm(g_keyShm);
    if (charging!=true) {
        int audio=0;
        // 48 kHz, AUDIO_S16LSB, stereo; "audiofix" selects a short 128-sample buffer.
        if (GetKeyShm(g_keyShm,MONITOR_AUDIOFIX)!=0)audio=Mix_OpenAudio(48000,0x8010,2,128);
        else audio=Mix_OpenAudio(48000,0x8010,2,4096);
        if (audio<0) {
            printf("Mix_OpenAudio: %s\n",SDL_GetError());
            UninitKeyShm(g_keyShm);
            appExit(2);
        }
        Mix_AllocateChannels(4);
    }
    SDL_ShowCursor(0);
    SDL_EnableKeyRepeat(500,100);   // held buttons repeat after 500 ms, every 100 ms
    printf("%d sdl inited\n",milliseconds());
    g_screen=SDL_SetVideoMode(640,480,32,0x40000001);   // SDL_HWSURFACE | SDL_DOUBLEBUF
    SDL_Flip(g_screen);
    g_backBuffer=SDL_CreateRGBSurface(1,640,480,32,0xff0000,0xff00,0xff,0xff000000);
    if (!g_screen) {
        fprintf(stderr,"Couldn't set video mode in UI: %s\n",SDL_GetError());
        if (g_screen)SDL_FreeSurface(g_screen);
        if (g_backBuffer)SDL_FreeSurface(g_backBuffer);
        UninitKeyShm(g_keyShm);
        appExit(2);
    }
    printf("%d screen inited\n",milliseconds());
    SetKeyShm(g_keyShm,MONITOR_VOLUME_CHANGED,0);
    loadThemeFolder(&g_themeList,"/mnt/SDCARD/Themes");
    loadSystemJson(g_keyShm,&g_themePath,&g_fontSize);
    setDefaultTheme(&g_skin);
    setBacklight(GetKeyShm(g_keyShm,MONITOR_BRIGHTNESS));
    // Use the theme from system.json if it is installed, else the built-in skin in ./.
    bool loaded=false;
    Theme*theme=findTheme(&g_themeList,g_themePath);
    if (!theme) {
        g_themePath=strdup("./");
        loaded=loadTheme(&g_skin,&g_fontSize);
    } else loaded=loadTheme(theme,&g_fontSize);
    if (loaded!=true) {
        fprintf(stderr,"Couldn't initialize skin: %s\n",SDL_GetError());
        freeTheme();
        SDL_FreeSurface(g_screen);
        SDL_FreeSurface(g_backBuffer);
        appExit(2);
    }
    initFramebuffer();
    loadLanguages("./");
    if (charging) {
        printf("%d charging\n",milliseconds());
        initBacklightPwm();
        unlink("/tmp/stay_awake");
        chargingScreenLoop();
    }
    startBackgroundThread();
    // Wi-Fi is switched on in the settings but not running: power it up and start wpa_supplicant.
    if (findProcess("wpa_supplicant",0)<0&&getWifiOn()!=0) {
        puts("start wpa_supplicant in ui");
        int rc=system("/customer/app/axp_test wifion");
        rc=system("sleep 2");
        rc=system("ifconfig wlan0 up");
        rc=system("wpa_supplicant -B -D nl80211 -iwlan0 -c /appconfigs/wpa_supplicant.conf");
        rc=system("touch /tmp/audioserver_restart");
    }
    g_statusMutex=SDL_CreateMutex();
    g_statusCond=SDL_CreateCond();
    g_statusThreadRun=1;
    SDL_Thread*statusThread=SDL_CreateThread(statusPollThread,NULL);
    printf("%d start hotplug_thread\n",milliseconds());
    SDL_Thread*hotplug=SDL_CreateThread(hotplugThread,NULL);
    loadPinyinTable();
    printf("%d dir is %s\n",(int)milliseconds(),argv[1]);
    std::vector<TrimusWindow*>windows;
    loadRecentList(&g_recentGames);
    encrypter_init();
    // Emulator, app and RetroArch-app folders; each entry has its own config.json.
    loadEmuFolder(&g_romSourcesA,"/mnt/SDCARD/Emu");
    loadAppFolder(&g_appFolder,"/mnt/SDCARD/App");
    loadEmuFolder(&g_romSourcesB,"/mnt/SDCARD/RApp");
    initAudioSettings(g_keyShm);
    ensureSwap(0);
    stopBackgroundThread();
    // Reopen the windows saved in /tmp/state.json before the last launch, else the main menu.
    printf("%d try to restore\n",milliseconds());
    if (restoreWindowState(&windows)<1) {
        printf("%d nothing to restore\n",milliseconds());
        createRootWindow(&windows);
    }
    loadRomListPositions(&g_romListPositions);
    printf("%d run windows\n",milliseconds());
    unlink("/tmp/stay_awake");
    runWindowLoop(&windows,&g_running);
    // Shut down: stop both threads, free everything, save the Recent list.
    SDL_mutexP(g_statusMutex);
    g_statusThreadRun=0;
    SDL_CondSignal(g_statusCond);
    SDL_mutexV(g_statusMutex);
    SDL_WaitThread(statusThread,NULL);
    stopHotplug();
    SDL_WaitThread(hotplug,NULL);
    clearWindows(&windows);
    freeTheme();
    SDL_FreeSurface(g_screen);
    SDL_FreeSurface(g_backBuffer);
    exitDeviceHook();
    SDL_DestroyMutex(g_statusMutex);
    SDL_DestroyCond(g_statusCond);
    saveRecentList(&g_recentGames);
    UninitKeyShm(g_keyShm);
    stopWifiMonitor();
    encrypter_deinit();
    closeFramebuffer();
    puts("what happen");
    free(g_themePath);
    appExit(0);
    return 0;
}
