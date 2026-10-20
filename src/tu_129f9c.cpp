// Theme fonts, surfaces and sounds, in binary address order.
// Proven 0x129f9c-0x12ce28: seven functions, 11,916 bytes, no generated tail.
#include <cstdio>
#include <cstring>
#include <cstddef>
#include "Theme.h"
extern "C" {
void *TTF_OpenFont(const char *, int); void TTF_CloseFont(void *);
void TTF_SetFontStyle(void *, int); int TTF_FontHeight(void *);
SDL_Surface *IMG_Load(const char *);
SDL_Surface *SDL_CreateRGBSurface(unsigned int, int, int, int, unsigned int, unsigned int, unsigned int, unsigned int);
int SDL_UpperBlit(SDL_Surface *, SDL_Rect *, SDL_Surface *, SDL_Rect *);
void SDL_FreeSurface(SDL_Surface *);
const char *SDL_GetError();
void *SDL_RWFromFile(const char *, const char *);
void *Mix_LoadWAV_RW(void *, int); void *Mix_LoadMUS(const char *);
void Mix_FreeMusic(void *); void Mix_FreeChunk(void *);
const char *getLanguageFile();
}
bool languageFontExists();
SDL_Surface *loadSkinImage(const char *, const char *, const char *, SDL_Surface *, bool);
extern SDL_Surface *g_backBuffer;
void *g_imeFont; // 0x18217c
void *g_iconFont; // 0x182180
void *g_smallFont; // 0x182184
void *g_labelFont; // 0x182188
void *g_titleFont; // 0x18218c
SDL_Surface *g_background; // 0x182190
SDL_Surface *g_unusedIndicator; // 0x182194
SDL_Surface *g_unusedStatus; // 0x182198
SDL_Surface *g_navItem; // 0x18219c
SDL_Surface *g_dotActive; // 0x1821a0
SDL_Surface *g_dotNormal; // 0x1821a4
SDL_Surface *g_titleBar; // 0x1821a8
SDL_Surface *g_keySettingFooter; // 0x1821ac
SDL_Surface *g_miniLogo; // 0x1821b0
SDL_Surface *g_emptyImage; // 0x1821b4
SDL_Surface *g_listNumber; // 0x1821b8
SDL_Surface *g_numberBg; // 0x1821bc
SDL_Surface *g_gameFocused; // 0x1821c0
SDL_Surface *g_gameNormal; // 0x1821c4
SDL_Surface *g_listStripe; // 0x1821c8
SDL_Surface *g_vertLine; // 0x1821cc
SDL_Surface *g_raRow; // 0x1821d0
SDL_Surface *g_iconA; // 0x1821d4
SDL_Surface *g_iconB; // 0x1821d8
SDL_Surface *g_chargeIcon; // 0x1821dc
SDL_Surface *g_battery0; // 0x1821e0
SDL_Surface *g_battery1; // 0x1821e4
SDL_Surface *g_battery2; // 0x1821e8
SDL_Surface *g_battery3; // 0x1821ec
SDL_Surface *g_battery4; // 0x1821f0
SDL_Surface *g_folderIcon; // 0x1821f4
SDL_Surface *g_gameIcon; // 0x1821f8
SDL_Surface *g_previewBg; // 0x1821fc
SDL_Surface *g_hotspotIcon; // 0x182200
SDL_Surface *g_wifi4; // 0x182204
SDL_Surface *g_wifi3; // 0x182208
SDL_Surface *g_wifi2; // 0x18220c
SDL_Surface *g_headphoneIcon; // 0x182210
SDL_Color g_wifiEmptyColor; // 0x182274
unsigned int g_clearColor; // 0x182278
SDL_Color g_labelColor; // 0x18227c
SDL_Color g_wifiSelectedColor; // 0x182280
SDL_Color g_imeTextColor; // 0x182284
SDL_Color g_grayColor; // 0x182288
SDL_Color g_highlightColor; // 0x18228c
SDL_Color g_lightColor; // 0x182290
SDL_Color g_textColor; // 0x182298
SDL_Color g_selectedColor; // 0x18229c
SDL_Color g_selectedTextColor; // 0x1822a0
SDL_Color g_emptyColor; // 0x1822a4
SDL_Color g_hintColor; // 0x1822a8
SDL_Color g_listColor; // 0x1822ac
SDL_Color g_accentColor; // 0x1822b0
ThemeColor g_fixedColor; // 0x182294
SDL_Surface *g_volumeIcons[21]; // 0x182214
void *g_themeMusic, *g_changeSound, *g_enterSound; // 0x182268, 0x18226c, 0x182270
extern const ThemeColor g_themeWhite, g_themeGray, g_themeDim; // 0x16180c, 0x161808, 0x161810
static_assert(sizeof(Theme) == 0x18c && offsetof(Theme, mainSize) == 0x17c, "theme layout");

void *openThemeFont(const std::string &root, const std::string &configured,
                    const char *fallbackRoot, const char *fallbackFile, int size)
{
    std::string path(root);
    if (configured.length() != 0) {
        if (configured.at(0) != '/') path += configured;
        else path = configured;
    } else path += fallbackFile;
    printf("open %s %d\n", path.c_str(), size);
    void *font = TTF_OpenFont(path.c_str(), size);
    if (font) return font;
    path = fallbackRoot;
    path += fallbackFile;
    printf("fail, try open %s\n", path.c_str());
    return TTF_OpenFont(path.c_str(), size);
}
SDL_Surface *loadImeImage(const char *root, const char *fallback, const char *file)
{
    std::string path(root);
    path += file;
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (surface) {
        printf("%s/%s open ok\n", root, file);
        return surface;
    }
    path = fallback;
    path += file;
    printf("%s/%s open fail, use %s/%s\n", root, file, fallback, file);
    return IMG_Load(path.c_str());
}
int changeLanguageFonts(Theme *theme, int *size)
{
    if (g_imeFont) { printf("change_lang_font close %p\n", g_imeFont); TTF_CloseFont(g_imeFont); g_imeFont = NULL; }
    if (g_iconFont) { printf("change_lang_font close %p\n", g_iconFont); TTF_CloseFont(g_iconFont); g_iconFont = NULL; }
    if (g_titleFont) { printf("change_lang_font close %p\n", g_titleFont); TTF_CloseFont(g_titleFont); g_titleFont = NULL; }
    if (g_smallFont) { printf("change_lang_font close %p\n", g_smallFont); TTF_CloseFont(g_smallFont); g_smallFont = NULL; }
    if (g_labelFont) { TTF_CloseFont(g_labelFont); g_labelFont = NULL; }
    *size = theme->mainSize;
    bool available = languageFontExists();
    if (available != true || strncmp(getLanguageFile(), "en.lang", 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, "./", "wqy-microhei.ttc", theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, "./", "HENB.TTF", theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, "./", "HENB.TTF", theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, "./", "HENB.TTF", theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, "./", "HENB.TTF", theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, "./", "Exo-2-Bold-Italic.ttf", theme->labelSize);
        printf("change_lang_font en stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n", g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, "./", "wqy-microhei.ttc", theme->titleSize);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, "./", "wqy-microhei.ttc", theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, "./", "wqy-microhei.ttc", theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);
        printf("change_lang_font stitlefont %p, sfonticon %p, sfontsmall %p, sfonotlabel %p\n", g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    }
    if (!g_iconFont) fprintf(stderr, "change font bold fail: %s\n", SDL_GetError());
    return 0;
}
SDL_Surface *compositeThemeImage(SDL_Surface *image, SDL_Surface *background)
{
    if (image) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, image->w, image->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_Surface *result = SDL_CreateRGBSurface(1, image->w, image->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        if (background) {
            SDL_Rect source;
            source.x = (background->w - image->w) / 2;
            source.y = (background->h - image->h) / 2;
            source.w = image->w; source.h = image->h;
            SDL_UpperBlit(background, &source, temp, NULL);
        }
        SDL_UpperBlit(image, NULL, temp, NULL);
        SDL_UpperBlit(temp, NULL, result, NULL);
        SDL_FreeSurface(temp);
        return result;
    }
    return NULL;
}
void setDefaultTheme(Theme *theme)
{
    theme->name = "Default";
    theme->root = "./";
    theme->titleSize = 0x24;
    theme->titleColor = g_themeWhite;
    theme->hintSize = 0x18;
    theme->hintColor = g_themeWhite;
    theme->labelSize = 0x28;
    theme->labelColor = g_themeWhite;
    theme->listSize = 0x28;
    theme->listColor = g_themeWhite;
    theme->emptySize = 0x28;
    theme->emptyColor = g_themeDim;
    theme->iconSize = 0x19;
    theme->smallSize = 0x12;
    theme->textColor = g_themeWhite;
    theme->selectedColor = g_themeGray;
    theme->field140 = 0;
    theme->field144 = 0;
    theme->field148 = 0;
    theme->mainSize = 0x18;
    theme->wifiColor = g_themeWhite;
    theme->imeColor = g_themeWhite;
    theme->accentColor.value = 0;
    theme->accentColor.bytes.r = 255;
    theme->accentColor.bytes.g = 255;
    theme->field144 = 0;
    theme->field140 = 0;
    theme->field148 = 0;
}
int loadTheme(Theme *theme, int *unusedFontSize)
{
    char filename[128];
    memset(filename, 0, 128);
    g_clearColor = 0;
    g_wifiEmptyColor = theme->titleColor.bytes;
    g_labelColor = theme->labelColor.bytes;
    g_wifiSelectedColor = theme->wifiColor.bytes;
    g_imeTextColor = theme->imeColor.bytes;
    g_textColor = theme->textColor.bytes;
    g_selectedColor = theme->selectedColor.bytes;
    g_selectedTextColor = theme->selectedColor.bytes;
    g_emptyColor = theme->emptyColor.bytes;
    g_hintColor = theme->hintColor.bytes;
    g_listColor = theme->listColor.bytes;
    g_accentColor = theme->accentColor.bytes;
    g_highlightColor.r = 0xd0; g_highlightColor.g = 0xe4; g_highlightColor.b = 0x58; g_highlightColor.unused = 0;
    g_lightColor.r = 0xf0; g_lightColor.g = 0xf0; g_lightColor.b = 0xf0; g_lightColor.unused = 0;
    g_fixedColor.value = 0;
    g_fixedColor.bytes.r = 0x68; g_fixedColor.bytes.g = 0x46; g_fixedColor.bytes.b = 0x18;
    g_grayColor.r = 0x68; g_grayColor.g = 0x68; g_grayColor.b = 0x68; g_grayColor.unused = 0;
    bool available = languageFontExists();
    printf("sys lang %s, path %s, support multilanguage %d\n", getLanguageFile(), theme->root.c_str(), available);
    if (available != true || strncmp(getLanguageFile(), "en.lang", 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, "./", "wqy-microhei.ttc", theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, "./", "HENB.TTF", theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, "./", "HENB.TTF", theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, "./", "HENB.TTF", theme->iconSize);
        printf("sfonticon = %p\n", g_iconFont);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, "./", "HENB.TTF", theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, "./", "Exo-2-Bold-Italic.ttf", theme->labelSize);

    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, "./", "wqy-microhei.ttc", theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, "./", "wqy-microhei.ttc", theme->titleSize);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, "./", "wqy-microhei.ttc", theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, "./", "wqy-microhei.ttc", theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);

    }
    printf("init_skin sfont %p stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n", g_imeFont, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    if (!g_imeFont) fprintf(stderr, "open font fail: %s\n", SDL_GetError());
    if (!g_iconFont) fprintf(stderr, "open font bold fail: %s\n", SDL_GetError());
    g_background = loadImeImage(theme->root.c_str(), "./", "skin/background.png");
    if (g_background) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_background->w, g_background->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_background, NULL, temp, NULL);
        SDL_FreeSurface(g_background);
        g_background = temp;
    } else fprintf(stderr, "open background fail: %s\n", SDL_GetError());
    g_listStripe = loadSkinImage(theme->root.c_str(), "./", "./skin/div-line-h.png", g_backBuffer, 0);
    if (g_listStripe) {
        SDL_Surface *temp = compositeThemeImage(g_listStripe, g_background);
        SDL_FreeSurface(g_listStripe);
        g_listStripe = temp;
    } else fprintf(stderr, "open div-line-h.png fail: %s\n", SDL_GetError());
    g_vertLine = loadSkinImage(theme->root.c_str(), "./", "./skin/div-line-v-01.png", g_backBuffer, 0);
    if (g_vertLine) {
        SDL_Surface *temp = compositeThemeImage(g_vertLine, g_background);
        SDL_FreeSurface(g_vertLine);
        g_vertLine = temp;
    } else fprintf(stderr, "open bg_hline fail: %s\n", SDL_GetError());
    g_unusedIndicator = NULL;
    g_dotActive = loadSkinImage(theme->root.c_str(), "./", "./skin/dot-a.png", g_backBuffer, 0);
    if (!g_dotActive) fprintf(stderr, "open dot-a fail: %s\n", SDL_GetError());
    g_dotNormal = loadSkinImage(theme->root.c_str(), "./", "./skin/dot-n.png", g_backBuffer, 0);
    if (!g_dotNormal) fprintf(stderr, "open dot-n fail: %s\n", SDL_GetError());
    g_unusedStatus = NULL;
    g_navItem = loadImeImage(theme->root.c_str(), "./", "skin/nav-bar-item-bg.png");
    if (g_navItem) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_navItem->w, g_navItem->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_navItem, NULL, temp, NULL);
        SDL_FreeSurface(g_navItem);
        g_navItem = temp;
    } else fprintf(stderr, "open stat_bg fail: %s\n", SDL_GetError());
    g_titleBar = loadSkinImage(theme->root.c_str(), "./", "./skin/bg-title.png", g_backBuffer, 1);
    if (g_titleBar) {
        SDL_Surface *temp = compositeThemeImage(g_titleBar, g_background);
        SDL_FreeSurface(g_titleBar);
        g_titleBar = temp;
    } else fprintf(stderr, "open bar_bg fail: %s\n", SDL_GetError());
    g_keySettingFooter = loadSkinImage(theme->root.c_str(), "./", "./skin/tips-bar-bg.png", g_backBuffer, 0);
    if (g_keySettingFooter) {
        SDL_Surface *temp = compositeThemeImage(g_keySettingFooter, g_background);
        SDL_FreeSurface(g_keySettingFooter);
        g_keySettingFooter = temp;
    } else fprintf(stderr, "open bar_bg fail: %s\n", SDL_GetError());
    g_miniLogo = loadSkinImage(theme->root.c_str(), "./", "./skin/miyoo-topbar.png", g_backBuffer, 0);
    if (!g_miniLogo) fprintf(stderr, "open mini_logo fail: %s\n", SDL_GetError());
    g_emptyImage = loadSkinImage(theme->root.c_str(), "./", "./skin/Empty.png", g_backBuffer, 0);
    if (!g_emptyImage) fprintf(stderr, "open Empty.png fail: %s\n", SDL_GetError());
    g_listNumber = loadSkinImage(theme->root.c_str(), "./", "./skin/list-num.png", g_backBuffer, 0);
    if (!g_listNumber) fprintf(stderr, "open list_num fail: %s\n", SDL_GetError());
    g_numberBg = loadSkinImage(theme->root.c_str(), "./", "./skin/num-bg.png", g_backBuffer, 0);
    if (!g_numberBg) fprintf(stderr, "open num_bg fail: %s\n", SDL_GetError());
    std::string path(theme->root);
    path += "sound/change.wav";
    printf("Load Music %s\n", path.c_str());
    g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), "rb"), 1);
    if (!g_changeSound) {
        printf("Load Default Music %s\n", "./sound/change.wav");
        g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile("./sound/change.wav", "rb"), 1);
        if (!g_changeSound) fprintf(stderr, "load music %s fail: %s\n", "./sound/change.wav", SDL_GetError());
    }
    path = theme->root;
    path += "sound/enter.wav";
    printf("Load Music %s\n", path.c_str());
    g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), "rb"), 1);
    if (!g_enterSound) {
        printf("Load Default Music %s\n", "/etc/enter.wav");
        g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile("/etc/enter.wav", "rb"), 1);
        if (!g_enterSound) fprintf(stderr, "load music %s fail: %s\n", "/etc/enter.wav", SDL_GetError());
    }
    path = theme->root;
    path += "sound/bgm.mp3";
    printf("Load Music %s\n", path.c_str());
    g_themeMusic = Mix_LoadMUS(path.c_str());
    if (!g_themeMusic) {
        printf("Load Default Music %s\n", "./sound/bgm.mp3");
        g_themeMusic = Mix_LoadMUS("./sound/bgm.mp3");
        if (!g_themeMusic) fprintf(stderr, "load music %s fail: %s\n", "./sound/bgm.mp3", SDL_GetError());
    }
    g_gameFocused = loadImeImage(theme->root.c_str(), "./", "skin/bg-game-item-f.png");
    if (g_gameFocused) {
        SDL_Surface *temp = compositeThemeImage(g_gameFocused, g_background);
        SDL_FreeSurface(g_gameFocused);
        g_gameFocused = temp;
    }
    g_gameNormal = loadImeImage(theme->root.c_str(), "./", "skin/bg-game-item-n.png");
    if (g_gameNormal) {
        SDL_Surface *temp = compositeThemeImage(g_gameNormal, g_background);
        SDL_FreeSurface(g_gameNormal);
        g_gameNormal = temp;
    }
    g_raRow = loadImeImage(theme->root.c_str(), "./", "skin/bg-ra-list-item.png");
    if (g_raRow) {
        SDL_Surface *temp = compositeThemeImage(g_raRow, NULL);
        SDL_FreeSurface(g_raRow);
        g_raRow = temp;
    }
    g_iconA = loadSkinImage(theme->root.c_str(), "./", "skin/icon-A-54.png", g_backBuffer, 0);
    g_iconB = loadSkinImage(theme->root.c_str(), "./", "skin/icon-B-54.png", g_backBuffer, 0);
    g_chargeIcon = loadSkinImage(theme->root.c_str(), "./", "skin/ic-power-charge-100%.png", g_backBuffer, 0);
    g_battery0 = loadSkinImage(theme->root.c_str(), "./", "skin/.batt-perc.png", g_backBuffer, 0);
    g_battery1 = loadSkinImage(theme->root.c_str(), "./", "skin/.batt-perc.png", g_backBuffer, 0);
    g_battery2 = loadSkinImage(theme->root.c_str(), "./", "skin/.batt-perc.png", g_backBuffer, 0);
    g_battery3 = loadSkinImage(theme->root.c_str(), "./", "skin/.batt-perc.png", g_backBuffer, 0);
    g_battery4 = loadSkinImage(theme->root.c_str(), "./", "skin/.batt-perc.png", g_backBuffer, 0);
    g_folderIcon = loadSkinImage(theme->root.c_str(), "./", "skin/icon-folder.png", g_backBuffer, 0);
    g_gameIcon = loadSkinImage(theme->root.c_str(), "./", "skin/icon-game.png", g_backBuffer, 0);
    g_previewBg = loadSkinImage(theme->root.c_str(), "./", "skin/preview-bg.png", g_backBuffer, 0);
    g_hotspotIcon = loadSkinImage(theme->root.c_str(), "./", "skin/icon-hotspot-wifi.png", g_backBuffer, 0);
    g_wifi4 = loadSkinImage(theme->root.c_str(), "./", "skin/icon-wifi-signal-04.png", g_backBuffer, 0);
    g_wifi3 = loadSkinImage(theme->root.c_str(), "./", "skin/icon-wifi-signal-03.png", g_backBuffer, 0);
    g_wifi2 = loadSkinImage(theme->root.c_str(), "./", "skin/icon-wifi-signal-02.png", g_backBuffer, 0);
    g_headphoneIcon = loadSkinImage(theme->root.c_str(), "./", "skin/headphone-icon.png", g_backBuffer, 0);
    for (int i = 0; i < 21; i++) {
        snprintf(filename, 128, "skin/icon-volume-%02d.png", i);
        g_volumeIcons[i] = loadSkinImage(theme->root.c_str(), "./", filename, g_backBuffer, 0);
    }
    if (!g_background || !g_imeFont) return 0;
    printf("load font height is %d\n", TTF_FontHeight(g_imeFont));
    return 1;
}
int freeTheme()
{
    if (g_background) {
        printf("free background %p %dx%d\n", g_background, g_background->w, g_background->h);
        SDL_FreeSurface(g_background);
    }
    if (g_imeFont) { printf("free sfont %p\n", g_imeFont); TTF_CloseFont(g_imeFont); }
    if (g_iconFont) TTF_CloseFont(g_iconFont);
    if (g_titleFont) TTF_CloseFont(g_titleFont);
    if (g_smallFont) TTF_CloseFont(g_smallFont);
    if (g_labelFont) TTF_CloseFont(g_labelFont);
    if (g_unusedStatus) SDL_FreeSurface(g_unusedStatus);
    if (g_navItem) SDL_FreeSurface(g_navItem);
    if (g_unusedIndicator) SDL_FreeSurface(g_unusedIndicator);
    if (g_dotActive) SDL_FreeSurface(g_dotActive);
    if (g_dotNormal) SDL_FreeSurface(g_dotNormal);
    if (g_titleBar) SDL_FreeSurface(g_titleBar);
    if (g_keySettingFooter) SDL_FreeSurface(g_keySettingFooter);
    if (g_miniLogo) SDL_FreeSurface(g_miniLogo);
    if (g_emptyImage) SDL_FreeSurface(g_emptyImage);
    if (g_numberBg) SDL_FreeSurface(g_numberBg);
    if (g_listNumber) SDL_FreeSurface(g_listNumber);
    if (g_gameFocused) SDL_FreeSurface(g_gameFocused);
    if (g_gameNormal) SDL_FreeSurface(g_gameNormal);
    if (g_raRow) SDL_FreeSurface(g_raRow);
    if (g_listStripe) SDL_FreeSurface(g_listStripe);
    if (g_vertLine) SDL_FreeSurface(g_vertLine);
    if (g_iconA) SDL_FreeSurface(g_iconA);
    if (g_iconB) SDL_FreeSurface(g_iconB);
    if (g_chargeIcon) SDL_FreeSurface(g_chargeIcon);
    if (g_battery0) SDL_FreeSurface(g_battery0);
    if (g_battery1) SDL_FreeSurface(g_battery1);
    if (g_battery2) SDL_FreeSurface(g_battery2);
    if (g_battery3) SDL_FreeSurface(g_battery3);
    if (g_battery4) SDL_FreeSurface(g_battery4);
    if (g_folderIcon) SDL_FreeSurface(g_folderIcon);
    if (g_gameIcon) SDL_FreeSurface(g_gameIcon);
    if (g_previewBg) SDL_FreeSurface(g_previewBg);
    if (g_hotspotIcon) SDL_FreeSurface(g_hotspotIcon);
    if (g_wifi4) SDL_FreeSurface(g_wifi4);
    if (g_wifi3) SDL_FreeSurface(g_wifi3);
    if (g_wifi2) SDL_FreeSurface(g_wifi2);
    if (g_headphoneIcon) SDL_FreeSurface(g_headphoneIcon);
    for (int i = 0; i < 21; i++) if (g_volumeIcons[i]) SDL_FreeSurface(g_volumeIcons[i]);
    if (g_themeMusic) Mix_FreeMusic(g_themeMusic);
    if (g_changeSound) Mix_FreeChunk(g_changeSound);
    if (g_enterSound) Mix_FreeChunk(g_enterSound);
    return 1;
}
