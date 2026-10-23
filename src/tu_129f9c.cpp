// tu_129f9c (0x129f9c-0x12ce28): the theme: fonts, images, colours and sounds.
//
// A theme is a folder (Theme.h; found by loadThemeFolder) with skin/ images, sound/
// files and font settings. loadTheme copies its colours into globals, opens the five fonts
// (English or multi-language ones, depending on the language and on the CJK font being present),
// and loads every skin image, the volume/battery/Wi-Fi icons and the sounds into the globals
// below, which all drawing code uses. Anything missing from the theme is taken from the default
// skin in the current directory ("./"). freeTheme releases it all.
// Seven functions, 11,916 bytes, no generated tail.
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
// Fonts (TTF_Font*), images (from skin/), sounds and colours set by loadTheme. Several names
// describe where the value is used rather than the theme key it comes from.
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

// The file's strings as an explicit pool, named after their text, in the original order. The
// pool is needed for the battery images: Onion patched all five paths to the same hidden file
// (skin/.batt-perc.png, presumably so its own battery display replaces MainUI's), but each keeps
// its original 28- or 24-byte slot, which ordinary literals would merge.
extern const char lit_open_s_n[] __attribute__((aligned(4))) = "open %s %d\n"; // 0x161838
extern const char lit_fail_try_open_s[] __attribute__((aligned(4))) = "fail, try open %s\n"; // 0x161844
extern const char lit_s_s_open_ok[] __attribute__((aligned(4))) = "%s/%s open ok\n"; // 0x161858
extern const char lit_s_s_open_fail_use_s[] __attribute__((aligned(4))) = "%s/%s open fail, use %s/%s\n"; // 0x161868
extern const char lit_change_lang_font_close_p[] __attribute__((aligned(4))) = "change_lang_font close %p\n"; // 0x161884
extern const char lit_en_lang[] __attribute__((aligned(4))) = "en.lang"; // 0x1618a0
extern const char lit_wqy_microhei_ttc[] __attribute__((aligned(4))) = "wqy-microhei.ttc"; // 0x1618a8
extern const char lit_here[] __attribute__((aligned(4))) = "./"; // 0x1618bc
extern const char lit_henb_ttf[] __attribute__((aligned(4))) = "HENB.TTF"; // 0x1618c0
extern const char lit_exo_2_bold_italic_ttf[] __attribute__((aligned(4))) = "Exo-2-Bold-Italic.ttf"; // 0x1618cc
extern const char lit_change_lang_font_en_stitlefont_p[] __attribute__((aligned(4))) = "change_lang_font en stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n"; // 0x1618e4
extern const char lit_change_lang_font_stitlefont_p_sfonticon[] __attribute__((aligned(4))) = "change_lang_font stitlefont %p, sfonticon %p, sfontsmall %p, sfonotlabel %p\n"; // 0x161934
extern const char lit_change_font_bold_fail_s[] __attribute__((aligned(4))) = "change font bold fail: %s\n"; // 0x161984
extern const char lit_default[] __attribute__((aligned(4))) = "Default"; // 0x1619a0
extern const char lit_sys_lang_s_path_s_support[] __attribute__((aligned(4))) = "sys lang %s, path %s, support multilanguage %d\n"; // 0x1619a8
extern const char lit_sfonticon_p[] __attribute__((aligned(4))) = "sfonticon = %p\n"; // 0x1619d8
extern const char lit_init_skin_sfont_p_stitlefont_p[] __attribute__((aligned(4))) = "init_skin sfont %p stitlefont %p, sfonticon %p, sfontsmall %p, sfontlabel %p\n"; // 0x1619e8
extern const char lit_open_font_fail_s[] __attribute__((aligned(4))) = "open font fail: %s\n"; // 0x161a38
extern const char lit_open_font_bold_fail_s[] __attribute__((aligned(4))) = "open font bold fail: %s\n"; // 0x161a4c
extern const char lit_skin_background_png[] __attribute__((aligned(4))) = "skin/background.png"; // 0x161a68
extern const char lit_open_background_fail_s[] __attribute__((aligned(4))) = "open background fail: %s\n"; // 0x161a7c
extern const char lit_here_skin_div_line_h_png[] __attribute__((aligned(4))) = "./skin/div-line-h.png"; // 0x161a98
extern const char lit_open_div_line_h_png_fail[] __attribute__((aligned(4))) = "open div-line-h.png fail: %s\n"; // 0x161ab0
extern const char lit_here_skin_div_line_v_01_png[] __attribute__((aligned(4))) = "./skin/div-line-v-01.png"; // 0x161ad0
extern const char lit_open_bg_hline_fail_s[] __attribute__((aligned(4))) = "open bg_hline fail: %s\n"; // 0x161aec
extern const char lit_here_skin_dot_a_png[] __attribute__((aligned(4))) = "./skin/dot-a.png"; // 0x161b04
extern const char lit_open_dot_a_fail_s[] __attribute__((aligned(4))) = "open dot-a fail: %s\n"; // 0x161b18
extern const char lit_here_skin_dot_n_png[] __attribute__((aligned(4))) = "./skin/dot-n.png"; // 0x161b30
extern const char lit_open_dot_n_fail_s[] __attribute__((aligned(4))) = "open dot-n fail: %s\n"; // 0x161b44
extern const char lit_skin_nav_bar_item_bg_png[] __attribute__((aligned(4))) = "skin/nav-bar-item-bg.png"; // 0x161b5c
extern const char lit_open_stat_bg_fail_s[] __attribute__((aligned(4))) = "open stat_bg fail: %s\n"; // 0x161b78
extern const char lit_here_skin_bg_title_png[] __attribute__((aligned(4))) = "./skin/bg-title.png"; // 0x161b90
extern const char lit_open_bar_bg_fail_s[] __attribute__((aligned(4))) = "open bar_bg fail: %s\n"; // 0x161ba4
extern const char lit_here_skin_tips_bar_bg_png[] __attribute__((aligned(4))) = "./skin/tips-bar-bg.png"; // 0x161bbc
extern const char lit_here_skin_miyoo_topbar_png[] __attribute__((aligned(4))) = "./skin/miyoo-topbar.png"; // 0x161bd4
extern const char lit_open_mini_logo_fail_s[] __attribute__((aligned(4))) = "open mini_logo fail: %s\n"; // 0x161bec
extern const char lit_here_skin_empty_png[] __attribute__((aligned(4))) = "./skin/Empty.png"; // 0x161c08
extern const char lit_open_empty_png_fail_s[] __attribute__((aligned(4))) = "open Empty.png fail: %s\n"; // 0x161c1c
extern const char lit_here_skin_list_num_png[] __attribute__((aligned(4))) = "./skin/list-num.png"; // 0x161c38
extern const char lit_open_list_num_fail_s[] __attribute__((aligned(4))) = "open list_num fail: %s\n"; // 0x161c4c
extern const char lit_here_skin_num_bg_png[] __attribute__((aligned(4))) = "./skin/num-bg.png"; // 0x161c64
extern const char lit_open_num_bg_fail_s[] __attribute__((aligned(4))) = "open num_bg fail: %s\n"; // 0x161c78
extern const char lit_sound_change_wav[] __attribute__((aligned(4))) = "sound/change.wav"; // 0x161c90
extern const char lit_load_music_s[] __attribute__((aligned(4))) = "Load Music %s\n"; // 0x161ca4
extern const char lit_rb[] __attribute__((aligned(4))) = "rb"; // 0x161cb4
extern const char lit_here_sound_change_wav[] __attribute__((aligned(4))) = "./sound/change.wav"; // 0x161cb8
extern const char lit_load_default_music_s[] __attribute__((aligned(4))) = "Load Default Music %s\n"; // 0x161ccc
extern const char lit_load_music_s_fail_s[] __attribute__((aligned(4))) = "load music %s fail: %s\n"; // 0x161ce4
extern const char lit_sound_enter_wav[] __attribute__((aligned(4))) = "sound/enter.wav"; // 0x161cfc
extern const char lit_etc_enter_wav[] __attribute__((aligned(4))) = "/etc/enter.wav"; // 0x161d0c
extern const char lit_sound_bgm_mp3[] __attribute__((aligned(4))) = "sound/bgm.mp3"; // 0x161d1c
extern const char lit_here_sound_bgm_mp3[] __attribute__((aligned(4))) = "./sound/bgm.mp3"; // 0x161d2c
extern const char lit_skin_bg_game_item_f_png[] __attribute__((aligned(4))) = "skin/bg-game-item-f.png"; // 0x161d3c
extern const char lit_skin_bg_game_item_n_png[] __attribute__((aligned(4))) = "skin/bg-game-item-n.png"; // 0x161d54
extern const char lit_skin_bg_ra_list_item_png[] __attribute__((aligned(4))) = "skin/bg-ra-list-item.png"; // 0x161d6c
extern const char lit_skin_icon_a_54_png[] __attribute__((aligned(4))) = "skin/icon-A-54.png"; // 0x161d88
extern const char lit_skin_icon_b_54_png[] __attribute__((aligned(4))) = "skin/icon-B-54.png"; // 0x161d9c
extern const char lit_skin_ic_power_charge_100_png[] __attribute__((aligned(4))) = "skin/ic-power-charge-100%.png"; // 0x161db0
extern const char lit_battery0_path[28] __attribute__((aligned(4))) = "skin/.batt-perc.png"; // 0x161dd0
extern const char lit_battery1_path[24] __attribute__((aligned(4))) = "skin/.batt-perc.png"; // 0x161dec
extern const char lit_battery2_path[24] __attribute__((aligned(4))) = "skin/.batt-perc.png"; // 0x161e04
extern const char lit_battery3_path[24] __attribute__((aligned(4))) = "skin/.batt-perc.png"; // 0x161e1c
extern const char lit_battery4_path[24] __attribute__((aligned(4))) = "skin/.batt-perc.png"; // 0x161e34
extern const char lit_skin_icon_folder_png[] __attribute__((aligned(4))) = "skin/icon-folder.png"; // 0x161e4c
extern const char lit_skin_icon_game_png[] __attribute__((aligned(4))) = "skin/icon-game.png"; // 0x161e64
extern const char lit_skin_preview_bg_png[] __attribute__((aligned(4))) = "skin/preview-bg.png"; // 0x161e78
extern const char lit_skin_icon_hotspot_wifi_png[] __attribute__((aligned(4))) = "skin/icon-hotspot-wifi.png"; // 0x161e8c
extern const char lit_skin_icon_wifi_signal_04_png[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-04.png"; // 0x161ea8
extern const char lit_skin_icon_wifi_signal_03_png[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-03.png"; // 0x161ec8
extern const char lit_skin_icon_wifi_signal_02_png[] __attribute__((aligned(4))) = "skin/icon-wifi-signal-02.png"; // 0x161ee8
extern const char lit_skin_headphone_icon_png[] __attribute__((aligned(4))) = "skin/headphone-icon.png"; // 0x161f08
extern const char lit_skin_icon_volume_02d_png[] __attribute__((aligned(4))) = "skin/icon-volume-%02d.png"; // 0x161f20
extern const char lit_load_font_height_is_n[] __attribute__((aligned(4))) = "load font height is %d\n"; // 0x161f3c
extern const char lit_free_background_p_n_x_n[] __attribute__((aligned(4))) = "free background %p %dx%d\n"; // 0x161f54
extern const char lit_free_sfont_p[] __attribute__((aligned(4))) = "free sfont %p\n"; // 0x161f70

// Opens a font: the theme's configured file (relative to the theme root, or absolute), or
// fallbackFile in the theme root if none is configured; if that fails, fallbackFile in
// fallbackRoot. Returns the TTF_Font, or NULL.
void *openThemeFont(const std::string &root, std::string &configured,
                    const char *fallbackRoot, const char *fallbackFile, int size)
{
    std::string path(root);
    if (configured.length() != 0) {
        if (configured.at(0) != '/') path += configured;
        else path = configured;
    } else path += fallbackFile;
    printf(lit_open_s_n, path.c_str(), size);
    void *font = TTF_OpenFont(path.c_str(), size);
    if (font) return font;
    path = fallbackRoot;
    path += fallbackFile;
    printf(lit_fail_try_open_s, path.c_str());
    return TTF_OpenFont(path.c_str(), size);
}
// Loads root/file, or fallback/file if that fails (despite the name, used for all kinds of
// images). loadSkinImage is the variant most callers use.
SDL_Surface *loadImeImage(const char *root, const char *fallback, const char *file)
{
    std::string path(root);
    path += file;
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (surface) {
        printf(lit_s_s_open_ok, root, file);
        return surface;
    }
    path = fallback;
    path += file;
    printf(lit_s_s_open_fail_use_s, root, file, fallback, file);
    return IMG_Load(path.c_str());
}
// Reopens the five fonts after a language change: English uses the theme's English fonts (or
// the CJK font for the main and title text when it is installed), other languages use the
// multi-language fonts, falling back to wqy-microhei.ttc.
int changeLanguageFonts(Theme *theme, int *size)
{
    if (g_imeFont) { printf(lit_change_lang_font_close_p, g_imeFont); TTF_CloseFont(g_imeFont); g_imeFont = NULL; }
    if (g_iconFont) { printf(lit_change_lang_font_close_p, g_iconFont); TTF_CloseFont(g_iconFont); g_iconFont = NULL; }
    if (g_titleFont) { printf(lit_change_lang_font_close_p, g_titleFont); TTF_CloseFont(g_titleFont); g_titleFont = NULL; }
    if (g_smallFont) { printf(lit_change_lang_font_close_p, g_smallFont); TTF_CloseFont(g_smallFont); g_smallFont = NULL; }
    if (g_labelFont) { TTF_CloseFont(g_labelFont); g_labelFont = NULL; }
    *size = theme->mainSize;
    bool available = languageFontExists();
    if (available != true || strncmp(getLanguageFile(), lit_en_lang, 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, lit_here, lit_wqy_microhei_ttc, theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, lit_here, lit_henb_ttf, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, lit_here, lit_henb_ttf, theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, lit_here, lit_henb_ttf, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, lit_here, lit_henb_ttf, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, lit_here, lit_exo_2_bold_italic_ttf, theme->labelSize);
        printf(lit_change_lang_font_en_stitlefont_p, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, lit_here, lit_wqy_microhei_ttc, theme->titleSize);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, lit_here, lit_wqy_microhei_ttc, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, lit_here, lit_wqy_microhei_ttc, theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);
        printf(lit_change_lang_font_stitlefont_p_sfonticon, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    }
    if (!g_iconFont) fprintf(stderr, lit_change_font_bold_fail_s, SDL_GetError());
    return 0;
}
// A copy of image drawn over the matching (centred) part of the background, so translucent
// skin parts can be blitted later without the background behind them. NULL if no image.
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
// The built-in theme ("Default", in ./): font sizes and colours.
void setDefaultTheme(Theme *theme)
{
    theme->name = lit_default;
    theme->root = lit_here;
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
// Loads a theme into the globals (see the file header). Returns false if the background or the
// main font could not be loaded; main then gives up.
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
    printf(lit_sys_lang_s_path_s_support, getLanguageFile(), theme->root.c_str(), available);
    if (available != true || strncmp(getLanguageFile(), lit_en_lang, 7) == 0) {
        if (available) {
            g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, lit_here, lit_wqy_microhei_ttc, theme->titleSize);
        } else {
            g_imeFont = openThemeFont(theme->root, theme->mainFontEnglish, lit_here, lit_henb_ttf, theme->mainSize);
            g_titleFont = openThemeFont(theme->root, theme->titleFontEnglish, lit_here, lit_henb_ttf, theme->titleSize);
        }
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontEnglish, lit_here, lit_henb_ttf, theme->iconSize);
        printf(lit_sfonticon_p, g_iconFont);
        g_smallFont = openThemeFont(theme->root, theme->mainFontEnglish, lit_here, lit_henb_ttf, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontEnglish, lit_here, lit_exo_2_bold_italic_ttf, theme->labelSize);

    } else {
        g_imeFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->mainSize);
        TTF_SetFontStyle(g_imeFont, 1);
        g_iconFont = openThemeFont(theme->root, theme->iconFontMulti, lit_here, lit_wqy_microhei_ttc, theme->iconSize);
        TTF_SetFontStyle(g_iconFont, 1);
        g_titleFont = openThemeFont(theme->root, theme->titleFontMulti, lit_here, lit_wqy_microhei_ttc, theme->titleSize);
        g_smallFont = openThemeFont(theme->root, theme->mainFontMulti, lit_here, lit_wqy_microhei_ttc, theme->smallSize);
        g_labelFont = openThemeFont(theme->root, theme->labelFontMulti, lit_here, lit_wqy_microhei_ttc, theme->labelSize);
        TTF_SetFontStyle(g_labelFont, 2);

    }
    printf(lit_init_skin_sfont_p_stitlefont_p, g_imeFont, g_titleFont, g_iconFont, g_smallFont, g_labelFont);
    if (!g_imeFont) fprintf(stderr, lit_open_font_fail_s, SDL_GetError());
    if (!g_iconFont) fprintf(stderr, lit_open_font_bold_fail_s, SDL_GetError());
    g_background = loadImeImage(theme->root.c_str(), lit_here, lit_skin_background_png);
    if (g_background) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_background->w, g_background->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_background, NULL, temp, NULL);
        SDL_FreeSurface(g_background);
        g_background = temp;
    } else fprintf(stderr, lit_open_background_fail_s, SDL_GetError());
    g_listStripe = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_div_line_h_png, g_backBuffer, 0);
    if (g_listStripe) {
        SDL_Surface *temp = compositeThemeImage(g_listStripe, g_background);
        SDL_FreeSurface(g_listStripe);
        g_listStripe = temp;
    } else fprintf(stderr, lit_open_div_line_h_png_fail, SDL_GetError());
    g_vertLine = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_div_line_v_01_png, g_backBuffer, 0);
    if (g_vertLine) {
        SDL_Surface *temp = compositeThemeImage(g_vertLine, g_background);
        SDL_FreeSurface(g_vertLine);
        g_vertLine = temp;
    } else fprintf(stderr, lit_open_bg_hline_fail_s, SDL_GetError());
    g_unusedIndicator = NULL;
    g_dotActive = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_dot_a_png, g_backBuffer, 0);
    if (!g_dotActive) fprintf(stderr, lit_open_dot_a_fail_s, SDL_GetError());
    g_dotNormal = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_dot_n_png, g_backBuffer, 0);
    if (!g_dotNormal) fprintf(stderr, lit_open_dot_n_fail_s, SDL_GetError());
    g_unusedStatus = NULL;
    g_navItem = loadImeImage(theme->root.c_str(), lit_here, lit_skin_nav_bar_item_bg_png);
    if (g_navItem) {
        SDL_Surface *temp = SDL_CreateRGBSurface(1, g_navItem->w, g_navItem->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
        SDL_UpperBlit(g_navItem, NULL, temp, NULL);
        SDL_FreeSurface(g_navItem);
        g_navItem = temp;
    } else fprintf(stderr, lit_open_stat_bg_fail_s, SDL_GetError());
    g_titleBar = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_bg_title_png, g_backBuffer, 1);
    if (g_titleBar) {
        SDL_Surface *temp = compositeThemeImage(g_titleBar, g_background);
        SDL_FreeSurface(g_titleBar);
        g_titleBar = temp;
    } else fprintf(stderr, lit_open_bar_bg_fail_s, SDL_GetError());
    g_keySettingFooter = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_tips_bar_bg_png, g_backBuffer, 0);
    if (g_keySettingFooter) {
        SDL_Surface *temp = compositeThemeImage(g_keySettingFooter, g_background);
        SDL_FreeSurface(g_keySettingFooter);
        g_keySettingFooter = temp;
    } else fprintf(stderr, lit_open_bar_bg_fail_s, SDL_GetError());
    g_miniLogo = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_miyoo_topbar_png, g_backBuffer, 0);
    if (!g_miniLogo) fprintf(stderr, lit_open_mini_logo_fail_s, SDL_GetError());
    g_emptyImage = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_empty_png, g_backBuffer, 0);
    if (!g_emptyImage) fprintf(stderr, lit_open_empty_png_fail_s, SDL_GetError());
    g_listNumber = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_list_num_png, g_backBuffer, 0);
    if (!g_listNumber) fprintf(stderr, lit_open_list_num_fail_s, SDL_GetError());
    g_numberBg = loadSkinImage(theme->root.c_str(), lit_here, lit_here_skin_num_bg_png, g_backBuffer, 0);
    if (!g_numberBg) fprintf(stderr, lit_open_num_bg_fail_s, SDL_GetError());
    std::string path(theme->root);
    path += lit_sound_change_wav;
    printf(lit_load_music_s, path.c_str());
    g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), lit_rb), 1);
    if (!g_changeSound) {
        printf(lit_load_default_music_s, lit_here_sound_change_wav);
        g_changeSound = Mix_LoadWAV_RW(SDL_RWFromFile(lit_here_sound_change_wav, lit_rb), 1);
        if (!g_changeSound) fprintf(stderr, lit_load_music_s_fail_s, lit_here_sound_change_wav, SDL_GetError());
    }
    path = theme->root;
    path += lit_sound_enter_wav;
    printf(lit_load_music_s, path.c_str());
    g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile(path.c_str(), lit_rb), 1);
    if (!g_enterSound) {
        printf(lit_load_default_music_s, lit_etc_enter_wav);
        g_enterSound = Mix_LoadWAV_RW(SDL_RWFromFile(lit_etc_enter_wav, lit_rb), 1);
        if (!g_enterSound) fprintf(stderr, lit_load_music_s_fail_s, lit_etc_enter_wav, SDL_GetError());
    }
    path = theme->root;
    path += lit_sound_bgm_mp3;
    printf(lit_load_music_s, path.c_str());
    g_themeMusic = Mix_LoadMUS(path.c_str());
    if (!g_themeMusic) {
        printf(lit_load_default_music_s, lit_here_sound_bgm_mp3);
        g_themeMusic = Mix_LoadMUS(lit_here_sound_bgm_mp3);
        if (!g_themeMusic) fprintf(stderr, lit_load_music_s_fail_s, lit_here_sound_bgm_mp3, SDL_GetError());
    }
    g_gameFocused = loadImeImage(theme->root.c_str(), lit_here, lit_skin_bg_game_item_f_png);
    if (g_gameFocused) {
        SDL_Surface *temp = compositeThemeImage(g_gameFocused, g_background);
        SDL_FreeSurface(g_gameFocused);
        g_gameFocused = temp;
    }
    g_gameNormal = loadImeImage(theme->root.c_str(), lit_here, lit_skin_bg_game_item_n_png);
    if (g_gameNormal) {
        SDL_Surface *temp = compositeThemeImage(g_gameNormal, g_background);
        SDL_FreeSurface(g_gameNormal);
        g_gameNormal = temp;
    }
    g_raRow = loadImeImage(theme->root.c_str(), lit_here, lit_skin_bg_ra_list_item_png);
    if (g_raRow) {
        SDL_Surface *temp = compositeThemeImage(g_raRow, NULL);
        SDL_FreeSurface(g_raRow);
        g_raRow = temp;
    }
    g_iconA = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_a_54_png, g_backBuffer, 0);
    g_iconB = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_b_54_png, g_backBuffer, 0);
    g_chargeIcon = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_ic_power_charge_100_png, g_backBuffer, 0);
    g_battery0 = loadSkinImage(theme->root.c_str(), lit_here, lit_battery0_path, g_backBuffer, 0);
    g_battery1 = loadSkinImage(theme->root.c_str(), lit_here, lit_battery1_path, g_backBuffer, 0);
    g_battery2 = loadSkinImage(theme->root.c_str(), lit_here, lit_battery2_path, g_backBuffer, 0);
    g_battery3 = loadSkinImage(theme->root.c_str(), lit_here, lit_battery3_path, g_backBuffer, 0);
    g_battery4 = loadSkinImage(theme->root.c_str(), lit_here, lit_battery4_path, g_backBuffer, 0);
    g_folderIcon = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_folder_png, g_backBuffer, 0);
    g_gameIcon = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_game_png, g_backBuffer, 0);
    g_previewBg = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_preview_bg_png, g_backBuffer, 0);
    g_hotspotIcon = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_hotspot_wifi_png, g_backBuffer, 0);
    g_wifi4 = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_wifi_signal_04_png, g_backBuffer, 0);
    g_wifi3 = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_wifi_signal_03_png, g_backBuffer, 0);
    g_wifi2 = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_icon_wifi_signal_02_png, g_backBuffer, 0);
    g_headphoneIcon = loadSkinImage(theme->root.c_str(), lit_here, lit_skin_headphone_icon_png, g_backBuffer, 0);
    for (int i = 0; i < 21; i++) {
        snprintf(filename, 128, lit_skin_icon_volume_02d_png, i);
        g_volumeIcons[i] = loadSkinImage(theme->root.c_str(), lit_here, filename, g_backBuffer, 0);
    }
    if (!g_background || !g_imeFont) return 0;
    printf(lit_load_font_height_is_n, TTF_FontHeight(g_imeFont));
    return 1;
}
// Frees every font, image and sound loadTheme loaded.
int freeTheme()
{
    if (g_background) {
        printf(lit_free_background_p_n_x_n, g_background, g_background->w, g_background->h);
        SDL_FreeSurface(g_background);
    }
    if (g_imeFont) { printf(lit_free_sfont_p, g_imeFont); TTF_CloseFont(g_imeFont); }
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
