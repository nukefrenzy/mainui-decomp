// Records shared by configuration loading and the main program.
#pragma once
#include <string>
struct RecentItem {
    std::string label, launch, rompath, imgpath;
    int type;
};
struct AppInfo {
    std::string label, icon, iconSelected, launch, description;
    int autoFlip, noAudioFix;
};
struct RomListPosition {
    std::string path;
    int current, start, selected;
};
