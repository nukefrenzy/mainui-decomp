// tu_14e0cc: Wi-Fi through wpa_cli (0x14e0cc-0x14f730): connect, scan, status and the monitor
// thread that refreshes them. The scan_results/status parsing reads /tmp files written by wpa_cli.
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include "PlatformApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void printThreadInfo(const char *name)  // 0x14e0cc
{
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    printf("%s pid %u tid %u (0x%x)\n", name, (unsigned int)pid, (unsigned int)tid, (unsigned int)tid);
}

int wifiSignalLevel(int rssi)   // 0x14e118
{
    if (rssi < -126 || rssi > 127)
        return 0;
    if (rssi < -80)
        return 1;
    if (rssi < -70)
        return 2;
    return 3;
}

int readFileTo(const char *path, void *buf)     // 0x14e184
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
        return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    int n = fread(buf, 1, size, f);
    fclose(f);
    return n;
}

// File-local statics go to .bss in declaration order (GCC 8 C, -O0); this is the original order.
struct WifiStatus;
static int g_apCount;               // 0x182644
static int g_wifiBusy;              // 0x182648
static pthread_mutex_t g_apMutex;   // 0x18264c
static int g_wifiScanEnable;        // 0x182664
static int g_wifiConnecting;        // 0x182668
static pthread_t g_monitorThread;   // 0x18266c
static int s_unrefBss_182670;       // 0x182670: unreferenced storage at this address in the Onion and stock builds; size and position proven, name, type and partition unknown
static int g_monitorRunning;        // 0x182674
static void (*g_statusCallback)(struct WifiStatus *); // 0x182678

int setWifiScanEnable(int on)   // 0x14e220
{
    g_wifiScanEnable = on;
    printf("rx_wifi_scan_enable %d\n", on);
}


int wifiConnectOpen(const char *ssid)   // 0x14e260
{
    int i;
    char cmd[128];
    for (i = 0; i < 16; i++) {
        sprintf(cmd, "wpa_cli   remove_network %d", i);
        system(cmd);
    }
    sprintf(cmd, "wpa_cli   add_network %d", 0);
    system(cmd);
    sprintf(cmd, "wpa_cli   set_network 0 ssid \\\"%s\\\"", ssid);
    system(cmd);
    sprintf(cmd, "wpa_cli   set_network 0 key_mgmt NONE");
    system(cmd);
    sprintf(cmd, "wpa_cli   select_network 0");
    system(cmd);
    sprintf(cmd, "wpa_cli   enable_network 0");
    system(cmd);
    system("wpa_cli   save_config");
    sync();
    usleep(1000000);
    g_wifiBusy = 0;
    return 0;
}

int wifiConnectPsk(const char *ssid, const char *psk)  // 0x14e39c
{
    int i;
    char cmd[128];
    for (i = 0; i < 16; i++) {
        sprintf(cmd, "wpa_cli   remove_network %d", i);
        system(cmd);
    }
    sprintf(cmd, "wpa_cli   add_network %d", 0);
    system(cmd);
    sprintf(cmd, "wpa_cli   set_network 0 ssid \\\"%s\\\"", ssid);
    system(cmd);
    sprintf(cmd, "wpa_cli   set_network 0 psk \\\"%s\\\"", psk);
    system(cmd);
    sprintf(cmd, "wpa_cli   select_network 0");
    system(cmd);
    sprintf(cmd, "wpa_cli   enable_network 0");
    system(cmd);
    system("wpa_cli   save_config");
    sync();
    usleep(1000000);
    g_wifiBusy = 0;
    return 0;
}

void runUdhcpc()                // 0x14e4e0
{
    g_wifiBusy = 1;
    usleep(5000000);
    system("udhcpc -i wlan0");
    g_wifiBusy = 0;
}

int wifiStartConnect()          // 0x14e528
{
    g_wifiConnecting = 1;
    g_wifiBusy = 1;
    return 0;
}

// One Wi-Fi network: a wpa_cli scan_results row, or the current connection.
typedef struct WifiAP {         // 0x114 bytes
    char raw0[8];
    char ssid[0x58];            // +0x08
    int rssi;                   // +0x60
    char raw64[8];
    int freq;                   // +0x6c
    char raw70[4];
    char bssid[0x18];           // +0x74
    char flags[0x80];           // +0x8c
    int secured;                // +0x10c
    int current;                // +0x110
} WifiAP;

// The current connection (wpa_cli status), 0x74 bytes.
typedef struct WifiStatus {
    int state;                  // 4 COMPLETED, 0 DISABLED, 1 INACTIVE, 2 SCANNING, 6 4WAY_HANDSHAKE
    char raw4[4];
    char ssid[0x40];            // +0x08
    char ip[0x18];              // +0x48
    int rssi;                   // +0x60
    int linkSpeed;              // +0x64
    int noise;                  // +0x68
    int freq;                   // +0x6c
    int id;                     // +0x70
} WifiStatus;

WifiStatus g_connStatus;        // 0x1889b0, COMMON; name chosen so ld's within-file COMMON (hash) order matches the original; the original name is stripped

WifiStatus *getWifiStatus()     // 0x14e564
{
    return &g_connStatus;
}

unsigned char hexDigit(unsigned char v)     // 0x14e584
{
    if (v <= 9)
        v += '0';
    else if (v > 9 && v <= 15)
        v += 'A' - 10;
    else
        v = 0xff;
    return v;
}

unsigned char hexValue(unsigned char c)     // 0x14e5f8
{
    if (c >= '0' && c <= '9')
        c -= '0';
    else if (c >= 'A' && c <= 'F')
        c -= 'A' - 10;
    else if (c >= 'a' && c <= 'f')
        c -= 'a' - 10;
    else
        c = 0xff;
    return c;
}

// Decodes the \xNN escapes wpa_cli uses for non-ASCII SSIDs, in place.
void unescapeSsid(char *s, int len)     // 0x14e6a0
{
    int i;
    int j;
    int v;
    char buf[128];
    if (strstr(s, "\\x") == NULL)
        return;
    j = 0;
    memset(buf, 0, 128);
    i = 0;
    while (i < len) {
        if (s[i] == '\\' && s[i + 1] == 'x') {
            v = hexValue(s[i + 2]) << 4;
            v += hexValue(s[i + 3]);
            buf[j] = v;
            i += 4;
        } else {
            buf[j] = s[i];
            i++;
        }
        j++;
    }
    strcpy(s, buf);
}

// Splits one scan_results line (bssid, freq, signal, flags, ssid) into ap.
int parseScanLine(const char *text, WifiAP *ap)     // 0x14e824
{
    int field;
    char *tok;
    char line[2048];
    strcpy(line, text);
    field = 0;
    char delim[2] = "\t";
    char *p = line;
    int freq;
    int rssi;
    tok = strsep(&p, delim);
    while (tok) {
        if (tok && *tok) {
            switch (field) {
            case 4:
                strcpy(ap->ssid, tok);
                unescapeSsid(ap->ssid, strlen(ap->ssid));
                if (g_connStatus.ssid[0] && strcmp(ap->ssid, g_connStatus.ssid) == 0)
                    ap->current = 1;
                else
                    ap->current = 0;
                break;
            case 0:
                strcpy(ap->bssid, tok);
                break;
            case 3:
                strcpy(ap->flags, tok);
                if (strstr(ap->flags, "WPA") || strstr(ap->flags, "WEP"))
                    ap->secured = 1;
                else
                    ap->secured = 0;
                break;
            case 1:
                freq = 0;
                sscanf(tok, "%d", &freq);
                ap->freq = freq;
                break;
            case 2:
                rssi = -127;
                sscanf(tok, "%d", &rssi);
                ap->rssi = rssi;
                break;
            }
            field++;
        }
        tok = strsep(&p, delim);
    }
    return field;
}

volatile WifiAP g_aps[32];          // 0x182730, COMMON, filled by the monitor thread

int isScannedSsid(const char *ssid)     // 0x14eacc
{
    int i;
    if (ssid == NULL)
        return 0;
    for (i = 0; i < g_apCount; i++) {
        if (g_aps[i].ssid[0] == 0)
            continue;
        if (strcmp((const char *)g_aps[i].ssid, ssid) == 0)
            return 1;
    }
    return 0;
}

int copyScanResults(WifiAP *out, int max)  // 0x14eb9c
{
    pthread_mutex_lock(&g_apMutex);
    int n = g_apCount;
    if (max < g_apCount)
        n = max;
    memcpy(out, (const void *)g_aps, n * sizeof(WifiAP));
    pthread_mutex_unlock(&g_apMutex);
    return n;
}

char g_scanBuffer[0x4000];         // 0x1849b0, COMMON; name chosen so ld's within-file COMMON (hash) order matches the original; the original name is stripped

WifiStatus queryWifiStatus();   // 0x14ef98

void rx_wifi_do_scan();         // 0x14eeac

// Refreshes the connection status and rebuilds g_aps from /tmp/wpa_scan.txt, dropping
// hidden and duplicate SSIDs.
void parseScanResults()         // 0x14ec28
{
    int count;
    char *tok;
    int n;
    int fields;
    char unused[0x4000];
    g_connStatus = queryWifiStatus();
    printf("parse current wifi [%s]: ip:[%s] rssi:%d\n", g_connStatus.ssid, g_connStatus.ip, g_connStatus.rssi);
    g_apCount = 0;
    memset(g_scanBuffer, 0, 0x4000);
    n = readFileTo("/tmp/wpa_scan.txt", g_scanBuffer);
    if (n <= 0)
        return;
    count = 0;
    fields = 0;
    char delim[3] = "\r\n";
    char *p = g_scanBuffer;
    memset((void *)g_aps, 0, sizeof(g_aps));
    tok = strsep(&p, delim);
    while (tok) {
        if (tok && *tok) {
            fields = parseScanLine(tok, (WifiAP *)&g_aps[count]);
            if (g_aps[count].ssid[0] == 0 || isScannedSsid((const char *)g_aps[count].ssid)) {
                memset((void *)&g_aps[count], 0, sizeof(WifiAP));
            } else {
                count++;
                g_apCount = count;
            }
        }
        tok = strsep(&p, delim);
    }
    g_apCount = count;
}

typedef union SDL_Event {       // SDL 1.2
    unsigned char type;
    struct {
        unsigned char type;
        int code;
        void *data1;
        void *data2;
    } user;
} SDL_Event;
int SDL_PushEvent(SDL_Event *event);
char *SDL_GetError(void);

void rx_wifi_do_scan()          // 0x14eeac
{
    char cmd[128];
    SDL_Event ev;
    sprintf(cmd, "wpa_cli   scan");
    system(cmd);
    usleep(2500000);
    sprintf(cmd, "wpa_cli   scan_result > %s", "/tmp/wpa_scan.txt");
    system(cmd);
    pthread_mutex_lock(&g_apMutex);
    parseScanResults();
    pthread_mutex_unlock(&g_apMutex);
    ev.type = 24;               // SDL_USEREVENT
    ev.user.code = 28;
    printf("rx_wifi_do_scan %d\n", g_apCount);
    if (SDL_PushEvent(&ev) < 0)
        printf("%s %d Failed to push SDL event %s\n", __func__, 437, SDL_GetError());
}

WifiStatus queryWifiStatus()    // 0x14ef98
{
    char *p = NULL;
    int val;
    char word[128];
    char text[4096];
    char cmd[128];
    WifiStatus st;
    memset(&st, 0, sizeof(st));
    memset(text, 0, 4096);
    memset(word, 0, 128);
    sprintf(cmd, "wpa_cli   status > %s", "/tmp/wpa_status.txt");
    system(cmd);
    sprintf(cmd, "wpa_cli   signal_poll >> %s", "/tmp/wpa_status.txt");
    system(cmd);
    readFileTo("/tmp/wpa_status.txt", text);
    p = strstr(text, "wpa_state=");
    if (p && sscanf(p, "wpa_state=%s\n", word) == 1) {
        if (strcmp("COMPLETED", word) == 0)
            st.state = 4;
        else if (strcmp("DISABLED", word) == 0)
            st.state = 0;
        else if (strcmp("INACTIVE", word) == 0)
            st.state = 1;
        else if (strcmp("SCANNING", word) == 0)
            st.state = 2;
        else if (strcmp("4WAY_HANDSHAKE", word) == 0)
            st.state = 6;
    }
    p = strstr(text, "ip_address=");
    if (p && sscanf(p, "ip_address=%s\n", word) == 1)
        strcpy(st.ip, word);
    p = strstr(text, "\nssid=");
    if (p && sscanf(p, "\nssid=%s\n", word) == 1) {
        strcpy(st.ssid, word);
        unescapeSsid(st.ssid, strlen(st.ssid));
    }
    p = strstr(text, "id=");
    if (p && sscanf(p, "id=%d\n", &val) == 1)
        st.id = val;
    p = strstr(text, "RSSI=");
    if (p && sscanf(p, "RSSI=%d\n", &val) == 1)
        st.rssi = val;
    p = strstr(text, "NOISE=");
    if (p && sscanf(p, "NOISE=%d\n", &val) == 1)
        st.noise = val;
    p = strstr(text, "LINKSPEED=");
    if (p && sscanf(p, "LINKSPEED=%d\n", &val) == 1)
        st.linkSpeed = val;
    p = strstr(text, "FREQUENCY=");
    if (p && sscanf(p, "FREQUENCY=%d\n", &val) == 1)
        st.freq = val;
    return st;
}

int findProcess(const char *name, int flags);  // at 0x123d28

void *wifiMonitorThread(void *arg)  // 0x14f530
{
    printThreadInfo("new thread_wifi_monitor:");
    while (g_monitorRunning) {
        if (findProcess("wpa_supplicant", 0) >= 0) {
            g_connStatus = queryWifiStatus();
            if (g_statusCallback)
                g_statusCallback(&g_connStatus);
        }
        if (g_wifiScanEnable)
            rx_wifi_do_scan();
        if (g_wifiConnecting) {
            runUdhcpc();
            g_wifiConnecting = 0;
        } else {
            usleep(3000000);
        }
    }
}

void startWifiMonitor(void (*callback)(WifiStatus *))  // 0x14f634
{
    memset(&g_connStatus, 0, sizeof(g_connStatus));
    g_statusCallback = callback;
    if (g_monitorRunning) {
        printf("wifi monitor already started.");
        return;
    }
    g_monitorRunning = 1;
    printf("wifi monitor start...");
    g_statusCallback = callback;
    int ret = pthread_create(&g_monitorThread, NULL, wifiMonitorThread, NULL);
    pthread_detach(g_monitorThread);
}

void stopWifiMonitor()          // 0x14f6f4
{
    g_monitorRunning = 0;
    g_statusCallback = NULL;
    usleep(500000);
}
