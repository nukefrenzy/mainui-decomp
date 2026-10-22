// Main program and status/hotplug callbacks, in binary address order.
// Proven 0x049c3c-0x04c418: 4,740 body bytes (10 written functions and an init pair),
// followed by 93 inline/template functions (5,464 bytes).
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
static_assert(sizeof(RecentItem)==100 && sizeof(AppInfo)==128, "configuration records");
static_assert(sizeof(RomListPosition)==36, "ROM position record");
struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
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
    int isPowerKeyReleased();
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
    int getWifiConnected();
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
    int stub_4ad18();
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
int g_statusThreadRun;
SDL_mutex*g_statusMutex;
SDL_cond*g_statusCond;
SDL_Surface*g_screen,*g_backBuffer;
std::list<RecentItem> g_recentGames;
std::vector<AppInfo> g_appFolder;
std::vector<EmuInfo> g_romSourcesA,g_romSourcesB;
std::vector<Theme> g_themeList;
char g_keyShm[8];
char*g_themePath;
Theme g_skin;
std::vector<RomListPosition> g_romListPositions;
std::atomic<bool> g_running;
int g_lastBattery;
unsigned long g_lastVolumeTime;
unsigned char g_unrefBss_181b18[96]; // 0x181b18: unreferenced storage at this address in the Onion and stock builds; size and position proven, name, type and partition unknown
int g_fontSize=24;
bool g_audioRun=true;
int g_lastPlug=-1;
unsigned g_lastWifiState=-1;
int g_lastReportedWifiStatus=-1;
static int g_loggedWifi;
void onSIGTERM(int signal) {
    g_running=false;
}

void appExit(int code) {
    putenv((char*)"trimui_show=no");
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    g_audioRun=0;
    if (code)exit(code);
}

void pollStatusA_pushEvent() {
    int battery=GetKeyShm(g_keyShm,7);
    MainEvent event;
    int bat,voltage,plug;
    readAxpStatus(&bat,&voltage,&plug);
    plug=plug==2||plug==3;
    if (battery!=g_lastBattery||g_lastPlug!=plug) {
        if (battery>0)g_lastBattery=battery;
        g_lastPlug=plug;
        event.type=24;
        event.user.code=18;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
    if (g_lastPlug!=plug&&plug==0) {
        if (isBacklightOff())setBacklight(GetKeyShm(g_keyShm,1));
    }
}

void pollStatusB_pushEvent() {
    bool changed=false;
    int state=wifiMode();
    changed=g_lastWifiState!=state;
    if (state==2&&!changed) {
        WifiStatus*status=getWifiStatus();
        if (status)changed=status->state!=g_lastReportedWifiStatus;
    }
    if (changed) {
        g_lastWifiState=state;
        MainEvent event;
        event.type=24;
        event.user.code=31;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
}

void pollVolume_pushEvent() {
    unsigned long now=milliseconds();
    if (GetKeyShm(g_keyShm,4)!=0) {
        g_lastVolumeTime=now;
        printf("volume change %ld\n",g_lastVolumeTime);
        SetKeyShm(g_keyShm,4,0);
        MainEvent event;
        event.type=24;
        event.user.code=40;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    } else if (now-g_lastVolumeTime>3000) {
        MainEvent event;
        event.type=24;
        event.user.code=41;
        if (SDL_PushEvent(&event)<0)puts("Failed to push SDL event");
    }
}

int statusPollThread(void*arg) {
    int count=0; // Retain the unused counter: each poll increments it at -O0.
    while (g_statusThreadRun==1) {
        SDL_mutexP(g_statusMutex);
        SDL_CondWaitTimeout(g_statusCond,g_statusMutex,1000);
        SDL_mutexV(g_statusMutex);
        if (g_statusThreadRun) {
            pollStatusA_pushEvent();
            pollStatusB_pushEvent();
            pollVolume_pushEvent();
            count++;
        }
    }
    return 0;
}

void pushHotplugEvent(int type,int unused) {
    MainEvent event;
    event.type=24;
    event.user.code=19;
    if (type==2) {
        if (SDL_PushEvent(&event)<0)puts("Failed to push hotplug SDL event");
    }
}

int hotplugThread(void*arg) {
    return hotplugLoop(reinterpret_cast<void(*)(int,int)>(pushHotplugEvent));
}

void wifiStatusCallback(WifiStatus*status) {
    if (access("/tmp/wifi_verbose",0)==0)printf("status SSID [%s]: IP:[%s] RSSI:%d\n",status->ssid,status->ip,status->rssi);
    if (status->state==4) {
        if (findProcess("udhcpc",0)<0)system("udhcpc -i wlan0 -s /etc/init.d/udhcpc.script &");
    }
    int wifi=getWifiOn(),hotspot=getWifiConnected();
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

int main(int argc,char**argv) {
    printf("%d ui start\n",milliseconds());
    system("echo ondemand > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    system("touch /tmp/stay_awake");
    if (waitForProbe()!=0)return 2;
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
    if (SDL_Init(0x21)<0) {
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
    bool charging=false;
    if (access("/tmp/no_charging_ui",0)!=0) {
        int battery,voltage,plug;
        readAxpStatus(&battery,&voltage,&plug);
        charging=!isPowerKeyReleased()&&(plug==2||plug==3);
        printf("bat=%d,voltage=%d,plugin=%d\n",battery,voltage,plug);
    }
    InitKeyShm(g_keyShm);
    if (charging!=true) {
        int audio=0;
        if (GetKeyShm(g_keyShm,13)!=0)audio=Mix_OpenAudio(48000,0x8010,2,128);
        else audio=Mix_OpenAudio(48000,0x8010,2,4096);
        if (audio<0) {
            printf("Mix_OpenAudio: %s\n",SDL_GetError());
            UninitKeyShm(g_keyShm);
            appExit(2);
        }
        Mix_AllocateChannels(4);
    }
    SDL_ShowCursor(0);
    SDL_EnableKeyRepeat(500,100);
    printf("%d sdl inited\n",milliseconds());
    g_screen=SDL_SetVideoMode(640,480,32,0x40000001);
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
    SetKeyShm(g_keyShm,4,0);
    loadThemeFolder(&g_themeList,"/mnt/SDCARD/Themes");
    loadSystemJson(g_keyShm,&g_themePath,&g_fontSize);
    setDefaultTheme(&g_skin);
    setBacklight(GetKeyShm(g_keyShm,1));
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
    loadEmuFolder(&g_romSourcesA,"/mnt/SDCARD/Emu");
    loadAppFolder(&g_appFolder,"/mnt/SDCARD/App");
    loadEmuFolder(&g_romSourcesB,"/mnt/SDCARD/RApp");
    initAudioSettings(g_keyShm);
    ensureSwap(0);
    stopBackgroundThread();
    printf("%d try to restore\n",milliseconds());
    if (restoreWindowState(&windows)<1) {
        printf("%d nothing to restore\n",milliseconds());
        createRootWindow(&windows);
    }
    loadRomListPositions(&g_romListPositions);
    printf("%d run windows\n",milliseconds());
    unlink("/tmp/stay_awake");
    runWindowLoop(&windows,&g_running);
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
    stub_4ad18();
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
