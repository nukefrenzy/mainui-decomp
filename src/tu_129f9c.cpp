// Theme fonts, surfaces and sounds, in binary address order.
// Proven 0x129f9c-0x12ce28: seven functions, 11,916 bytes, no generated tail.
#include "PlatformApi.h"
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
SDL_Surface *g_volumeIcons[21]; // 0x182214
void *g_themeMusic, *g_changeSound, *g_enterSound; // 0x182268, 0x18226c, 0x182270
SDL_Color g_wifiEmptyColor; // 0x182274
SDL_Color g_clearColor; // 0x182278
SDL_Color g_labelColor; // 0x18227c
SDL_Color g_wifiSelectedColor; // 0x182280
SDL_Color g_imeTextColor; // 0x182284
SDL_Color g_grayColor; // 0x182288
SDL_Color g_highlightColor; // 0x18228c
SDL_Color g_lightColor; // 0x182290
ThemeColor g_fixedColor; // 0x182294
SDL_Color g_textColor; // 0x182298
SDL_Color g_selectedColor; // 0x18229c
SDL_Color g_selectedTextColor; // 0x1822a0
SDL_Color g_emptyColor; // 0x1822a4
SDL_Color g_hintColor; // 0x1822a8
SDL_Color g_listColor; // 0x1822ac
SDL_Color g_accentColor; // 0x1822b0
// Original palette follows std::piecewise_construct. Struct copies retain loads at -O0.
extern const SDL_Color g_themeGray = {104,104,104,0}; // 0x161808
extern const SDL_Color g_themeWhite = {240,240,240,0}; // 0x16180c
extern const SDL_Color g_themeDim = {170,170,170,0}; // 0x161810
extern const SDL_Color g_themeUnusedPalette[] = { // 0x161814-0x161838
    {0,0,0,0}, {17,100,143,0}, {210,180,95,0}, {208,228,88,0},
    {240,0,36,0}, {255,0,98,0}, {210,172,115,0}, {104,70,24,0}, {255,255,0,0}
};
static_assert(sizeof(Theme) == 0x18c && offsetof(Theme, mainSize) == 0x17c, "theme layout");

// Explicit pool preserves the target's patched, distinct 28/24-byte battery paths.
// Other literals retain original first-use order; all values are readable source constants.
extern const char theme_literal_161838[] __attribute__((aligned(4))) = "open %s %d\n";
extern const char theme_literal_161844[] __attribute__((aligned(4))) = "fail, try open %s\n";
extern const char theme_literal_161858[] __attribute__((aligned(4))) = "%s/%s open ok\n";
extern const char theme_literal_161868[] __attribute__((aligned(4))) = "%s/%s open fail, use %s/%s\n";
extern const char theme_literal_161884[] __attribute__((aligned(4))) = "change_lang_font close %p\n";
extern const char theme_literal_1618a0[] __attribute__((aligned(4))) = "en.lang";
extern const char theme_literal_1618a8[] __attribute__((aligned(4))) = "wqy-microhei.ttc";
extern const char theme_literal_1618bc[] __attribute__((aligned(4))) = "./";
extern const char theme_literal_1618c0[] __attribute__((aligned(4))) = "HENB.TTF";
extern const char theme_literal_1618cc[] __attribute__((aligned(4))) = "Exo-2-Bold-Italic.ttf";
extern const char theme_literal_1618e4[] __attribute__((aligned(4))) = "change_lang_font en stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n";
extern const char theme_literal_161934[] __attribute__((aligned(4))) = "change_lang_font stitlefont %p, sfonticon %p, sfontsmall %p, sfonotlabel %p\n";
extern const char theme_literal_161984[] __attribute__((aligned(4))) = "change font bold fail: %s\n";
extern const char theme_literal_1619a0[] __attribute__((aligned(4))) = "Default";
extern const char theme_literal_1619a8[] __attribute__((aligned(4))) = "sys lang %s, path %s, support multilanguage %d\n";
extern const char theme_literal_1619d8[] __attribute__((aligned(4))) = "sfonticon = %p\n";
extern const char theme_literal_1619e8[] __attribute__((aligned(4))) = "init_skin sfont %p stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n";
extern const char theme_literal_161a38[] __attribute__((aligned(4))) = "open font fail: %s\n";
extern const char theme_literal_161a4c[] __attribute__((aligned(4))) = "open font bold fail: %s\n";
extern const char theme_literal_161a68[] __attribute__((aligned(4))) = "skin/background.png";
extern const char theme_literal_161a7c[] __attribute__((aligned(4))) = "open background fail: %s\n";
extern const char theme_literal_161a98[] __attribute__((aligned(4))) = "./skin/div-line-h.png";
extern const char theme_literal_161ab0[] __attribute__((aligned(4))) = "open div-line-h.png fail: %s\n";
extern const char theme_literal_161ad0[] __attribute__((aligned(4))) = "./skin/div-line-v-01.png";
extern const char theme_literal_161aec[] __attribute__((aligned(4))) = "open bg_hline fail: %s\n";
extern const char theme_literal_161b04[] __attribute__((aligned(4))) = "./skin/dot-a.png";
extern const char theme_literal_161b18[] __attribute__((aligned(4))) = "open dot-a fail: %s\n";
extern const char theme_literal_161b30[] __attribute__((aligned(4))) = "./skin/dot-n.png";
extern const char theme_literal_161b44[] __attribute__((aligned(4))) = "open dot-n fail: %s\n";
extern const char theme_literal_161b5c[] __attribute__((aligned(4))) = "skin/nav-bar-item-bg.png";
extern const char theme_literal_161b78[] __attribute__((aligned(4))) = "open stat_bg fail: %s\n";
extern const char theme_literal_161b90[] __attribute__((aligned(4))) = "./skin/bg-title.png";
extern const char theme_literal_161ba4[] __attribute__((aligned(4))) = "open bar_bg fail: %s\n";
extern const char theme_literal_161bbc[] __attribute__((aligned(4))) = "./skin/tips-bar-bg.png";
extern const char theme_literal_161bd4[] __attribute__((aligned(4))) = "./skin/miyoo-topbar.png";
extern const char theme_literal_161bec[] __attribute__((aligned(4))) = "open mini_logo fail: %s\n";
extern const char theme_literal_161c08[] __attribute__((aligned(4))) = "./skin/Empty.png";
extern const char theme_literal_161c1c[] __attribute__((aligned(4))) = "open Empty.png fail: %s\n";
extern const char theme_literal_161c38[] __attribute__((aligned(4))) = "./skin/list-num.png";
extern const char theme_literal_161c4c[] __attribute__((aligned(4))) = "open list_num fail: %s\n";
extern const char theme_literal_161c64[] __attribute__((aligned(4))) = "./skin/num-bg.png";
extern const char theme_literal_161c78[] __attribute__((aligned(4))) = "open num_bg fail: %s\n";
extern const char theme_literal_161c90[] __attribute__((aligned(4))) = "sound/change.wav";
extern const char theme_literal_161ca4[] __attribute__((aligned(4))) = "Load Music %s\n";
extern const char theme_literal_161cb4[] __attribute__((aligned(4))) = "rb";
extern const char theme_literal_161cb8[] __attribute__((aligned(4))) = "./sound/change.wav";
extern const char theme_literal_161ccc[] __attribute__((aligned(4))) = "Load Default Music %s\n";
extern const char theme_literal_161ce4[] __attribute__((aligned(4))) = "load music %s fail: %s\n";
extern const char theme_literal_161cfc[] __attribute__((aligned(4))) = "sound/enter.wav";
extern const char theme_literal_161d0c[] __attribute__((aligned(4))) = "/etc/enter.wav";
extern const char theme_literal_161d1c[] __attribute__((aligned(4))) = "sound/bgm.mp3";
extern const char theme_literal_161d2c[] __attribute__((aligned(4))) = "./sound/bgm.mp3";
extern const char theme_literal_161d3c[] __attribute__((aligned(4))) = "skin/bg-game-item-f.png";
extern const char theme_literal_161d54[] __attribute__((aligned(4))) = "skin/bg-game-item-n.png";
extern const char theme_literal_161d6c[] __attribute__((aligned(4))) = "skin/bg-ra-list-item.png";
extern const char theme_literal_161d88[] __attribute__((aligned(4))) = "skin/icon-A-54.png";
extern const char theme_literal_161d9c[] __attribute__((aligned(4))) = "skin/icon-B-54.png";
extern const char theme_literal_161db0[] __attribute__((aligned(4))) = "skin/ic-power-charge-100%.png";
extern const char theme_literal_161dd0[28] __attribute__((aligned(4))) = "skin/.batt-perc.png";
extern const char theme_literal_161dec[24] __attribute__((aligned(4))) = "skin/.batt-perc.png";
extern const char theme_literal_161e04[24] __attribute__((aligned(4))) = "skin/.batt-perc.png";
extern const char theme_literal_161e1c[24] __attribute__((aligned(4))) = "skin/.batt-perc.png";
extern const char theme_literal_161e34[24] __attribute__((aligned(4))) = "skin/.batt-perc.png";
extern const char theme_literal_161e4c[] __attribute__((aligned(4))) = "skin/icon-folder.png";
extern const char theme_literal_161e64[] __attribute__((aligned(4))) = "skin/icon-game.png";
extern const char theme_literal_161e78[] __attribute__((aligned(4))) = "skin/preview-bg.png";
extern const char theme_literal_161e8c[] __attribute__((aligned(4))) = "skin/icon-hotspot-wifi.png";
extern const char theme_literal_161ea8[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-04.png";
extern const char theme_literal_161ec8[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-03.png";
extern const char theme_literal_161ee8[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-02.png";
extern const char theme_literal_161f08[] __attribute__((aligned(4))) = "skin/headphone-icon.png";
extern const char theme_literal_161f20[] __attribute__((aligned(4))) = "skin/icon-volume-%02d.png";
extern const char theme_literal_161f3c[] __attribute__((aligned(4))) = "load font height is %d\n";
extern const char theme_literal_161f54[] __attribute__((aligned(4))) = "free background %p %dx%d\n";
extern const char theme_literal_161f70[] __attribute__((aligned(4))) = "free sfont %p\n";

void *openThemeFont(const std::string &root, std::string &configured,
                    const char *fallbackRoot, const char *fallbackFile, int size)
{
    std::string path(root);
    if (configured.length() != 0) {
        if (configured.at(0) != '/') path += configured;
        else path = configured;
    } else path += fallbackFile;
    printf(theme_literal_161838, path.c_str(), size);
    void *font = TTF_OpenFont(path.c_str(), size);
    if (font) return font;
    path = fallbackRoot;
    path += fallbackFile;
    printf(theme_literal_161844, path.c_str());
    return TTF_OpenFont(path.c_str(), size);
}
SDL_Surface *loadImeImage(const char *root, const char *fallback, const char *file)
{
    std::string path(root);
    path += file;
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (surface) {
        printf(theme_literal_161858, root, file);
        return surface;
    }
    path = fallback;
    path += file;
    printf(theme_literal_161868, root, file, fallback, file);
    return IMG_Load(path.c_str());
}
int changeLanguageFonts(Theme *theme, int *size)
{
    if (g_imeFont) { printf(theme_literal_161884, g_imeFont); TTF_CloseFont(g_imeFont); g_imeFont = NULL; }
    if (g_iconFont) { printf(theme_literal_161884, g_iconFont); TTF_CloseFont(g_iconFont); g_iconFont = NULL; }
    if (g_titleFont) { printf(theme_literal_161884, g_titleFont); TTF_CloseFont(g_titleFont); g_titleFont = NULL; }
    if (g_smallFont) { printf(theme_literal_161884, g_smallFont); TTF_CloseFont(g_smallFont); g_smallFont = NULL; }
    if (g_labelFont) { TTF_CloseFont(g_labelFont); g_labelFont = NULL; }
    *size = theme->mainSize;
    bool available = languageFontExists();
    if (available != true || strncmp(getLanguageFile(), theme_literal_1618a0, 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, theme_literal_1618bc, theme_literal_1618a8, theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, theme_literal_1618bc, theme_literal_1618cc, theme->labelSize);
        printf(theme_literal_1618e4, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->titleSize);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);
        printf(theme_literal_161934, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    }
    if (!g_iconFont) fprintf(stderr, theme_literal_161984, SDL_GetError());
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
    theme->name = theme_literal_1619a0;
    theme->root = theme_literal_1618bc;
    theme->titleSize = 0x24;
    theme->titleColor.bytes = g_themeWhite;
    theme->hintSize = 0x18;
    theme->hintColor.bytes = g_themeWhite;
    theme->labelSize = 0x28;
    theme->labelColor.bytes = g_themeWhite;
    theme->listSize = 0x28;
    theme->listColor.bytes = g_themeWhite;
    theme->emptySize = 0x28;
    theme->emptyColor.bytes = g_themeDim;
    theme->iconSize = 0x19;
    theme->smallSize = 0x12;
    theme->textColor.bytes = g_themeWhite;
    theme->selectedColor.bytes = g_themeGray;
    theme->field140 = 0;
    theme->field144 = 0;
    theme->field148 = 0;
    theme->mainSize = 0x18;
    theme->wifiColor.bytes = g_themeWhite;
    theme->imeColor.bytes = g_themeWhite;
    theme->accentColor.value = 0;
    theme->accentColor.bytes.r = 255;
    theme->accentColor.bytes.g = 255;
    theme->field144 = 0;
    theme->field140 = 0;
    theme->field148 = 0;
}
bool loadTheme(Theme *theme, int *unusedFontSize)
{
    char filename[128];
    memset(filename, 0, 128);
    g_clearColor = {};
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
    printf(theme_literal_1619a8, getLanguageFile(), theme->root.c_str(), available);
    if (available != true || strncmp(getLanguageFile(), theme_literal_1618a0, 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, theme_literal_1618bc, theme_literal_1618a8, theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->iconSize);
        printf(theme_literal_1619d8, g_iconFont);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, theme_literal_1618bc, theme_literal_1618c0, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, theme_literal_1618bc, theme_literal_1618cc, theme->labelSize);

    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->titleSize);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, theme_literal_1618bc, theme_literal_1618a8, theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);

    }
    printf(theme_literal_1619e8, g_imeFont, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    if (!g_imeFont) fprintf(stderr, theme_literal_161a38, SDL_GetError());
    if (!g_iconFont) fprintf(stderr, theme_literal_161a4c, SDL_GetError());
    g_background = loadImeImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161a68);
    if (g_background) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_background->w, g_background->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_background, NULL, temp, NULL);
        SDL_FreeSurface(g_background);
        g_background = temp;
    } else fprintf(stderr, theme_literal_161a7c, SDL_GetError());
    g_listStripe = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161a98, g_backBuffer, 0);
    if (g_listStripe) {
        SDL_Surface *temp = compositeThemeImage(g_listStripe, g_background);
        SDL_FreeSurface(g_listStripe);
        g_listStripe = temp;
    } else fprintf(stderr, theme_literal_161ab0, SDL_GetError());
    g_vertLine = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161ad0, g_backBuffer, 0);
    if (g_vertLine) {
        SDL_Surface *temp = compositeThemeImage(g_vertLine, g_background);
        SDL_FreeSurface(g_vertLine);
        g_vertLine = temp;
    } else fprintf(stderr, theme_literal_161aec, SDL_GetError());
    g_unusedIndicator = NULL;
    g_dotActive = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161b04, g_backBuffer, 0);
    if (!g_dotActive) fprintf(stderr, theme_literal_161b18, SDL_GetError());
    g_dotNormal = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161b30, g_backBuffer, 0);
    if (!g_dotNormal) fprintf(stderr, theme_literal_161b44, SDL_GetError());
    g_unusedStatus = NULL;
    g_navItem = loadImeImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161b5c);
    if (g_navItem) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_navItem->w, g_navItem->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_navItem, NULL, temp, NULL);
        SDL_FreeSurface(g_navItem);
        g_navItem = temp;
    } else fprintf(stderr, theme_literal_161b78, SDL_GetError());
    g_titleBar = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161b90, g_backBuffer, 1);
    if (g_titleBar) {
        SDL_Surface *temp = compositeThemeImage(g_titleBar, g_background);
        SDL_FreeSurface(g_titleBar);
        g_titleBar = temp;
    } else fprintf(stderr, theme_literal_161ba4, SDL_GetError());
    g_keySettingFooter = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161bbc, g_backBuffer, 0);
    if (g_keySettingFooter) {
        SDL_Surface *temp = compositeThemeImage(g_keySettingFooter, g_background);
        SDL_FreeSurface(g_keySettingFooter);
        g_keySettingFooter = temp;
    } else fprintf(stderr, theme_literal_161ba4, SDL_GetError());
    g_miniLogo = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161bd4, g_backBuffer, 0);
    if (!g_miniLogo) fprintf(stderr, theme_literal_161bec, SDL_GetError());
    g_emptyImage = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161c08, g_backBuffer, 0);
    if (!g_emptyImage) fprintf(stderr, theme_literal_161c1c, SDL_GetError());
    g_listNumber = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161c38, g_backBuffer, 0);
    if (!g_listNumber) fprintf(stderr, theme_literal_161c4c, SDL_GetError());
    g_numberBg = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161c64, g_backBuffer, 0);
    if (!g_numberBg) fprintf(stderr, theme_literal_161c78, SDL_GetError());
    std::string path(theme->root);
    path += theme_literal_161c90;
    printf(theme_literal_161ca4, path.c_str());
    g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), theme_literal_161cb4), 1);
    if (!g_changeSound) {
        printf(theme_literal_161ccc, theme_literal_161cb8);
        g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile(theme_literal_161cb8, theme_literal_161cb4), 1);
        if (!g_changeSound) fprintf(stderr, theme_literal_161ce4, theme_literal_161cb8, SDL_GetError());
    }
    path = theme->root;
    path += theme_literal_161cfc;
    printf(theme_literal_161ca4, path.c_str());
    g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), theme_literal_161cb4), 1);
    if (!g_enterSound) {
        printf(theme_literal_161ccc, theme_literal_161d0c);
        g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile(theme_literal_161d0c, theme_literal_161cb4), 1);
        if (!g_enterSound) fprintf(stderr, theme_literal_161ce4, theme_literal_161d0c, SDL_GetError());
    }
    path = theme->root;
    path += theme_literal_161d1c;
    printf(theme_literal_161ca4, path.c_str());
    g_themeMusic = Mix_LoadMUS(path.c_str());
    if (!g_themeMusic) {
        printf(theme_literal_161ccc, theme_literal_161d2c);
        g_themeMusic = Mix_LoadMUS(theme_literal_161d2c);
        if (!g_themeMusic) fprintf(stderr, theme_literal_161ce4, theme_literal_161d2c, SDL_GetError());
    }
    g_gameFocused = loadImeImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161d3c);
    if (g_gameFocused) {
        SDL_Surface *temp = compositeThemeImage(g_gameFocused, g_background);
        SDL_FreeSurface(g_gameFocused);
        g_gameFocused = temp;
    }
    g_gameNormal = loadImeImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161d54);
    if (g_gameNormal) {
        SDL_Surface *temp = compositeThemeImage(g_gameNormal, g_background);
        SDL_FreeSurface(g_gameNormal);
        g_gameNormal = temp;
    }
    g_raRow = loadImeImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161d6c);
    if (g_raRow) {
        SDL_Surface *temp = compositeThemeImage(g_raRow, NULL);
        SDL_FreeSurface(g_raRow);
        g_raRow = temp;
    }
    g_iconA = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161d88, g_backBuffer, 0);
    g_iconB = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161d9c, g_backBuffer, 0);
    g_chargeIcon = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161db0, g_backBuffer, 0);
    g_battery0 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161dd0, g_backBuffer, 0);
    g_battery1 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161dec, g_backBuffer, 0);
    g_battery2 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e04, g_backBuffer, 0);
    g_battery3 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e1c, g_backBuffer, 0);
    g_battery4 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e34, g_backBuffer, 0);
    g_folderIcon = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e4c, g_backBuffer, 0);
    g_gameIcon = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e64, g_backBuffer, 0);
    g_previewBg = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e78, g_backBuffer, 0);
    g_hotspotIcon = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161e8c, g_backBuffer, 0);
    g_wifi4 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161ea8, g_backBuffer, 0);
    g_wifi3 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161ec8, g_backBuffer, 0);
    g_wifi2 = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161ee8, g_backBuffer, 0);
    g_headphoneIcon = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, theme_literal_161f08, g_backBuffer, 0);
    for (int i = 0; i < 21; i++) {
        snprintf(filename, 128, theme_literal_161f20, i);
        g_volumeIcons[i] = loadSkinImage(theme->root.c_str(), theme_literal_1618bc, filename, g_backBuffer, 0);
    }
    if (!g_background || !g_imeFont) return 0;
    printf(theme_literal_161f3c, TTF_FontHeight(g_imeFont));
    return 1;
}
int freeTheme()
{
    if (g_background) {
        printf(theme_literal_161f54, g_background, g_background->w, g_background->h);
        SDL_FreeSurface(g_background);
    }
    if (g_imeFont) { printf(theme_literal_161f70, g_imeFont); TTF_CloseFont(g_imeFont); }
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
