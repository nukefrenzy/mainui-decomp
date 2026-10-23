// C entry points shared by the launcher C and C++ translation units.
#pragma once

// WifiStatus.state, translated from wpa_cli's "wpa_state=" (queryWifiStatus, Wifi.c).
// MainUI's own numbering, not wpa_supplicant's; other wpa_cli states leave it 0.
enum WifiState {
    WIFI_STATE_DISABLED = 0,
    WIFI_STATE_INACTIVE = 1,
    WIFI_STATE_SCANNING = 2,
    WIFI_STATE_COMPLETED = 4,        // associated and authenticated: ready for DHCP
    WIFI_STATE_4WAY_HANDSHAKE = 6,   // checking the password
};

// What the Wi-Fi chip is doing, from the processes that run (wifiMode, WifiMenus.cpp).
enum WifiMode {
    WIFI_MODE_OFF = 1,
    WIFI_MODE_STATION = 2,   // wpa_supplicant runs: client of an access point
    WIFI_MODE_HOTSPOT = 3,   // hostapd runs: the device is the access point
};
#ifdef __cplusplus
extern "C" {
#endif
int findProcess(const char *name, int debug);
const char *getLanguageFile(void);
void saveSystemJson(void *shm, const char *theme, int fontSize);
unsigned char waitForProbe(void);
unsigned char authenticateDevice(unsigned char *challenge, unsigned char *response);
#ifdef __cplusplus
}
#endif
