// Shared 0x74-byte wpa_cli connection status (0x1889b0).
#pragma once
struct WifiStatus {
    int state; // 4 = COMPLETED
    int status;
    char ssid[0x40], ip[0x18];
    int rssi; // +0x60
    int linkSpeed, noise, freq, id;
};
