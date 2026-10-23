// MainUI.cpp (0x15b20-0x49c3c): the main user interface, MainUI's largest source file.
//
// Drawing helpers, the window stack and loop, every menu and dialog, launching, favorites and the
// menu factory, in address order. Sections marked with ==== lines group neighboring functions;
// MainUI.h holds the declarations.
//
// Header/inline definition order reproduces the 798-function template tail (see
// docs/internals/unit-workflow.md, "Tail order").
// <string> supplies the first char_traits helpers; <iostream> follows the UI declarations so
// its system-error string helpers do not precede the ListMenu/TextMenu vector instantiations.
// The static init (0x39228) constructs std::ios_base::Init and then the pending window queue.
#include <string>
#include <atomic>
#include <vector>
#include "MainUI.h"
#include <iostream>

// 0x17ff3c, the only global with a constructor (ctor 0x3b130, dtor 0x49be0 via __aeabi_atexit).
// Declared after the headers and before the body to preserve template first-use order.
std::vector<TrimusWindow *> *g_windowStack; // 0x17ff38
std::vector<TrimusWindow *> g_pendingWindows;

// Global variables. Each comment gives the address in the original binary.
unsigned char g_deviceId[5]; // 0x17ff48: the five device-id bytes; the full size is unknown
unsigned int g_unrefBss_17ff50; // 0x17ff50: unreferenced storage at this address in the Onion and stock builds; size and position proven, name, type and partition unknown
char g_launchCmd[2048]; // 0x17ff54
char g_countBuffer[256]; // 0x180754
SDL_Thread *g_loadingThread; // 0x180854
// These TU-local objects follow iostream's local .comm initializer; making them
// public moved them ahead of it. Original target addresses are listed below.
static fb_var_screeninfo g_fbVar; // 0x18085c
static bool g_settingsDirty; // 0x1808fc
static int g_powerDownCount; // 0x180900
static bool g_powerSent; // 0x180904
static bool g_startHeld; // 0x180905
static bool g_selectHeld; // 0x180906
static bool g_volumeIndicatorVisible; // 0x180907
static char g_cwdBuf[4096]; // 0x180908
static volatile int g_busy; // 0x181908
static volatile int g_progressCur; // 0x18190c
static volatile int g_progressTotal; // 0x181910
static unsigned char g_charging; // 0x181914
int g_fbFd = -1; // 0x17e488


// =================================================================================================
// 0x15b20-0x18c54: Runtime helpers, text drawing, the status bar, global input, launching.
// =================================================================================================

// 0x15b20: name of a key or event code (no caller in the binary).
const char *keyEventName(int key)
{
    switch (key) {
        case 0: return "key_none";
        case 1: return "key_a";
        case 2: return "key_b";
        case 3: return "key_x";
        case 4: return "key_y";
        case 5: return "key_up";
        case 6: return "key_down";
        case 7: return "key_left";
        case 8: return "key_right";
        case 9: return "key_start";
        case 10: return "key_coin";
        case 11: return "key_nenu_key";
        case 12: return "key_hotkey1";
        case 13: return "key_hotkey2";
        case 14: return "key_hotkey3";
        case 15: return "key_hotkey4";
        case 18: return "event_battery";
        case 19: return "event_hotplug";
        case 20: return "event_notify";
        case 45: return "key_max_key";
        default: return "event_unkown";
    }
}

// 0x15d00: show the frame (SDL double buffering).
void flipDisplay()
{
    SDL_Flip(g_screen);
}

// 0x15d24: draws `text` in `rect` (align 0 left, 1 centered, 3 vertically centered, 4 right and
// vertically centered, else right), optionally over a shadow; returns the text width. The shadow blit
// was patched to a NOP by Onion; the asm block below reproduces that patch at its offset.
asm(R"(
.set .Ldraw_text_nops, 0
.macro bl target
    .set .Lskip_patched_call, 0
    .irp patch, 0x2b0
        .if (. - _Z8drawTextP11SDL_SurfacePv8SDL_RectPKc9SDL_Colorib) == \patch
            .set .Lskip_patched_call, 1
        .endif
    .endr
    .if .Lskip_patched_call
        .inst 0xe320f000
        .set .Ldraw_text_nops, .Ldraw_text_nops + 1
    .else
        .inst 0xebfffffe
        .reloc . - 4, R_ARM_CALL, \target
    .endif
.endm
)");
int drawText(SDL_Surface *dst, TTF_Font *font, SDL_Rect rect, const char *text, SDL_Color color,
             int align, bool shadow)
{
    if (font == NULL)
        return 0;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (s == NULL)
        return 0;
    SDL_Rect r;
    if (align == 1) {
        r.x = rect.x + (rect.w - s->w) / 2;
        r.y = rect.y + (rect.h - s->h) / 2;
    } else if (align == 0) {
        r.x = rect.x;
        r.y = rect.y;
    } else if (align == 3) {
        r.x = rect.x;
        r.y = rect.y + (rect.h - s->h) / 2;
    } else if (align == 4) {
        r.x = rect.x + rect.w - s->w;
        r.y = rect.y + (rect.h - s->h) / 2;
    } else {
        r.x = rect.x + rect.w - s->w;
        r.y = rect.y;
    }
    r.w = s->w;
    r.h = s->h;
    if (shadow) {
        SDL_Surface *sh = TTF_RenderUTF8_Blended(font, text, g_clearColor);
        SDL_Rect sr = r;
        sr.x += 1;
        sr.y += 2;
        SDL_BlitSurface(sh, NULL, dst, &sr);
        SDL_FreeSurface(sh);
    }
    SDL_BlitSurface(s, NULL, dst, &r);
    int w = s->w;
    SDL_FreeSurface(s);
    return w;
}

asm(R"(
.purgem bl
.if .Ldraw_text_nops != 1
    .error "target NOP patch count changed"
.endif
)");

// 0x16018: splits `text` at `delim` and draws the pieces as rows of equal height.
int drawTextLines(SDL_Surface *dst, TTF_Font *font, SDL_Rect *rect, const char *text,
                  const char *delim, SDL_Color *color, int align, bool shadow)
{
    char *buf = strdup(text);
    if (buf != NULL) {
        std::vector<char *> lines;
        char *tok = NULL;
        tok = strtok(buf, delim);
        while (tok != NULL) {
            lines.push_back(tok);
            tok = strtok(NULL, delim);
        }
        if (lines.size() > 0) {
            int lineH = rect->h / lines.size();
            SDL_Rect r = *rect;
            r.h = lineH;
            for (int i = 0; i < lines.size(); i++) {
                r.y = rect->y + i * lineH;
                drawText(dst, font, r, lines[i], *color, align, shadow);
            }
        }
        free(buf);
    }
    return 0;
}

// 0x1622c: blits `bg` at `rect` and `icon` centered on it (or centered in 48x48 without `bg`).
void drawIconOnBg(SDL_Surface *dst, SDL_Surface *bg, SDL_Rect *rect, SDL_Surface *icon)
{
    if (bg != NULL) {
        SDL_Rect r = *rect;
        SDL_BlitSurface(bg, NULL, dst, &r);
        if (icon != NULL) {
            r.x = rect->x + (bg->w - icon->w) / 2;
            r.y = rect->y + (bg->h - icon->h) / 2;
            SDL_BlitSurface(icon, NULL, dst, &r);
        }
    } else if (icon != NULL) {
        SDL_Rect r = *rect;
        r.x = rect->x + (48 - icon->w) / 2;
        r.y = rect->y + (48 - icon->h) / 2;
        SDL_BlitSurface(icon, NULL, dst, &r);
    }
}

extern const char g_skinSubdir[] = "./"; // Original pooled root at 0x151a48.

// 0x163d0: as drawIconOnBg, loading the icon from the theme's skin folder.
void drawSkinIconOnBg(SDL_Surface *dst, SDL_Surface *bg, SDL_Rect *rect, const char *file)
{
    SDL_Surface *icon = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    drawIconOnBg(dst, bg, rect, icon);
    SDL_FreeSurface(icon);
}

// 0x16450: blits `bg` at `rect` and centers `text` on it.
void drawTextOnBg(SDL_Surface *dst, SDL_Surface *bg, SDL_Rect *rect, const char *text)
{
    if (bg != NULL) {
        int h = TTF_FontHeight(g_imeFont);
        SDL_Rect r = *rect;
        SDL_BlitSurface(bg, NULL, dst, &r);
        if (text != NULL) {
            r.x = rect->x + 1;
            r.y = rect->y;
            r.w = bg->w - 1;
            r.h = bg->h;
            drawText(dst, g_imeFont, r, text, g_selectedColor, 1, false);
        }
    }
}

// 0x1656c: status-bar icon for key shm slot 0, drawn right to left at position `slot`;
// returns the next slot, or -1 when there is nothing to draw.
int drawKeyStateIndicator(SDL_Surface *dst, int slot, bool focused)
{
    int v = GetKeyShm(g_keyShm, MONITOR_VOLUME);
    if (v < 0)
        v = 0;
    if (v > 20)
        v = 20;
    if (g_volumeIcons[v] == NULL)
        return -1;
    int w;
    int h;
    if (g_unusedStatus != NULL) {
        w = g_unusedStatus->w;
        h = g_unusedStatus->h;
    } else {
        w = 48;
        h = 48;
    }
    SDL_Rect r;
    r.x = 620 - ((w + 20) * slot + w);
    r.y = (60 - h) / 2;
    r.w = w;
    r.h = h;
    if (g_background != NULL) {
        SDL_Rect src = r;
        SDL_BlitSurface(g_background, &src, dst, &r);
    } else {
        SDL_FillRect(dst, &r, 0x109d);
    }
    if (g_titleBar != NULL && focused) {
        SDL_Rect src = r;
        SDL_BlitSurface(g_titleBar, &src, dst, &r);
    }
    drawIconOnBg(dst, g_unusedStatus, &r, g_volumeIcons[v]);
    return slot + 1;
}

// 0x16788: Wi-Fi status icon (hotspot, or signal level when connected). The background blits
// were patched to NOPs by Onion; the asm block below reproduces those patches at their offsets.
asm(R"(
.set .Ldraw_wifi_nops, 0
.macro bl target
    .set .Lskip_patched_call, 0
    .irp patch, 0x12c, 0x144, 0x190, 0x2a4, 0x2bc, 0x308
        .if (. - _Z17drawWifiIndicatorP11SDL_Surfaceib) == \patch
            .set .Lskip_patched_call, 1
        .endif
    .endr
    .if .Lskip_patched_call
        .inst 0xe320f000
        .set .Ldraw_wifi_nops, .Ldraw_wifi_nops + 1
    .else
        .inst 0xebfffffe
        .reloc . - 4, R_ARM_CALL, \target
    .endif
.endm
)");
int drawWifiIndicator(SDL_Surface *dst, int slot, bool focused)
{
    int w = 48;
    int h = 48;
    if (g_unusedStatus != NULL) {
        w = g_unusedStatus->w;
        h = g_unusedStatus->h;
    }
    int mode = wifiMode();
    if (mode == 3) {
        SDL_Rect r;
        r.x = 620 - ((w + 20) * slot + w);
        r.y = (60 - h) / 2;
        r.w = w;
        r.h = h;
        if (g_background != NULL) {
            SDL_Rect src = r;
            SDL_BlitSurface(g_background, &src, dst, &r);
        } else {
            SDL_FillRect(dst, &r, 0x109d);
        }
        if (g_titleBar != NULL && focused) {
            SDL_Rect src = r;
            SDL_BlitSurface(g_titleBar, &src, dst, &r);
        }
        drawIconOnBg(dst, g_unusedStatus, &r, g_hotspotIcon);
        return slot + 1;
    } else if (mode == 2) {
        WifiStatus *st = getWifiStatus();
        if (st != NULL && st->state == 4) {
            SDL_Rect r;
            r.x = 620 - ((w + 20) * slot + w);
            r.y = (60 - h) / 2;
            r.w = w;
            r.h = h;
            if (g_background != NULL) {
                SDL_Rect src = r;
                SDL_BlitSurface(g_background, &src, dst, &r);
            } else {
                SDL_FillRect(dst, &r, 0x109d);
            }
            if (g_titleBar != NULL && focused) {
                SDL_Rect src = r;
                SDL_BlitSurface(g_titleBar, &src, dst, &r);
            }
            int level = wifiSignalLevel(st->rssi);
            if (level > 2)
                drawIconOnBg(dst, g_unusedStatus, &r, g_wifi4);
            else if (level > 1)
                drawIconOnBg(dst, g_unusedStatus, &r, g_wifi3);
            else
                drawIconOnBg(dst, g_unusedStatus, &r, g_wifi2);
            return slot + 1;
        }
    }
    return -1;
}

asm(R"(
.purgem bl
.if .Ldraw_wifi_nops != 6
    .error "target NOP patch count changed"
.endif
)");

// 0x16b54: headphone icon in slot 2 when sound card 2 has a DSP.
int drawSoundCardIndicator(SDL_Surface *dst, bool focused)
{
    int w = 48;
    int h = 48;
    if (g_unusedStatus != NULL) {
        w = g_unusedStatus->w;
        h = g_unusedStatus->h;
    }
    if (hasDsp1(2) > 0) {
        SDL_Rect r;
        r.x = 620 - (w + 20) * 2 - w;
        r.y = (60 - h) / 2;
        r.w = w;
        r.h = h;
        printf("draw sound card at %d,%d\n", r.x, r.y);
        drawIconOnBg(dst, g_unusedStatus, &r, g_headphoneIcon);
        return 2;
    }
    return -1;
}

// 0x16c98: battery icon in slot 0.
int drawBatteryIndicator(SDL_Surface *dst, bool focused)
{
    int w;
    int h;
    int unused = 0;
    int battery = 100;
    int voltage = 0;
    int status = 0;
    readAxpStatus(&battery, &voltage, &status);
    bool charging = status == 2 || status == 3;
    if (g_unusedStatus != NULL) {
        w = g_unusedStatus->w;
        h = g_unusedStatus->h;
    } else {
        w = 48;
        h = 48;
    }
    SDL_Rect r;
    r.x = 620 - w;
    r.y = (60 - h) / 2;
    r.w = w;
    r.h = h;
    if (charging)
        drawIconOnBg(dst, g_unusedStatus, &r, g_chargeIcon);
    else if (battery > 69)
        drawIconOnBg(dst, g_unusedStatus, &r, g_battery0);
    else if (battery > 44)
        drawIconOnBg(dst, g_unusedStatus, &r, g_battery1);
    else if (battery > 14)
        drawIconOnBg(dst, g_unusedStatus, &r, g_battery2);
    else if (battery > 4)
        drawIconOnBg(dst, g_unusedStatus, &r, g_battery3);
    else
        drawIconOnBg(dst, g_unusedStatus, &r, g_battery4);
    return 1;
}

// 0x16ee0: plays the enter sound on channel 0.
int playEnterSound(int, int)
{
    if (g_enterSound != NULL) {
        if (Mix_PlayChannel(0, g_enterSound, 0) == -1)
            printf("Mix_PlayChunk: %s\n", SDL_GetError());
    }
    return 0;
}

// 0x16f68: title bar, with the logo for "MIYOO" (or no title) and the title text otherwise.
void drawTitleBar(SDL_Surface *dst, const char *title, bool showTitle)
{
    SDL_Rect r;
    if (g_titleBar != NULL)
        SDL_BlitSurface(g_titleBar, NULL, dst, NULL);
    if (showTitle) {
        if ((title != NULL && strcasecmp(title, "MIYOO") != 0) || g_miniLogo == NULL) {
            int h = TTF_FontHeight(g_titleFont);
            r.x = 20;
            r.y = (60 - h) / 2;
            r.w = 600;
            r.h = h;
            drawText(dst, g_titleFont, r, title, g_wifiEmptyColor, 1, true);
        } else {
            r.x = 20;
            r.y = (60 - g_miniLogo->h) / 2;
            SDL_BlitSurface(g_miniLogo, NULL, dst, &r);
        }
    }
}

// 0x170f8: an icon (optionally drawn over a skin image) followed by a label; returns the width.
int drawIconLabel(SDL_Surface *dst, SDL_Surface *icon, int x, int y, const char *file,
                  const char *text)
{
    if (icon != NULL) {
        TTF_Font *font = g_labelFont;
        if (font == NULL)
            font = g_imeFont;
        int fh = TTF_FontHeight(font);
        SDL_Rect r;
        r.x = x;
        r.y = (60 - icon->h) / 2 + y;
        if (file != NULL)
            drawSkinIconOnBg(dst, icon, &r, file);
        else
            SDL_BlitSurface(icon, NULL, dst, &r);
        r.x = icon->w + x + 5;
        r.y = (60 - fh) / 2 + y;
        int tw = drawText(dst, font, r, text, g_labelColor, 0, false);
        return icon->w + 5 + tw;
    }
    return 0;
}

// 0x172a0: footer bar with the A and B button hints.
void drawFooter(SDL_Surface *dst, const char *textA, const char *textB)
{
    TTF_Font *font = g_labelFont;
    if (font == NULL)
        font = g_imeFont;
    int fh = TTF_FontHeight(font);
    int unused = 0;
    int y = 420;
    int x = 20;
    if (g_keySettingFooter != NULL) {
        SDL_Rect dr;
        dr.x = 0;
        dr.y = y;
        SDL_Rect sr;
        sr.x = 0;
        sr.y = 0;
        sr.w = g_keySettingFooter->w;
        sr.h = g_keySettingFooter->h;
        SDL_BlitSurface(g_keySettingFooter, &sr, dst, &dr);
    }
    x = 20;
    if (textA != NULL && g_iconA != NULL)
        x += drawIconLabel(dst, g_iconA, x, y, NULL, textA) + 30;
    if (textB != NULL && g_iconB != NULL)
        drawIconLabel(dst, g_iconB, x, y, NULL, textB);
}

// 0x17450: screen background and bottom bar (or a plain fill).
void drawBackground(SDL_Surface *dst)
{
    if (g_background != NULL) {
        SDL_BlitSurface(g_background, NULL, dst, NULL);
        if (g_unusedIndicator != NULL) {
            SDL_Rect r;
            r.x = 0;
            r.y = 480 - g_unusedIndicator->h;
            if (r.y < 0)
                r.y = 0;
            r.w = g_unusedIndicator->w;
            r.h = g_unusedIndicator->h;
            SDL_BlitSurface(g_unusedIndicator, NULL, dst, &r);
        }
    } else {
        SDL_Rect r = {0, 0, 640, 480};
        SDL_FillRect(dst, &r, 0x109d);
    }
}

// 0x17568: background, title bar, list stripe and the standard footer.
void drawScreenFrame(SDL_Surface *dst, const char *title, int unusedArg, bool showTitle)
{
    int unused = 0;
    drawBackground(dst);
    drawTitleBar(dst, title, showTitle);
    if (g_listStripe != NULL) {
        SDL_Rect r;
        r.x = 0;
        r.y = 60;
        SDL_BlitSurface(g_listStripe, NULL, dst, &r);
    }
    drawFooter(dst, languageString(STR_OPEN_LABEL), languageString(STR_BACK));
}

extern const char g_powerOffCommand[32];

// 0x1761c: saves pending settings, then runs the shutdown commands; returns 2 (quit).
int powerOff(void *arg)
{
    if (g_settingsDirty)
        saveSystemJson(g_keyShm, g_themePath, g_fontSize);
    puts("poweroff2");
    system("sync");
    system("sleep 1");
    system(g_powerOffCommand);
    return 2;
}

// Patched command retains its original 32-byte slot at 0x151aa8.
extern const char g_powerOffCommand[32] = "mainui_off_order";

// 0x176a8: the power-off countdown's end: power off.
void powerOffNow()
{
    powerOff(0);
}

// 0x176c0: asks the window loop to power off (EVENT_POWER_OFF); MainUI then shows a countdown.
int sendPowerOffEvent(int, int)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = EVENT_POWER_OFF;
    puts("send poweroff event");
    if (SDL_PushEvent(&event) < 0)
        puts("Failed to push poweroff SDL event");
    return 0;
}

// 0x1772c: called for every event except status notifications; empty in this build.
void onUserActivity()
{
}

// 0x17744: the one place where windows get input. Waits for the next SDL event, copies it to
// `out` (when not NULL) and translates it into a MainButton code; user events come back as their
// UserEventCode. Returns BUTTON_NONE (0) for events a window can ignore.
//
// Buttons report on release, except the arrows, L1/R1 and L2/R2, which report on press (so they
// can repeat). Along the way it:
// - plays the "change" sound on each press (not for power or L1/R1);
// - tracks SELECT and START being held: L2/R2 are swallowed while either is held;
// - opens the power-off dialog once the power key has been held for over a second (the key
//   repeats about every 100 ms) and returns EVENT_POWER_DIALOG;
// - reopens the audio device when one is plugged in or out (EVENT_HOTPLUG), opens the power-off
//   countdown on EVENT_POWER_OFF and shows/hides the volume indicator (EVENT_VOLUME_SHOWN/HIDDEN);
// - turns a tap at a screen edge into an arrow button (touch-panel devices only).
int handleGlobalInput(SDL_Event *out)
{
    SDL_Event event;
    if (SDL_WaitEvent(&event)) {
        // Status updates from MainUI's own threads do not count as user activity.
        if (event.type != SDL_USEREVENT || (event.user.code != EVENT_BATTERY && event.user.code != EVENT_WIFI_STATUS &&
                                            event.user.code != EVENT_VOLUME_SHOWN && event.user.code != EVENT_VOLUME_HIDDEN &&
                                            event.user.code != 0))
            onUserActivity();
        if (out != NULL)
            *out = event;
        switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym != MIYOO_KEY_POWER && event.key.keysym.sym != MIYOO_KEY_R1 &&
                    event.key.keysym.sym != MIYOO_KEY_L1 && g_changeSound != NULL) {
                    if (Mix_PlayChannel(1, g_changeSound, 0) == -1)
                        printf("Mix_PlayChunk: %s\n", SDL_GetError());
                }
                printf("%lu keydown event %d\n", milliseconds(), event.key.keysym.sym);
                switch (event.key.keysym.sym) {
                    case MIYOO_KEY_UP:
                        return BUTTON_UP;
                    case MIYOO_KEY_DOWN:
                        return BUTTON_DOWN;
                    case MIYOO_KEY_LEFT:
                        return BUTTON_LEFT;
                    case MIYOO_KEY_RIGHT:
                        return BUTTON_RIGHT;
                    case MIYOO_KEY_SELECT:
                        g_selectHeld = true;
                        break;
                    case MIYOO_KEY_START:
                        g_startHeld = true;
                        break;
                    case MIYOO_KEY_L2:
                        if (!g_startHeld && !g_selectHeld)
                            return BUTTON_HOTKEY1;
                        break;
                    case MIYOO_KEY_R2:
                        if (!g_startHeld && !g_selectHeld)
                            return BUTTON_HOTKEY2;
                        break;
                    case MIYOO_KEY_L1:
                        return BUTTON_HOTKEY3;
                    case MIYOO_KEY_R1:
                        return BUTTON_HOTKEY4;
                    case MIYOO_KEY_POWER:
                        // Counts key repeats while held; the dialog opens once per hold.
                        g_powerDownCount++;
                        printf("SDLK_POWER down %d, sent %d\n", g_powerDownCount, g_powerSent);
                        if ((g_powerDownCount - 1) * 100 > 1000 && !g_powerSent) {
                            g_powerSent = true;
                            TrimusWindow *w = new DialogWindow(STR_POWER_OFF, STR_SHUT_DOWN_TEXT, false,
                                                               new GeneralAction(-1, powerOff, NULL));
                            pushWindow(w);
                            return EVENT_POWER_DIALOG;
                        }
                        break;
                    default:
                        printf("event.key.keysym.sym = %d\n", event.key.keysym.sym);
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case MIYOO_KEY_MENU:
                        return BUTTON_MENU;
                    case MIYOO_KEY_Y:
                        return BUTTON_Y;
                    case MIYOO_KEY_A:
                        return BUTTON_A;
                    case MIYOO_KEY_B:
                        return BUTTON_B;
                    case MIYOO_KEY_X:
                        return BUTTON_X;
                    case MIYOO_KEY_START:
                        g_startHeld = false;
                        puts("return KEY_START");
                        return BUTTON_START;
                    case MIYOO_KEY_SELECT:
                        g_selectHeld = false;
                        puts("return KEY_SELECT");
                        return BUTTON_SELECT;
                    case MIYOO_KEY_POWER:
                        g_powerDownCount = 0;
                        g_powerSent = false;
                        break;
                    case MIYOO_KEY_LSUPER:
                        break;
                    case MIYOO_KEY_RSUPER:
                        break;
                    default:
                        break;
                }
                break;
            case SDL_USEREVENT:
                if (event.user.code == EVENT_HOTPLUG) {
                    // Reopen audio on the right device (/dev/dsp1 when a second one is present) at
                    // 48 kHz, signed 16-bit, stereo, and restart the theme music.
                    setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
                    Mix_CloseAudio();
                    if (hasDsp1(2) > 0)
                        putenv((char *)"AUDIODEV=/dev/dsp1");
                    else
                        putenv((char *)"AUDIODEV=/dev/dsp");
                    if (Mix_OpenAudio(48000, 0x8010, 2, 128) < 0) {   // 0x8010 = AUDIO_S16LSB
                        printf("open auido fail: %s\n", SDL_GetError());
                        Mix_PauseMusic();
                    } else {
                        Mix_PlayMusic(g_themeMusic, -1);
                    }
                } else if (event.user.code == EVENT_POWER_OFF) {
                    TrimusWindow *w = new CountDownWindow(STR_POWER_OFF_IN, powerOffNow, onUserActivity);
                    pushWindow(w);
                } else if (event.user.code == EVENT_VOLUME_SHOWN) {
                    g_volumeIndicatorVisible = true;
                } else if (event.user.code == EVENT_VOLUME_HIDDEN) {
                    g_volumeIndicatorVisible = false;
                }
                return event.user.code;
            case SDL_MOUSEBUTTONUP:
                // Same edge bands as the touch thread (KeySettingWindow.cpp).
                if (event.button.x < 100 && event.button.y > 100 && event.button.y < 380)
                    return BUTTON_LEFT;
                if (event.button.x > 540 && event.button.y > 100 && event.button.y < 380)
                    return BUTTON_RIGHT;
                if (event.button.y < 100 && event.button.x > 100 && event.button.x < 540)
                    return BUTTON_UP;
                if (event.button.y > 380 && event.button.x > 100 && event.button.x < 540)
                    return BUTTON_DOWN;
            default:
                break;
        }
    }
    return BUTTON_NONE;
}

// 0x17f10: scandir comparator by name, ignoring case.
int compareDirentNames(const dirent **a, const dirent **b)
{
    const char *na = (*a)->d_name;
    const char *nb = (*b)->d_name;
    return strcasecmp(na, nb);
}

// 0x17f60: comparator by the leading std::string member (no caller found; the record type is
// a guess, AppInfo::label).
int compareLabels(const void *a, const void *b)
{
    const AppInfo *x = (const AppInfo *)a;
    const AppInfo *y = (const AppInfo *)b;
    return strcmp(x->label.c_str(), y->label.c_str());
}

// 0x17fc0: writes the command MainUI leaves for the shell script that runs it (MainUI exits next).
void writeCmdToRun(const char *cmd)
{
    FILE *f = fopen("/tmp/cmd_to_run.sh", "w+");
    if (f != NULL) {
        fputs(cmd, f);
        fclose(f);
    }
}

// 0x18018: shows the "please wait" screen, pans the framebuffer back to the first page and
// writes the command MainUI leaves for the shell.
void showLaunchScreen(const char *cmd)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 60;
    r.w = 640;
    r.h = 360;
    drawBackground(g_backBuffer);
    drawText(g_backBuffer, g_imeFont, r, languageString(STR_LOADING_CAPS), g_wifiEmptyColor, 1, false);
    SDL_BlitSurface(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    SDL_BlitSurface(g_backBuffer, NULL, g_screen, NULL);
    g_fbVar.yoffset = 0;
    if (g_fbFd >= 0)
        ioctl(g_fbFd, FBIOPAN_DISPLAY, &g_fbVar);
    writeCmdToRun(cmd);
}

// 0x18170: an action that does nothing (its original name, from __func__, includes the typo).
int donoting_action(void *, void *)
{
    puts(__func__);
    return 1;
}

// 0x181a0: builds the ROM launch command (through libpadsp.so unless the audio fix is off),
// shows the launch screen and records the game in the recent list; returns 3 (exit MainUI), or -1
// after opening the swap dialog when the emulator needs swap and none is enabled.
int launchRom(const char *label, EmuInfo *emu, const char *rompath, const char *imgpath, int type)
{
    bool audioFix = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX) != 0;
    char *cwd = getcwd(g_cwdBuf, 4096);
    if (audioFix && emu->noAudioFix != 0) {
        audioFix = false;
        system("touch /tmp/no_audioserver");
    }
    if (emu->useSwap != 0) {
        int swap = swapSize();
        if (swap > 0) {
            if (audioFix) {
                if (cwd != NULL)
                    snprintf(g_launchCmd, 2048, "LD_PRELOAD=%s/../lib/libpadsp.so \"%s\" \"%s\"", cwd,
                             emu->launch.c_str(), rompath);
                else
                    snprintf(g_launchCmd, 2048, "LD_PRELOAD=/customer/lib/libpadsp.so \"%s\" \"%s\"",
                             emu->launch.c_str(), rompath);
            } else {
                snprintf(g_launchCmd, 2048, "\"%s\" \"%s\"", emu->launch.c_str(), rompath);
            }
            printf("Load rom %s\n", g_launchCmd);
            encrypt_path(rompath, strlen(rompath));
            showLaunchScreen(g_launchCmd);
        } else {
            ProgressDialog *d = new ProgressDialog(STR_CREATE_CACHE, new GeneralAction(-1, enableSwapAction, NULL));
            d->onShow();
            g_windowStack->push_back(d);
            return -1;
        }
    } else {
        if (audioFix) {
            if (cwd != NULL)
                snprintf(g_launchCmd, 2048, "LD_PRELOAD=%s/../lib/libpadsp.so  \"%s\" \"%s\"", cwd,
                         emu->launch.c_str(), rompath);
            else
                snprintf(g_launchCmd, 2048, "LD_PRELOAD=/customer/lib/libpadsp.so \"%s\" \"%s\"",
                         emu->launch.c_str(), rompath);
        } else {
            snprintf(g_launchCmd, 2048, "\"%s\" \"%s\"", emu->launch.c_str(), rompath);
        }
        printf("Load rom %s\n", g_launchCmd);
        encrypt_path(rompath, strlen(rompath));
        showLaunchScreen(g_launchCmd);
    }
    setupKeymap(getKeymap());
    addRecent(&g_recentGames, label, emu->launch.c_str(), rompath, imgpath, type, true);
    return 3;
}

// 0x18560: builds "cd <dir>; chmod a+x <file>; [LD_PRELOAD=libpadsp.so <env>] <file>" for an app,
// writes it to /tmp/cmd_to_run.sh and records the app in the recent list; returns 3 (exit MainUI),
// or -1 when the launch path has no '/'.
int launchApp_buildCommand(AppInfo *app, int type)
{
    std::string file;
    std::string dir;
    std::string::size_type pos = app->launch.rfind("/");
    if (pos != std::string::npos) {
        dir = app->launch.substr(0, pos);
        file = g_skinSubdir;
        file += app->launch.substr(pos + 1);
    } else {
        return -1;
    }
    bool audioFix = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX) != 0;
    if (audioFix && app->noAudioFix != 0) {
        audioFix = false;
        system("touch /tmp/no_audioserver");
    }
    bool flip = app->autoFlip != 0;
    if (audioFix || flip) {
        char *cwd = getcwd(g_cwdBuf, 4096);
        if (cwd != NULL)
            snprintf(g_launchCmd, 2048, "cd %s; chmod a+x %s; LD_PRELOAD=%s/../lib/libpadsp.so %s %s %s ",
                     dir.c_str(), file.c_str(), cwd, audioFix ? "" : "PADSP_NO_DSP=1",
                     flip ? "PADSP_EN_FLIP=1" : "", file.c_str());
        else
            snprintf(g_launchCmd, 2048, "cd %s; chmod a+x %s; LD_PRELOAD=/customer/lib/libpadsp.so %s %s %s ",
                     dir.c_str(), file.c_str(), audioFix ? "" : "PADSP_NO_DSP=1",
                     flip ? "PADSP_EN_FLIP=1" : "", file.c_str());
        printf("Load app %s\n", file.c_str());
    } else {
        snprintf(g_launchCmd, 2048, "cd %s; chmod a+x %s; %s ", dir.c_str(), file.c_str(), file.c_str());
        printf("Load app %s\n", g_launchCmd);
    }
    writeCmdToRun(g_launchCmd);
    addRecent(&g_recentGames, app->label.c_str(), app->launch.c_str(), NULL, NULL, type, true);
    return 3;
}

// 0x1897c: launches the ROM, or opens a window on the ROM folder named by list item `param`
// (a TextItem *).
int EmuAction::execute(void *param)
{
    TextItem *item = (TextItem *)param;
    if (isDir == 0) {
        return launchRom(label.c_str(), emu, rompath.c_str(), imgpath.c_str(), result);
    } else {
        printf("load dir %s, %s\n", item->label.c_str(), folderArg.c_str());
        MenuWindow *w = new MenuWindow(item->label.c_str(), result, emu);
        w->setParentPath(folderArg);
        w->onShow();
        g_windowStack->push_back(w);
        return 0;
    }
}

// 0x18b00: opens the game detail window for list item `item` (a TextItem *).
int EmuAction::progress(int *item, int arg)
{
    TextItem *ti = (TextItem *)item;
    if (detail) {
        GameDetailWindow *w = new GameDetailWindow(84, 8, emu != NULL ? emu->label.c_str() : "",
                                                   ti->label.c_str(), rompath.c_str(),
                                                   imgpath.c_str(), arg, result, ti->favorite);
        w->onShow();
        g_windowStack->push_back(w);
        return 0;
    }
    return -1;
}
// =================================================================================================
// 0x18c54-0x1a44c: List actions (apps, favorites, search, dialogs) and the busy screens.
// =================================================================================================

// 0x18c54: launch the app (writes /tmp/cmd_to_run.sh; the caller then quits MainUI).
int AppAction::execute(void *param)
{
    return launchApp_buildCommand(app, result);
}

// 0x18c94: open the detail window for the selected item.
int AppAction::progress(int *row, int index)
{
    TextItem *item = (TextItem *)row;
    if (detail) {
        GameDetailWindow *w = new GameDetailWindow(STR_GAMEINFO, WIN_GAME_INFO, app->label.c_str(), item->label.c_str(),
                                                   app->launch.c_str(), app->iconSelected.c_str(),
                                                   index, result, item->favorite);
        w->onShow();
        g_windowStack->push_back(w);
        return 0;
    } else {
        return -1;
    }
}

// 0x18dd4: number of decimal digits.
int digitCount(int n)
{
    int count = 0;
    do {
        count++;
        n /= 10;
    } while (n != 0);
    return count;
}

// 0x18e3c: animate four dots (and "cur/total") until g_busy clears; 28 ms per frame, 5 frames a step.
void animateBusy(SDL_Surface *bg, SDL_Surface *dot, SDL_Rect bgRect, SDL_Rect dst, bool showCount)
{
    int frame = 0;
    SDL_Rect r;
    SDL_Rect src;
    while (g_busy) {
        if (dot != NULL && bg != NULL) {
            r.y = dst.y;
            if (frame / 5 % 4 == 0)
                SDL_UpperBlit(bg, &bgRect, g_backBuffer, &dst);
            if (frame / 5 % 4 == 1) {
                r.x = dst.x + (dst.w - dot->w * 3) / 4;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (frame / 5 % 4 == 2) {
                r.x = dst.x + (dst.w - dot->w * 3) / 2 + dot->w;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (frame / 5 % 4 == 3) {
                r.x = dst.x + (dst.w - dot->w * 3) * 3 / 4 + dot->w * 2;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            }
            if (showCount && g_progressTotal > 0) {
                r.x = dst.x - 15;
                r.w = dst.w + 30;
                r.y = dst.y + dot->h + 10;
                r.h = dst.h - dot->h - 10;
                if (frame / 5 % 4 != 0) {
                    src.x = bgRect.x - 15;
                    src.w = bgRect.w + 30;
                    src.y = bgRect.y + dot->h + 10;
                    src.h = bgRect.h - dot->h - 10;
                    SDL_UpperBlit(bg, &src, g_backBuffer, &r);
                }
                snprintf(g_launchCmd, 0x800, "%*d/%d", digitCount(g_progressTotal), g_progressCur, g_progressTotal);
                drawText(g_backBuffer, g_titleFont, r, g_launchCmd, g_selectedTextColor, 1, 0);
                SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
                SDL_Flip(g_screen);
            } else {
                SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
                SDL_Flip(g_screen);
            }
        }
        frame++;
        SDL_Delay(28);
    }
}

// 0x193a8: thread body: draw the frame and status bar, dim the screen, wait up to 280 ms,
// then show the "please wait" popup with the dot animation until g_busy clears.
int busyScreenThread(void *arg)
{
    int x;
    BusyData *sdata = (BusyData *)arg;
    x = 0;
    int r = 0;
    SDL_Rect rect;
    SDL_Rect inner;
    printf("sdata->type is %d, sdata->title is %s\n", sdata->type, sdata->title);
    drawScreenFrame(g_backBuffer, sdata->title, sdata->type, 1);
    x = drawBatteryIndicator(g_backBuffer, 1);
    r = drawSoundCardIndicator(g_backBuffer, 1);
    if (r >= 0)
        x = r;
    r = drawWifiIndicator(g_backBuffer, x, 1);
    if (r >= 0)
        x = r;
    if (g_volumeIndicatorVisible)
        drawKeyStateIndicator(g_backBuffer, x, 1);
    SDL_Surface *dim = createSurface(640, 480);
    if (dim != NULL) {
        SDL_Rect src = {0, 0, 640, 360};
        SDL_Rect dst;
        dst.x = 0;
        dst.y = 60;
        dst.w = 0;
        dst.h = 0;
        SDL_FillRect(dim, NULL, 0xaa000000);
        SDL_UpperBlit(dim, &src, g_backBuffer, &dst);
        SDL_FreeSurface(dim);
    }
    for (x = 0; x < 10 && g_busy; x++)
        SDL_Delay(28);
    if (g_busy) {
        SDL_Surface *bg = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, 0);
        if (bg != NULL) {
            rect.x = (640 - bg->w) / 2;
            rect.y = (480 - bg->h) / 2 + 10;
            rect.w = bg->w;
            rect.h = bg->h;
            SDL_UpperBlit(bg, NULL, g_backBuffer, &rect);
            rect.h = 50;
            drawText(g_backBuffer, g_imeFont, rect, languageString(STR_SCAN_GAMES), g_selectedTextColor, 1, 0);
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_Flip(g_screen);
            SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, 0);
            if (dot != NULL) {
                int h = dot->h;
                h += 40;
                rect.x = 255;
                rect.y = (480 - h) / 2 + 5;
                rect.w = 140;
                rect.h = h;
                inner.x = (bg->w - 140) / 2 + 5;
                inner.w = 140;
                inner.y = (bg->h - h) / 2 + 5;
                inner.h = h;
                animateBusy(bg, dot, inner, rect, true);
                SDL_FreeSurface(dot);
            }
            SDL_FreeSurface(bg);
        }
    }
    return 0;
}

// 0x198a8: thread body: plain background, a centered text line and the dot animation below it.
int loadingScreenThread(void *arg)
{
    drawBackground(g_backBuffer);
    int ret = 0;
    void *font = g_imeFont;
    int fh = TTF_FontHeight(font);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    int w = drawText(g_backBuffer, font, rect, languageString(STR_LOADING_CAPS), g_wifiEmptyColor, 1, 0);
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, 0);
    rect.x = (640 - w) / 2 + 5;
    rect.y = (480 - fh) / 2 + fh + 6;
    rect.w = w;
    if (dot != NULL)
        rect.h = dot->h;
    animateBusy(g_background, dot, rect, rect, false);
    if (dot != NULL)
        SDL_FreeSurface(dot);
    return 0;
}

// 0x19acc: mark the item removed from the favorites list.
int RemoveFavoriteAction::execute(void *param)
{
    item->removed = true;
    return 1;
}

// 0x19b04: open the result list for a search string (param), in a menu with the given id.
int SearchAction::execute(void *param)
{
    const char *key = (const char *)param;
    MenuWindow *w = new MenuWindow(STR_SEARCH, id, emu);
    w->setPath(key);
    w->onShow();
    g_windowStack->push_back(w);
    return -1;
}

// 0x19c18: the same, over every ROM list (WIN_SEARCH).
int GlobalSearchAction::execute(void *param)
{
    const char *key = (const char *)param;
    MenuWindow *w = new MenuWindow(STR_SEARCH, WIN_SEARCH, (void *)0);
    w->setPath(key);
    w->onShow();
    g_windowStack->push_back(w);
    return -1;
}

// 0x19d24: ask for a search string; the keyboard runs a SearchAction with it.
int PopupSearchIME::execute(void *param)
{
    ImeWindow *w = new ImeWindow(STR_SEARCH, new SearchAction(arg, id, emu), STR_SEARCH);
    pushWindow(w);
    return 4;
}

// 0x19dc0: opens a yes/no dialog for an action (the pop-up that offered it closes: result 4).
int PopupDialogAction::execute(void *param)
{
    DialogWindow *w = new DialogWindow(title, text, ownsAction, action);
    pushWindow(w);
    return 4;
}

// 0x19e44: runs an action behind a "please wait" dialog.
int PopupProgressAction::execute(void *param)
{
    ProgressDialog *w = new ProgressDialog(title, action);
    w->onShow();
    pushWindow(w);
    return 4;
}

// 0x19ecc: call a plain function.
int GeneralAction::execute(void *param)
{
    if (fn != 0) {
        if (verbose)
            puts("GeneralAction dowork");
        return fn(arg);
    } else {
        if (verbose)
            printf("GeneralAction dowork dosth %p\n", fn);
        return -1;
    }
}

// 0x19f64: "Clear" in favorites: delete favourite.json and empty the list.
int ClearFavoriteAction::execute(void *param)
{
    if (!unlink("/mnt/SDCARD/Roms/favourite.json")) {
        menu->clearItems();
        sync();
    }
    return 1;
}

// 0x19fd8: "Clear" in Recent: empty the list and delete recentlist.json.
int ClearRecentAction::execute(void *param)
{
    clearRecent(&g_recentGames);
    menu->clearItems();
    return 1;
}

// 0x1a02c: append the item to favourite.json; result is the window id it was opened from.
int AddFavoriteAction::execute(void *param)
{
    const char *label = 0;
    const char *launch = 0;
    const char *rompath = 0;
    if (result != WIN_APPS && result != WIN_ROM_LIST && result != WIN_RETROARCH_LIST && result != WIN_SEARCH) {
        printf("add to favorite error: unkown win %d\n", result);
        return 1;
    }
    FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "a+");
    if (f != NULL) {
        printf("open %s %p\n", "/mnt/SDCARD/Roms/favourite.json", f);
        cJSON *obj = cJSON_CreateObject();
        if (obj != NULL) {
            if (result != WIN_APPS) {
                EmuAction *emu = (EmuAction *)item->action;
                if (emu != NULL) {
                    if (emu->emu != NULL) {
                        launch = emu->emu->launch.c_str();
                        label = item->label.c_str();
                    } else {
                        printf("add to favorite error: emuitem %p\n", emu->emu);
                    }
                    rompath = emu->rompath.c_str();
                } else {
                    printf("add to favorite error: emu param %p\n", emu);
                }
                printf("add to favorite label %s, launch %s, type %d, rompath %s\n", label, launch, result, rompath);
                if (launch != NULL && !isFavorite(0, f, rompath)) {
                    if (cJSON_AddStringToObject(obj, "label", label) && cJSON_AddStringToObject(obj, "launch", launch) &&
                        cJSON_AddNumberToObject(obj, "type", result) && cJSON_AddStringToObject(obj, "rompath", rompath)) {
                        char *s = cJSON_PrintUnformatted(obj);
                        fprintf(f, "%s\n", s);
                        free(s);
                        item->favorite = true;
                    }
                }
            } else {
                AppAction *app = (AppAction *)item->action;
                if (app != NULL) {
                    label = app->app->label.c_str();
                    launch = app->app->launch.c_str();
                } else {
                    printf("add to favorite error: app param %p\n", app);
                }
                if (cJSON_AddStringToObject(obj, "label", label) && cJSON_AddStringToObject(obj, "launch", launch) &&
                    cJSON_AddNumberToObject(obj, "type", result)) {
                    char *s = cJSON_PrintUnformatted(obj);
                    fprintf(f, "%s\n", s);
                    free(s);
                    item->favorite = true;
                }
            }
            cJSON_Delete(obj);
        }
        fclose(f);
        sync();
    }
    return 1;
}
// =================================================================================================
// 0x1a44c-0x1d9d8: Favorites, Recent and app lists, ROM deletion, the window base.
// =================================================================================================

#include <sys/time.h>
#include <sys/stat.h>

int findEmuByLaunch(std::vector<EmuInfo> *list, const char *path);  // 0x120f80
int findApp(std::vector<AppInfo> *list, const char *path);          // 0x12114c
extern "C" const char *getLanguageFile();                          // 0x14ba18
// cJSON fields read directly (cJSON 1.7.10 layout).
struct cJSONItem {
    cJSONItem *next, *prev, *child;
    int type;           // 8 = cJSON_Number
    char *valuestring;
    int valueint;
};

int DeleteRomAction::execute(void *param)
{
    menu->removeItem(itemIndex);
    printf("removerom_action menu %p, pos %d\n", menu, itemIndex);
    return 1;
}

// Rewrites a favorites file from a list: one JSON object per line.
void saveFavorites(const char *path, std::vector<void *> *items)
{
    FILE *f = fopen(path, "w");
    if (f != NULL) {
        cJSON *obj = cJSON_CreateObject();
        if (obj != NULL) {
            if (items != NULL) {
                for (int i = 0; i < items->size(); i++) {
                    TextItem *item = (TextItem *)items->at(i);
                    cJSON_AddStringToObject(obj, "label", item->label.c_str());
                    if (item->action->result == 3) {
                        AppAction *action = (AppAction *)item->action;
                        AppInfo *app = action->app;
                        cJSON_AddStringToObject(obj, "launch", app->launch.c_str());
                    } else {
                        EmuAction *action = (EmuAction *)item->action;
                        EmuInfo *emu = action->emu;
                        cJSON_AddStringToObject(obj, "launch", emu->launch.c_str());
                        cJSON_AddStringToObject(obj, "rompath", action->rompath.c_str());
                    }
                    cJSON_AddNumberToObject(obj, "type", item->action->result);
                    char *s = cJSON_PrintUnformatted(obj);
                    fprintf(f, "%s\n", s);
                    printf("write %s to file\n", s);
                    free(s);
                    cJSON_DeleteItemFromObject(obj, "label");
                    cJSON_DeleteItemFromObject(obj, "launch");
                    cJSON_DeleteItemFromObject(obj, "type");
                    if (item->action->result != 3) {
                        cJSON_DeleteItemFromObject(obj, "rompath");
                    }
                }
            } else {
                char *s = cJSON_PrintUnformatted(obj);
                fprintf(f, "%s\n", s);
                free(s);
            }
            cJSON_Delete(obj);
        }
        fclose(f);
        sync();
    }
}

// Removes the lines whose "label" starts with label from a favorites file (via /tmp/favourite.tmp).
void removeFavorite(const char *label, const char *path)
{
    if (label == NULL)
        return;
    bool found = false;
    FILE *f = fopen(path, "r+");
    if (f != NULL) {
        FILE *tmp = fopen("/tmp/favourite.tmp", "w+");
        char *buf = (char *)malloc(0x2000);
        if (buf != NULL && tmp != NULL) {
            while (fgets(buf, 0x2000, f) != NULL) {
                cJSON *json = cJSON_Parse(buf);
                if (json != NULL) {
                    cJSON *item = cJSON_GetObjectItem(json, "label");
                    char *s = cJSON_GetStringValue(item);
                    if (s != NULL && strncmp(s, label, strlen(label)) != 0) {
                        fwrite(buf, 1, strlen(buf), tmp);
                    } else {
                        found = true;
                    }
                }
            }
            fclose(tmp);
        }
        free(buf);
        fclose(f);
    }
    if (found) {
        snprintf(g_launchCmd, 0x800, "mv %s %s", "/tmp/favourite.tmp", path);
        system(g_launchCmd);
    }
}

// Builds the favorites list from a file of JSON lines (type 3 apps, 5/17 ROMs). Drops duplicates and
// rewrites the file if any were found.
TextMenu *buildFavoriteList(const char *path)
{
    FILE *f = fopen(path, "r+");
    bool repeat = false;
    printf("create favourite %s, file=%p\n", path, f);
    if (f != NULL) {
        char *buf = (char *)malloc(0x2000);
        TextMenu *menu = new TextMenu(false, true, 6, NULL);
        int type = 0;
        while (fgets(buf, 0x2000, f) != NULL) {
            cJSON *json = cJSON_Parse(buf);
            if (json != NULL) {
                cJSON *jlabel = cJSON_GetObjectItem(json, "label");
                cJSON *jlaunch = cJSON_GetObjectItem(json, "launch");
                cJSON *jtype = cJSON_GetObjectItem(json, "type");
                cJSON *jrompath = cJSON_GetObjectItem(json, "rompath");
                cJSON *jimgpath = cJSON_GetObjectItem(json, "imgpath");
                char *label = cJSON_GetStringValue(jlabel);
                char *launch = cJSON_GetStringValue(jlaunch);
                char *rompath = cJSON_GetStringValue(jrompath);
                char *imgpath = cJSON_GetStringValue(jimgpath);
                if (jtype != NULL && ((cJSONItem *)jtype)->type == 8)
                    type = ((cJSONItem *)jtype)->valueint;
                if (label != NULL && launch != NULL) {
                    printf("favourite menu %s %d %s\n", label, type, launch);
                    TextItem *item = NULL;
                    if (type == 3) {
                        int idx = findApp(&g_appFolder, launch);
                        if (idx >= 0 && idx < g_appFolder.size()) {
                            item = new TextItem(label, 0, new AppAction(&g_appFolder[idx], false, type), true);
                        }
                        printf("label %s, path %s, type %d, idx %d, size %d\n", label, launch, type, idx,
                               g_appFolder.size());
                    } else if ((type == 5 || type == 0x11) && rompath != NULL) {
                        std::vector<EmuInfo> *list = NULL;
                        if (type == 5)
                            list = &g_emuFolders;
                        else
                            list = &g_rappFolders;
                        int idx = findEmuByLaunch(list, launch);
                        if (idx >= 0 && idx < list->size()) {
                            char *name = NULL;
                            if ((*list)[idx].hasCpinyin != 0)
                                name = GetGameName(getLanguageFile(), label);
                            if (name == NULL)
                                name = label;
                            if (name != label)
                                printf("label %s, show %s, path %s\n", label, name, launch);
                            else
                                printf("label %s, path %s\n", label, launch);
                            item = new TextItem(name, 0,
                                                new EmuAction(&(*list)[idx], name, rompath ? rompath : "",
                                                              imgpath ? imgpath : "", type, true, 0),
                                                true);
                        }
                    }
                    if (item != NULL && !menu->addUnique(item)) {
                        printf("%s is repeat\n", buf);
                        delete item;
                        repeat = true;
                    }
                }
                cJSON_Delete(json);
            }
        }
        if (repeat) {
            puts("need to format favourit file");
            std::vector<void *> *items = menu->getItems();
            if (items != NULL) {
                cJSON *obj = cJSON_CreateObject();
                rewind(f);
                if (obj != NULL) {
                    for (int i = 0; i < items->size(); i++) {
                        TextItem *it = (TextItem *)items->at(i);
                        if (it->action->result == 5 || it->action->result == 0x11) {
                            EmuAction *action = (EmuAction *)it->action;
                            if (action->emu != NULL) {
                                cJSON_AddStringToObject(obj, "label", it->label.c_str());
                                cJSON_AddStringToObject(obj, "launch", action->emu->launch.c_str());
                            } else {
                                cJSON_AddStringToObject(obj, "label", it->label.c_str());
                                cJSON_AddStringToObject(obj, "launch", "");
                            }
                            cJSON_AddStringToObject(obj, "rompath", action->rompath.c_str());
                            cJSON_AddStringToObject(obj, "imgpath", action->imgpath.c_str());
                            cJSON_AddNumberToObject(obj, "type", action->result);
                        } else {
                            AppAction *action = (AppAction *)it->action;
                            cJSON_AddStringToObject(obj, "label", action->app->label.c_str());
                            cJSON_AddStringToObject(obj, "launch", action->app->launch.c_str());
                            cJSON_AddNumberToObject(obj, "type", action->result);
                        }
                        char *s = cJSON_PrintUnformatted(obj);
                        fprintf(f, "%s\n", s);
                        free(s);
                    }
                    cJSON_Delete(obj);
                }
            }
        }
        fclose(f);
        free(buf);
        return menu;
    } else {
        TextMenu *menu = new TextMenu(false, true, 6, NULL);
        return menu;
    }
}

// Looks for "rompath" (quoted) in buf, or else in each line of f.
bool isFavorite(char *buf, FILE *f, const char *rompath)
{
    std::string key;
    std::string quote("\"");
    std::string path(rompath);
    key.append(quote);
    key.append(path);
    key.append(quote);
    if (buf != NULL) {
        printf("isFavourite search %s in buf(%s)\n", key.c_str(), buf);
        return strstr(buf, key.c_str()) != NULL;
    } else if (f != NULL) {
        fseek(f, 0, SEEK_SET);
        buf = (char *)malloc(0x2000);
        while (fgets(buf, 0x2000, f) != NULL) {
            if (strstr(buf, key.c_str()) != NULL) {
                printf("isFavourite found %s in file line(%s)\n", key.c_str(), buf);
                free(buf);
                return true;
            }
        }
        free(buf);
        return false;
    }
    return false;
}

// Builds the recent-games list; favorites are marked from favourite.json.
TextMenu *buildRecentList(std::list<RecentItem> *list)
{
    if (list->size() != 0) {
        FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "r");
        TextMenu *menu = new TextMenu(false, true, 6, NULL);
        std::list<RecentItem>::iterator it;
        for (it = list->begin(); it != list->end(); it++) {
            printf("recent list %s\n", it->label.c_str());
            TextItem *item = NULL;
            if (it->type == 3) {
                int idx = findApp(&g_appFolder, it->launch.c_str());
                if (idx >= 0 && idx < g_appFolder.size()) {
                    item = new TextItem(it->label.c_str(), 0,
                                        new AppAction(&g_appFolder[idx], false, it->type), true);
                }
            } else {
                int idx = findEmuByLaunch(&g_emuFolders, it->launch.c_str());
                if (idx >= 0 && idx < g_emuFolders.size()) {
                    const char *name = NULL;
                    if (g_emuFolders[idx].hasCpinyin != 0)
                        name = GetGameName(getLanguageFile(), it->label.c_str());
                    if (name == NULL)
                        name = it->label.c_str();
                    item = new TextItem(name, 0,
                                        new EmuAction(&g_emuFolders[idx], name, it->rompath.c_str(),
                                                      it->imgpath.c_str(), it->type, true, 0),
                                        true);
                } else {
                    idx = findEmuByLaunch(&g_rappFolders, it->launch.c_str());
                    if (idx >= 0 && idx < g_rappFolders.size()) {
                        const char *name = NULL;
                        if (g_rappFolders[idx].hasCpinyin != 0)
                            name = GetGameName(getLanguageFile(), it->label.c_str());
                        if (name == NULL)
                            name = it->label.c_str();
                        item = new TextItem(name, 0,
                                            new EmuAction(&g_rappFolders[idx], name, it->rompath.c_str(),
                                                          it->imgpath.c_str(), it->type, true, 0),
                                            true);
                    }
                }
            }
            if (f != NULL && item != NULL)
                item->favorite = isFavorite(0, f, it->label.c_str());
            if (item == NULL || !menu->addUnique(item))
                delete item;
        }
        if (f != NULL)
            fclose(f);
        return menu;
    } else {
        return NULL;
    }
}

// Builds the app list (with descriptions and icons); favorites are marked from favourite.json.
TextMenu *buildAppList(std::vector<AppInfo> *apps)
{
    if (apps->size() != 0) {
        FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "r");
        const int columns = 4;
        TextMenu *menu = new TextMenu(false, true, columns, "skin/bg-list-l.png");
        menu->setRowHeight(90);
        for (int i = 0; i < apps->size(); i++) {
            TextItem *item = new TextItem((*apps)[i].label.c_str(), 0, new AppAction(&(*apps)[i], true, 3), true);
            item->subtitle = (*apps)[i].description;
            item->thumbnailPath = (*apps)[i].icon;
            item->showThumbnail = true;
            if (f != NULL)
                item->favorite = isFavorite(0, f, (*apps)[i].label.c_str());
            if (!menu->addUnique(item))
                delete item;
        }
        if (f != NULL)
            fclose(f);
        return menu;
    } else {
        return NULL;
    }
}

// SigmaStar MI_DISP_LcdParam_t.
struct LcdParam {
    int cscMatrix;
    int luma;
    int contrast;
    int hue;
    int saturation;
    int sharpness;
};
extern "C" {
int MI_DISP_GetLcdParam(int layer, LcdParam *param);
int MI_DISP_SetLcdParam(int layer, LcdParam *param);


void initBacklightPwm();                // 0x14ae74
int setBacklight(int level);            // 0x14aed4
}

// Sets the panel color from the settings scale (luma 0-, hue/saturation x5, contrast).
int setLcdColor(int luma, int hue, int saturation, int contrast)
{
    LcdParam param;
    memset(&param, 0, sizeof(param));
    MI_DISP_GetLcdParam(0, &param);
    printf("getlcd: %d, %d, %d, %d\n", param.luma, param.hue, param.saturation, param.contrast);
    param.luma = luma + 35;
    param.hue = hue * 5;
    param.saturation = saturation * 5;
    param.contrast = (contrast + 15) * 2;
    printf("Set: luma %d hue %d sat %d constract %d\n", param.luma, param.hue, param.saturation, param.contrast);
    MI_DISP_SetLcdParam(0, &param);
    return 0;
}

// Applies the saved keymap, volume, backlight and music volume (key shm slots 0, 1, 5).
void initAudioSettings(void *shm)
{
    setupKeymap("L2,L,R2,R,X,A,B,Y");
    setVolumeTinymix(GetKeyShm(shm, MONITOR_VOLUME));
    initBacklightPwm();
    setBacklight(GetKeyShm(shm, MONITOR_BRIGHTNESS));
    Mix_PlayMusic(g_themeMusic, -1);
    Mix_VolumeMusic(GetKeyShm(shm, MONITOR_BGM_VOLUME) * 128 / 20);
    Mix_Volume(1, GetKeyShm(shm, MONITOR_BGM_VOLUME) * 128 / 20);
    printf("restore background music volume %d\n", GetKeyShm(shm, MONITOR_BGM_VOLUME) * 128 / 20);
}

// Deletes the items marked removed from a favorites list (menu id 2); true if any were removed.
bool removeDeletedFavorites(void *menu, int id)
{
    bool changed = false;
    if (id == 2) {
        TextMenu *textMenu = (TextMenu *)menu;
        std::vector<void *> *items = textMenu->getItems();
        if (items != NULL) {
            int unused = 0;
            std::vector<void *>::iterator it = items->begin();
            while (it != items->end()) {
                TextItem *item = (TextItem *)*it;
                printf("refresh %s removed %d\n", item->label.c_str(), item->removed);
                if (item->removed) {
                    changed = true;
                    delete item;
                    items->erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    return changed;
}

extern "C" int readWholeFile(const char *name, FILE *f, char **out);  // 0x14b8a4

// Recreates the windows saved in /tmp/state.json (then deletes it); returns how many.
int restoreWindowState(std::vector<TrimusWindow *> *stack)
{
    int restored = 0;
    char *buf = NULL;
    int unused = 0;
    int i = 0;
    int n = 0;
    FILE *f = fopen("/tmp/state.json", "r");
    if (f == NULL) {
        printf("open %s fail\n", "/tmp/state.json");
        return restored;
    }
    do {
        if (readWholeFile("/tmp/state.json", f, &buf) < 0)
            break;
        cJSON *json = cJSON_Parse(buf);
        if (json == NULL)
            break;
        cJSON *windows = cJSON_GetObjectItem(json, "list");
        if (windows != NULL) {
            n = cJSON_GetArraySize(windows);
            for (i = 0; i < n; i++) {
                cJSON *item = cJSON_GetArrayItem(windows, i);
                if (item != NULL) {
                    cJSONItem *type = (cJSONItem *)cJSON_GetObjectItem(item, "type");
                    cJSONItem *title = (cJSONItem *)cJSON_GetObjectItem(item, "title");
                    if (title != NULL && title->type == 8 && type != NULL && type->type == 8) {
                        if (type->valueint == 8) {
                            GameDetailWindow *w = new GameDetailWindow(title->valueint, type->valueint);
                            w->restoreState(item);
                            w->onShow();
                            stack->push_back(w);
                            restored++;
                        } else {
                            MenuWindow *w = new MenuWindow(title->valueint, type->valueint, (void *)0);
                            w->restoreState(item);
                            w->onShow();
                            stack->push_back(w);
                            restored++;
                        }
                    }
                }
            }
        }
        cJSON_Delete(json);
    } while (0);
    if (buf != NULL)
        free(buf);
    fclose(f);
    unlink("/tmp/state.json");
    printf("%d windows restored\n", restored);
    return restored;
}


// Writes {"list": [{title, type, ...saveState}, ...]} for every window on the stack.
void saveWindowState(std::vector<TrimusWindow *> *stack, const char *path)
{
    if (stack->size() == 0)
        return;
    printf("%d wins need to be saved\n", stack->size());
    FILE *f = fopen(path, "w+");
    if (f != NULL) {
        cJSON *root = cJSON_CreateObject();
        if (root != NULL) {
            cJSON *windows = cJSON_AddArrayToObject(root, "list");
            if (windows != NULL) {
                for (int i = 0; i < stack->size(); i++) {
                    TrimusWindow *w = stack->at(i);
                    printf("save state for window %p\n", w);
                    cJSON *obj = cJSON_CreateObject();
                    cJSON *title = cJSON_CreateNumber(w->titleLabelId);
                    cJSON_AddItemToObject(obj, "title", title);
                    cJSON *type = cJSON_CreateNumber(w->id);
                    cJSON_AddItemToObject(obj, "type", type);
                    w->saveState(obj);
                    cJSON_AddItemToArray(windows, obj);
                }
                char *s = cJSON_Print(root);
                if (s != NULL) {
                    fputs(s, f);
                    printf("save %s\n", s);
                    free(s);
                }
            }
            cJSON_Delete(root);
        }
        fclose(f);
    }
}

// Remembers the selection of a ROM list (keyed by path), adding an entry if needed.
void setRomListPosition(std::vector<RomListPosition> *list, const std::string &path, int cur, int start, int selected)
{
    bool found = false;
    for (unsigned int i = 0; i < list->size(); i++) {
        if ((*list)[i].path.compare(path) == 0) {
            (*list)[i].current = cur;
            (*list)[i].start = start;
            (*list)[i].selected = selected;
            found = true;
        }
    }
    if (!found) {
        RomListPosition pos;
        pos.path = path;
        pos.current = cur;
        pos.start = start;
        pos.selected = selected;
        list->push_back(pos);
    }
}

int getRomListPosition(std::vector<RomListPosition> *list, std::string path, RomListPosition *out)
{
    for (unsigned int i = 0; i < list->size(); i++) {
        if ((*list)[i].path.compare(path) == 0) {
            *out = (*list)[i];
            return 0;
        }
    }
    return -1;
}

// Reads /appconfigs/romwinidx.json: {"list": [{rompath, pos, start, end}, ...]}. The count it prints and
// returns is never incremented.
int loadRomListPositions(std::vector<RomListPosition> *list)
{
    int restored = 0;
    char *buf = NULL;
    int unused = 0;
    int i = 0;
    int n = 0;
    FILE *f = fopen("/appconfigs/romwinidx.json", "r");
    if (f == NULL) {
        printf("open %s fail\n", "/appconfigs/romwinidx.json");
        return restored;
    }
    do {
        if (readWholeFile("/appconfigs/romwinidx.json", f, &buf) < 0)
            break;
        cJSON *json = cJSON_Parse(buf);
        if (json == NULL)
            break;
        cJSON *entries = cJSON_GetObjectItem(json, "list");
        if (entries != NULL) {
            n = cJSON_GetArraySize(entries);
            for (i = 0; i < n; i++) {
                cJSON *item = cJSON_GetArrayItem(entries, i);
                if (item != NULL) {
                    cJSONItem *rompath = (cJSONItem *)cJSON_GetObjectItem(item, "rompath");
                    cJSONItem *pos = (cJSONItem *)cJSON_GetObjectItem(item, "pos");
                    cJSONItem *start = (cJSONItem *)cJSON_GetObjectItem(item, "start");
                    cJSONItem *end = (cJSONItem *)cJSON_GetObjectItem(item, "end");
                    if (rompath != NULL && rompath->type == 16 && pos != NULL && pos->type == 8 &&
                        start != NULL && start->type == 8 && end != NULL && end->type == 8) {
                        setRomListPosition(list, rompath->valuestring, pos->valueint, start->valueint,
                                           end->valueint);
                    }
                }
            }
        }
        cJSON_Delete(json);
    } while (0);
    if (buf != NULL)
        free(buf);
    fclose(f);
    printf("%d idxs restored\n", restored);
    return restored;
}


void saveRomListPositions(std::vector<RomListPosition> *list)
{
    printf("%d idxs need to be saved\n", list->size());
    FILE *f = fopen("/appconfigs/romwinidx.json", "w+");
    RomListPosition pos;
    if (f != NULL) {
        cJSON *root = cJSON_CreateObject();
        if (root != NULL) {
            cJSON *entries = cJSON_AddArrayToObject(root, "list");
            if (entries != NULL) {
                for (unsigned int i = 0; i < list->size(); i++) {
                    pos = list->at(i);
                    cJSON *obj = cJSON_CreateObject();
                    cJSON *rompath = cJSON_CreateString(pos.path.c_str());
                    cJSON_AddItemToObject(obj, "rompath", rompath);
                    cJSON *cur = cJSON_CreateNumber(pos.current);
                    cJSON_AddItemToObject(obj, "pos", cur);
                    cJSON *start = cJSON_CreateNumber(pos.start);
                    cJSON_AddItemToObject(obj, "start", start);
                    cJSON *end = cJSON_CreateNumber(pos.selected);
                    cJSON_AddItemToObject(obj, "end", end);
                    cJSON_AddItemToArray(entries, obj);
                }
                char *s = cJSON_Print(root);
                if (s != NULL) {
                    fputs(s, f);
                    printf("save %s\n", s);
                    free(s);
                }
            }
            cJSON_Delete(root);
        }
        fclose(f);
    }
}


SDL_Surface *loadImage(const char *path, SDL_Surface *format, bool debug);  // 0x123a38
#include "PixelScale.h"

// Loads an image as 32-bit RGBA, scaled down to width w (aspect kept) when it is wider.
SDL_Surface *loadThumbnail(const char *path, int w, int h)
{
    SDL_Surface *result = NULL;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    bool convert = true;
    if (path == NULL || path[0] == '\0')
        return NULL;
    int ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    SDL_Surface *img = loadImage(path, g_backBuffer, false);
    if (convert && img != NULL) {
        gettimeofday(&tv, NULL);
        ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        SDL_Surface *rgba = NULL;
        printf("thumb3bytes pixel %d\n", img->format->bitsPerPixel);
        if (img->format->bytesPerPixel != 4) {
            rgba = SDL_CreateRGBSurface(0, img->w, img->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
            if (rgba != NULL) {
                printf("copy %dx%d %d:%d->%dx%d %d:%d\n", img->w, img->h, img->pitch, img->format->bytesPerPixel,
                       rgba->w, rgba->h, rgba->pitch, rgba->format->bytesPerPixel);
                SDL_UpperBlit(img, NULL, rgba, NULL);
            }
            SDL_FreeSurface(img);
        } else {
            rgba = img;
        }
        if (rgba != NULL) {
            int dw = w;
            int dh = h;
            if (w <= rgba->w) {
                dh = w * rgba->h / rgba->w;
                result = SDL_CreateRGBSurface(0, dw, dh, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
                if (result != NULL) {
                    printf("bitmap_scale %dx%d %d:%d->%dx%d %d:%d\n", rgba->w, rgba->h, rgba->pitch,
                           rgba->format->bytesPerPixel, result->w, result->h, result->pitch,
                           result->format->bytesPerPixel);
                    scalePixels(0, 0, rgba->w, rgba->h, dw, dh, rgba->w, 0, (int *)rgba->pixels,
                                (int *)result->pixels);
                }
                SDL_FreeSurface(rgba);
            } else {
                result = rgba;
            }
        }
        ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        printf("process thumbnail %s at %dms\n", path, ms);
        return result;
    }
    return img;
}

// Thumbnail of a game: img itself when it is a .png, else <img><game name>.png, or <game without
// extension>.png without img; "" when that file does not exist.
std::string makeThumbPath(std::string *game, const char *img)
{
    std::string result;
    std::string ext(".png");
    if (strstr(img, ".png") != NULL)
        return img;
    if (img != NULL) {
        printf("makeThumbPath game %s, img %s\n", game->c_str(), img);
        std::string dir(img);
        std::string::size_type dot = game->rfind(".");
        if (dot != std::string::npos) {
            std::string::size_type slash = game->rfind("/");
            if (slash != std::string::npos)
                result = dir + game->substr(slash, dot - slash) + ext;
            else
                result = "";
        } else {
            result = dir;
        }
    } else {
        printf("makeThumbPath game %s\n", game->c_str());
        std::string::size_type dot = game->rfind(".");
        if (dot != std::string::npos)
            result = game->substr(0, dot) + ext;
        else
            result = "";
    }
    struct stat st;
    if (stat(result.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        printf("file %s not exists\n", result.c_str());
        result = "";
    }
    return result;
}

// A 32-bit RGBA surface.
SDL_Surface *createSurface(int w, int h)
{
    unsigned int rmask, gmask, bmask, amask;
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
    SDL_Surface *surface = SDL_CreateRGBSurface(1, w, h, 32, rmask, gmask, bmask, amask);
    if (surface == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        return NULL;
    }
    return surface;
}

TrimusWindow::TrimusWindow(int titleLabelId, int id)
{
    this->titleLabelId = titleLabelId;
    this->id = id;
}

TrimusWindow::~TrimusWindow()
{
}

void TrimusWindow::dump()
{
    printf("title %s, type %d\n", titleLabelId >= 0 ? languageString(titleLabelId) : "", id);
}

ListMenu::ListMenu(bool flag)
{
    gridMode = flag;
    cur = 0;
    pageStart = 0;
    pageEnd = 0;
    lastKeyTime = 0;
    previewWidth = 0;
}

void ListMenu::setRect(SDL_Rect *r)
{
    rect = *r;
}
// =================================================================================================
// 0x1d9d8-0x20500: ListMenu navigation and TextMenuDrawer drawing.
// =================================================================================================
#include <sys/time.h>   // gettimeofday (no inline functions, so it does not move the tail)

extern SDL_Color g_highlightColor;     // 0x18228c (only drawListNumber uses it)
extern SDL_Color g_accentColor;         // 0x1822b0 (search match in a list row)

SDL_Surface *loadImage(const char *path, SDL_Surface *fmt, bool flag); // 0x123a38 (Config.cpp)
int pinyinFind(const char *text, const char *initials, const char *key, const char **start,
               int *bytes);                                     // 0x143cf4 (Pinyin.cpp)

// 0x1d9d8: restore a saved position if it is still inside the list.
void ListMenu::restorePosition(int cur, int pageStart, int pageEnd)
{
    if (cur >= 0 && cur < count())
        this->cur = cur;
    if (pageStart >= 0 && pageStart < count() && pageEnd >= 0 && pageEnd < count()) {
        this->pageStart = pageStart;
        this->pageEnd = pageEnd;
    }
}

// 0x1dae8: next item, wrapping to the first.
int ListMenu::moveDown()
{
    if (cur < count() - 1)
        cur++;
    else
        cur = 0;
    return 0;
}

// 0x1db6c: previous item, wrapping to the last.
int ListMenu::moveUp()
{
    if (cur > 0)
        cur--;
    else
        cur = count() - 1;
    return 0;
}

// 0x1dbdc: six items up.
int ListMenu::pageUp()
{
    cur -= 6;
    if (cur < 0)
        cur = 0;
    return 0;
}

// 0x1dc30: six items down.
int ListMenu::pageDown()
{
    cur += 6;
    if (cur >= count())
        cur = count() - 1;
    return 0;
}

// 0x1dcc8: remember keys repeated within 200 ms (held direction keys).
void ListMenu::recordKey(int key)
{
    int now = milliseconds();
    if ((unsigned long)(now - lastKeyTime) < 200) {
        if (history.size() != 0) {
            if (history[history.size() - 1] == key) {
                history.push_back(key);
                lastKeyTime = now;
            }
        } else {
            lastKeyTime = now;
            history.push_back(key);
        }
    } else {
        history.clear();
        lastKeyTime = now;
    }
}

// 0x1ddf4: a key has been held long enough to scroll by pages.
bool ListMenu::keyRepeating()
{
    return history.size() > 25;
}

// 0x1de34: one button. Up/down move by one row, or by a page while the key repeats; L2/R2 page;
// left/right move in grids (gridMode: IconMenu) and are otherwise returned as LIST_LEFT/LIST_RIGHT;
// A or Y runs onSelect; B or X returns LIST_CLOSE; anything else goes to onUnhandledEvent.
int ListMenu::handleKey()
{
    int key = handleGlobalInput(0);
    recordKey(key);
    switch (key) {
        case BUTTON_UP:
            if (!gridMode) {
                if (!keyRepeating())
                    moveUp();
                else
                    pageUp();
            }
            break;
        case BUTTON_DOWN:
            if (!gridMode) {
                if (!keyRepeating())
                    moveDown();
                else
                    pageDown();
            }
            break;
        case BUTTON_LEFT:
            if (gridMode)
                moveUp();
            else
                return LIST_LEFT;
            break;
        case BUTTON_RIGHT:
            if (gridMode)
                moveDown();
            else
                return LIST_RIGHT;
            break;
        case BUTTON_HOTKEY1:
            if (!gridMode)
                pageUp();
            break;
        case BUTTON_HOTKEY2:
            if (!gridMode)
                pageDown();
            break;
        case BUTTON_A:
        case BUTTON_Y:
            struct timeval tv;
            gettimeofday(&tv, NULL);
            int ms;
            ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            printf("a pressed at %dms\n", ms);
            return onSelect();
        case BUTTON_B:
        case BUTTON_X:
            return 1;
        case BUTTON_MENU:
            return BUTTON_MENU;
        case BUTTON_START:
            return BUTTON_START;
        default:
            return onUnhandledEvent(key);
    }
    return -1;
}

// 0x1e128: a list of text rows: without noSelectedBg the selection is drawn with an image
// (skin/bg-list-s.png or `background`); stripes draws separators; rows per page.
TextMenuDrawer::TextMenuDrawer(bool noSelectedBg, bool stripes, int rows, const char *background)
    : ListMenu(false)
{
    if (!noSelectedBg) {
        SDL_Surface *s = NULL;
        if (background) {
            s = loadSkinImage(g_themePath, g_skinSubdir, background, g_backBuffer, 0);
            this->background = background;
        } else {
            s = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, 0);
            this->background = "skin/bg-list-s.png";
        }
        selectedBackground = s;
    } else {
        selectedBackground = NULL;
    }
    rowBackground = NULL;
    favoriteMark = loadSkinImage(g_themePath, g_skinSubdir, "skin/ic-favorite-mark.png", g_backBuffer, 0);
    themePath = g_themePath;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    textRowHeight = 60;
    unusedPopupFlag = true;
    textColor = g_imeTextColor;
    colorBC = g_imeTextColor;
    customBackground = NULL;
    shadowSelected = true;
    drawStripes = stripes;
    rowsPerPage = rows;
}

// 0x1e3f8 (D0 0x1e4e4)
TextMenuDrawer::~TextMenuDrawer()
{
    if (rowBackground)
        SDL_FreeSurface(rowBackground);
    if (selectedBackground)
        SDL_FreeSurface(selectedBackground);
    if (favoriteMark)
        SDL_FreeSurface(favoriteMark);
    if (customBackground)
        SDL_FreeSurface(customBackground);
}

// 0x1e518: onSelect, shared by every text menu: run the current item's Action with the item.
int TextMenuDrawer::onSelect()
{
    TextItem *item = (TextItem *)getItem(cur);
    if (item && item->action) {
        int r;
        if ((r = item->action->execute(item)) >= 0)
            return r;
    }
    return -1;
}

// 0x1e5d0: reload the theme images when the theme changed.
void TextMenuDrawer::reloadTheme()
{
    if (themePath.compare(g_themePath) != 0) {
        if (background.size() != 0) {
            if (selectedBackground)
                SDL_FreeSurface(selectedBackground);
            SDL_Surface *s = NULL;
            s = loadSkinImage(themePath.c_str(), g_skinSubdir, background.c_str(), g_backBuffer, 0);
            selectedBackground = s;
        }
        if (favoriteMark)
            SDL_FreeSurface(favoriteMark);
        favoriteMark = loadSkinImage(themePath.c_str(), g_skinSubdir, "skin/ic-favorite-mark.png",
                                     g_backBuffer, 0);
        themePath = g_themePath;
    }
}

// 0x1e76c: copy `src` into a new 32-bit surface with alpha and free it.
SDL_Surface *TextMenuDrawer::makeSelectedBackground(SDL_Surface *src)
{
    SDL_Surface *sel = NULL;
    sel = SDL_CreateRGBSurface(1, src->w, src->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
    SDL_Surface *tmp = SDL_CreateRGBSurface(1, src->w, src->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
    printf("TextMenuDrawer mSelBG %p\n", sel);
    SDL_UpperBlit(src, NULL, tmp, NULL);
    SDL_UpperBlit(tmp, NULL, sel, NULL);
    SDL_FreeSurface(src);
    SDL_FreeSurface(tmp);
    return sel;
}

// 0x1e868: draw `n` (at most 9999, as "%04d") centered in `rect`: digit images from g_listNumber
// (ten digits side by side) over g_numberBg, or plain text when the theme has no such images.
// Returns the width drawn. Called by GameDetailWindow::draw.
int drawListNumber(int n, SDL_Rect *rect, SDL_Surface *dst, TTF_Font *font)
{
    char buf[8] = {0};
    if (n > 9999)
        n = 9999;
    snprintf(buf, 8, "%04d", (short)n);
    if (g_numberBg && g_listNumber) {
        int i;
        SDL_Rect pos;
        pos.x = rect->x + (rect->w - g_numberBg->w) / 2;
        pos.y = rect->y + (rect->h - g_numberBg->h) / 2;
        SDL_UpperBlit(g_numberBg, NULL, dst, &pos);
        int digitW = g_listNumber->w / 10;
        int x = (rect->w - g_numberBg->w) / 2 + (g_numberBg->w - strlen(buf) * (digitW + 1)) / 2;
        for (i = 0; i < strlen(buf); i++) {
            SDL_Rect src;
            src.x = (buf[i] - '0') * digitW;
            src.y = 0;
            src.w = digitW;
            src.h = g_listNumber->h;
            pos.x = rect->x + x + (digitW + 1) * i;
            pos.y = rect->y + (rect->h - g_listNumber->h) / 2;
            SDL_UpperBlit(g_listNumber, &src, dst, &pos);
        }
        return g_numberBg->w;
    } else {
        return drawText(dst, font, *rect, buf, g_highlightColor, 1, 0);
    }
}

// 0x1ec00 (unreferenced): cut the UTF-8 string `s` (`len` bytes) to
// about `width` columns (ASCII 1, other characters 1.5) and end it with up to three dots.
int truncateUtf8(char *s, int len, int width)
{
    int i;
    int j;
    float w;
    w = 0;
    i = 0;
    while (i < len && width > (int)w) {
        if (s[i] >= 0) {
            w += 1.0;
            i++;
        } else {
            j = i;
            i++;
            while (i < len && (s[i] & 0xc0) == 0x80)
                i++;
            if (i < len) {
                w += 1.5;
            } else {
                i = j;
                break;
            }
        }
    }
    if (w >= width && i < len) {
        for (j = 0; i + j < len && j < 3; j++)
            s[i + j] = '.';
        for (i += j; i < len; i++)
            s[i] = 0;
    }
    return 0;
}

// 0x1ede4: draw a row label with the part matching the search `key` (found through the pinyin
// `initials`) in g_accentColor. Align 0/3 draws left to right, 2/4 right to left, anything else
// centered (pieces drawn with align 3).
void TextMenuDrawer::drawHighlighted(SDL_Surface *dst, TTF_Font *font, SDL_Rect rect,
                                     const char *text, SDL_Color color, int align, bool shadow,
                                     const char *key, const char *initials)
{
    if (*key != '\0') {
        int w, h;
        SDL_Rect r = rect;
        TTF_SizeText(font, text, &w, &h);
        const char *start = NULL;
        int bytes = 0;
        if (pinyinFind(text, initials, key, &start, &bytes) >= 0) {
            int len = bytes;
            int tw;
            if (align == 0 || align == 3) {
                if (text < start) {
                    textScratch = text;
                    textScratch = textScratch.substr(0, start - text);
                    tw = drawText(dst, font, r, textScratch.c_str(), color, align, shadow);
                    r.x += tw;
                }
                textScratch = start;
                textScratch = textScratch.substr(0, len);
                tw = drawText(dst, font, r, textScratch.c_str(), g_accentColor, align, shadow);
                r.x += tw;
                if (strlen(text) > start - text + len) {
                    textScratch = start + len;
                    drawText(dst, font, r, textScratch.c_str(), color, align, shadow);
                }
            } else if (align == 2 || align == 4) {
                if (strlen(text) > start - text + len) {
                    textScratch = start + len;
                    drawText(dst, font, r, textScratch.c_str(), color, align, shadow);
                    r.w -= tw;
                }
                textScratch = start;
                textScratch = textScratch.substr(0, len);
                tw = drawText(dst, font, r, textScratch.c_str(), g_accentColor, align, shadow);
                r.w -= tw;
                if (text < start) {
                    textScratch = text;
                    textScratch = textScratch.substr(0, start - text);
                    drawText(dst, font, r, textScratch.c_str(), color, align, shadow);
                }
            } else {
                r.x = rect.x + (rect.w - w) / 2;
                if (text < start) {
                    textScratch = text;
                    textScratch = textScratch.substr(0, start - text);
                    tw = drawText(dst, font, r, textScratch.c_str(), color, 3, shadow);
                    r.x += tw;
                }
                textScratch = start;
                textScratch = textScratch.substr(0, len);
                tw = drawText(dst, font, r, textScratch.c_str(), g_accentColor, 3, shadow);
                r.x += tw;
                if (strlen(text) > start - text + len) {
                    textScratch = start + len;
                    drawText(dst, font, r, textScratch.c_str(), color, 3, shadow);
                }
            }
        } else {
            drawText(dst, font, rect, text, color, align, shadow);
        }
    } else {
        drawText(dst, font, rect, text, color, align, shadow);
    }
}

// 0x1f5e0: load `file` from the current theme into customBackground and remember it as the background.
void TextMenuDrawer::setBackground(const char *file)
{
    if (customBackground)
        SDL_FreeSurface(customBackground);
    customBackground = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, 0);
    themePath = g_themePath;
    background = file;
}

// 0x1f694: free the selection image (no caller found).
void TextMenuDrawer::freeSelectedBackground(bool really)
{
    if (really && selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x1f6f0: draw the visible rows. Keeps the page (pageStart..pageEnd, rowsPerPage rows) around the
// current item, then per row: row background, selection background, icon (TextItem::value is an
// SDL_Surface* here), the label with the search match highlighted, or for two-line items
// (showThumbnail) a 71x71 thumbnail (thumbnailSurface, loaded from thumbnailPath), the label and str28 below it; finally
// the favorite mark.
void TextMenuDrawer::drawList(SDL_Surface *dst)
{
    int i;
    int row;
    TTF_Font *font = g_imeFont;
    int fontH = TTF_FontHeight(font);
    if (count() <= 0)
        return;
    if (pageStart == 0 && pageEnd == 0)
        pageEnd = rowsPerPage - 1;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + rowsPerPage - 1;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - rowsPerPage + 1;
    }
    if (pageStart < 0)
        pageStart = 0;
    if (pageEnd >= count() - 1)
        pageEnd = count() - 1;
    SDL_Rect sr;
    SDL_Color color;
    SDL_Rect dr;
    if (drawStripes) {
        for (i = 0; i < rowsPerPage - 1 && g_listStripe; i++) {
            dr.x = 0;
            dr.y = (i + 1) * textRowHeight + 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dr);
        }
    }
    for (i = pageStart, row = 0; i <= pageEnd; i++, row++) {
        TextItem *item = (TextItem *)getItem(i);
        color = textColor;
        int x = rect.x;
        int y = rect.y + 2 + row * textRowHeight;
        if (rowBackground) {
            dr.x = x;
            if (textRowHeight > rowBackground->h)
                dr.y = y + (textRowHeight - rowBackground->h) / 2;
            else
                dr.y = y;
            sr.x = 0;
            sr.y = 0;
            sr.w = rect.w;
            sr.h = textRowHeight;
            SDL_UpperBlit(rowBackground, &sr, dst, &dr);
        }
        if (i == cur) {
            if (selectedBackground) {
                dr.x = x;
                if (textRowHeight > selectedBackground->h)
                    dr.y = y + (textRowHeight - selectedBackground->h) / 2;
                else
                    dr.y = y;
                sr.x = 0;
                sr.y = 0;
                sr.w = rect.w;
                sr.h = textRowHeight;
                SDL_UpperBlit(selectedBackground, &sr, dst, &dr);
                color = colorBC;
            } else {
                color = colorBC;
            }
        }
        if (customBackground && i == cur) {
            sr.x = x + 18;
            sr.y = y + (textRowHeight - customBackground->h) / 2;
            SDL_UpperBlit(customBackground, NULL, dst, &sr);
        }
        if (item && item->value) {
            sr.x = x + 5;
            sr.y = y + (textRowHeight - (item->value)->h) / 2;
            sr.w = (item->value)->w;
            sr.h = (item->value)->h;
            SDL_UpperBlit(item->value, NULL, dst, &sr);
            sr.x = (short)x + sr.w + 20;
        } else {
            sr.x = x + 20;
        }
        sr.y = y + (textRowHeight - fontH) / 2;
        sr.w = rect.w - 40;
        sr.h = textRowHeight;
        if (item == NULL) {
            drawText(dst, font, sr, languageString(STR_LOADING), color, 0, shadowSelected ? i == cur : false);
        } else {
            if (!item->showThumbnail) {
                if (item->label.length() <= 48) {
                    drawHighlighted(dst, font, sr, item->label.c_str(), color, 0, shadowSelected ? i == cur : false,
                                    searchText.c_str(),
                                    item->displayLabel.length() ? item->displayLabel.c_str() : item->label.c_str());
                } else {
                    strncpy(shortLabel, item->label.c_str(), 64);
                    drawHighlighted(dst, font, sr, shortLabel, color, 0, shadowSelected ? i == cur : false,
                                    searchText.c_str(),
                                    item->displayLabel.length() ? item->displayLabel.c_str() : item->label.c_str());
                }
            } else {
                if (item->thumbnailSurface == NULL)
                    if (item->thumbnailPath.length() != 0)
                        item->thumbnailSurface = loadImage(item->thumbnailPath.c_str(), g_backBuffer, 0);
                if (item->thumbnailSurface) {
                    dr.x = x + 20;
                    int size = 71;
                    if (textRowHeight >= (item->thumbnailSurface)->h ||
                        textRowHeight >= (item->thumbnailSurface)->w) {
                        dr.y = y + (textRowHeight - (item->thumbnailSurface)->h) / 2;
                        SDL_UpperBlit(item->thumbnailSurface, NULL, dst, &dr);
                    } else {
                        dr.y = y;
                        sr.x = (item->thumbnailSurface->w - 71) / 2;
                        sr.y = (item->thumbnailSurface->h - 71) / 2;
                        sr.w = 71;
                        sr.h = 71;
                        SDL_UpperBlit(item->thumbnailSurface, &sr, dst, &dr);
                    }
                    sr.x = x + 111;
                    sr.w = rect.w - 111;
                } else {
                    sr.x = x + 20;
                    sr.w = rect.w - 20;
                }
                sr.y = y + 20;
                sr.h = textRowHeight / 2 - 20;
                drawHighlighted(dst, font, sr, item->label.c_str(), color, 0, shadowSelected ? i == cur : false,
                                searchText.c_str(),
                                item->displayLabel.length() ? item->displayLabel.c_str() : item->label.c_str());
                sr.y = y + textRowHeight / 2;
                drawText(dst, font, sr, item->subtitle.c_str(), g_selectedColor, 0, shadowSelected ? i == cur : false);
            }
            if (favoriteMark && item->favorite) {
                sr.x = rect.w + x - favoriteMark->w - previewWidth - 20;
                sr.y = y + (textRowHeight - favoriteMark->h) / 2;
                SDL_UpperBlit(favoriteMark, NULL, dst, &sr);
            }
        }
    }
}

// 0x204d4: key input, ListMenu's.
int TextMenuDrawer::handleKey()
{
    return ListMenu::handleKey();
}
// =================================================================================================
// 0x20500-0x23548: TextMenu and the ROM cache database (scan, gamelist.xml import).
// =================================================================================================
#include <sys/time.h>
#include <fstream>
#include <iterator>
#include "rapidxml.hpp"   // scripts/fetch-rapidxml.sh (deps "rapidxml" in src/matches.tsv)

#include "SQLiteApi.h"

const char *getLanguageFile();                                  // 0x14ba18 (Settings.c)
char *pinyinInitials(const char *s);                            // 0x143ae4 (Pinyin.cpp)
int countDirectoryItems(std::string dir, bool (*filter)(const char *, const char *)); // 0x12d400

// 0x20500: a TextMenuDrawer that owns its rows (TextItem*).
TextMenu::TextMenu(bool noSelectedBg, bool stripes, int rows, const char *background)
    : TextMenuDrawer(noSelectedBg, stripes, rows, background)
{
}

// 0x20570
TextMenu::~TextMenu()
{
    freeListItem();
}

// 0x205f8: free every item (slot 18).
void TextMenu::freeListItem()
{
    for (int i = 0; i < items.size(); i++) {
        TextItem *item = (TextItem *)items[i];
        printf("TextMenu::freeListItem %d %s\n", i, item->label.c_str());
        delete item;
    }
    puts("TextMenu::freeListItem end");
    items.clear();
}

// 0x206e0: reload (slot 13).
void TextMenu::clearItems()
{
    freeListItem();
}

// 0x20714: the row at index, or NULL.
void *TextMenu::getItem(int index)
{
    if (index >= 0 && index < items.size())
        return items[index];
    else
        return 0;
}

// 0x2079c: remove the item at index (slot 12).
void TextMenu::removeItem(int index)
{
    if (index >= 0 && index < items.size()) {
        TextItem *item = (TextItem *)items[index];
        items.erase(items.begin() + index);
        delete item;
    }
}

// 0x20898
void TextMenu::addItem(void *item)
{
    items.push_back(item);
}

// 0x208d0: insert before the first item cmp() does not rank after.
void TextMenu::insertSorted(void *item, int (*cmp)(TextItem *, TextItem *))
{
    int i = 0;
    while (i < items.size()) {
        if (cmp((TextItem *)item, (TextItem *)items[i]) <= 0)
            break;
        i++;
    }
    if (i < items.size())
        items.insert(items.begin() + i, item);
    else
        items.push_back(item);
}

// 0x20a3c: append unless an item with the same label is listed; true if added.
bool TextMenu::addUnique(TextItem *item)
{
    for (int i = 0; i < items.size(); i++) {
        if (item->label.compare(((TextItem *)items[i])->label) == 0)
            return false;
    }
    items.push_back((void *)item);
    return true;
}

// 0x20b1c: strip trailing slashes and return the last path component (same as GlobalSearch.cpp's).
char *romFolderName(char *path)
{
    int len = strlen(path);
    if (len > 0) {
        for (int i = len - 1; i > 0; i--) {
            if (path[i] == '/') {
                if (i < len - 1)
                    return path + i + 1;
                path[i] = '\0';
            }
        }
    }
    return path;
}

// 0x20bd4: rebuild <table>_roms from a gamelist.xml (<gameList><game><path/><name/><image/>...);
// returns the rows added.
int importGameList(const char *xml, int arg, const char *table, sqlite3 *db)
{
    char *err;
    std::string name;
    std::string path;
    std::string image;
    int count = 0;
    int rc = -1;
    char *sql = NULL;
    int cost;
    rapidxml::xml_node<> *game;
    const int bufSize = 512;
    char *dir = strdup(xml);
    char *slash = strrchr(dir, '/');
    if (slash)
        *slash = '\0';
    cost = 0;
    int t1 = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int t0 = tv.tv_sec * 1000000 + tv.tv_usec;
    if (db) {
        sql = (char *)malloc(bufSize);
        snprintf(sql, bufSize, "DROP TABLE %s_roms;", table);
        if (sqlite3_exec(db, sql, NULL, NULL, &err) != 0) {
            fprintf(stderr, "db exec at %d error: %s\n", 3581, err);
            sqlite3_free(err);
        }
        snprintf(sql, bufSize,
                 "CREATE TABLE %s_roms (id INTEGER PRIMARY KEY AUTOINCREMENT,disp TEXT NOT NULL,path TEXT NOT NULL,imgpath TEXT NOT NULL,type INTEGER DEFAULT 0,ppath TEXT NOT NULL,pinyin TEXT NOT NULL,cpinyin TEXT NOT NULL);",
                 table);
        rc = sqlite3_exec(db, sql, NULL, NULL, &err);
        if (rc != 0) {
            fprintf(stderr, "db exec at %d error: %s\n", 3596, err);
            sqlite3_free(err);
        }
    }
    printf("read xml from %s\n", xml);
    if (access(xml, 0) == 0) {
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        rapidxml::xml_document<> doc;
        std::ifstream file(xml);
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
        rapidxml::xml_node<> *root = doc.first_node("gameList");
        if (root) {
            for (game = root->first_node("game"); game; game = game->next_sibling()) {
                rapidxml::xml_node<> *node = game->first_node("path");
                if (!node) {
                    puts("no 'path' in xml");
                    break;
                }
                if (node->value()) {
                    path = dir;
                    path += "/";
                    path += node->value();
                }
                const char *ext = strrchr(path.c_str(), '.');
                if (strncasecmp(ext, ".zip", 4) != 0) {
                    if (findEmuForFile(path.c_str()) < 0)
                        continue;
                }
                node = game->first_node("name");
                if (!node) {
                    puts("no 'name' in xml");
                    break;
                }
                if (node->value())
                    name = node->value();
                node = game->first_node("image");
                if (!node) {
                    puts("no 'image' in xml");
                    break;
                }
                if (node->value()) {
                    image = dir;
                    image += "/";
                    image += node->value();
                }
                if (name.length() && path.length() && image.length()) {
                    if (access(path.c_str(), 0) == 0) {
                        if (rc == 0) {
                            gettimeofday(&tv, NULL);
                            t1 = tv.tv_sec * 1000000 + tv.tv_usec;
                            std::string pinyin;
                            char *py = pinyinInitials(name.c_str());
                            if (py)
                                pinyin = py;
                            snprintf(sql, bufSize,
                                     "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,0,'.',?,?);",
                                     table);
                            sqlite3_stmt *stmt;
                            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                                sqlite3_bind_text(stmt, 1, name.c_str(), name.length(), NULL);
                                sqlite3_bind_text(stmt, 2, path.c_str(), path.length(), NULL);
                                sqlite3_bind_text(stmt, 3, image.c_str(), image.length(), NULL);
                                sqlite3_bind_text(stmt, 4, pinyin.c_str(), pinyin.length(), NULL);
                                sqlite3_bind_text(stmt, 5, pinyin.c_str(), pinyin.length(), NULL);
                                int r = sqlite3_step(stmt);
                                if (r == 101 || r == 100) {
                                    count++;
                                } else {
                                    const char *e = sqlite3_errmsg(db);
                                    fprintf(stderr, "db exec at %d error : %s\n", 3700, sqlite3_errmsg(db));
                                }
                                sqlite3_finalize(stmt);
                            } else {
                                fprintf(stderr, "db exec at %d error : %s\n", 3704, sqlite3_errmsg(db));
                            }
                            if (py)
                                free(py);
                            gettimeofday(&tv, NULL);
                            cost += tv.tv_sec * 1000000 + tv.tv_usec - t1;
                        }
                        g_progressCur++;
                    }
                }
            }
        }
        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        printf("%d items added cost %dms\n", count, cost / 1000);
    }
    sync();
    free(dir);
    if (sql)
        free(sql);
    gettimeofday(&tv, NULL);
    t0 = tv.tv_sec * 1000000 + tv.tv_usec - t0;
    printf("total cost %dms\n", t0 / 1000);
    return count;
}

// 0x21818: true if a directory entry is not a ROM of this emulator (hidden files, BIOS, licenses,
// saves, or an extension outside the emulator's list; a .bin with a matching .cue).
bool isHiddenRomFile(EmuInfo *emu, const char *path, const char *name, int hideBios)
{
    if (hideBios && strncmp(name, "[BIOS]", 6) == 0)
        return true;
    if (name[0] == '.')
        return true;
    if (strncmp(name, "neogeo.zip", 10) == 0)
        return true;
    if (strncmp(name, "gngeo_data.zip", 14) == 0)
        return true;
    if (strncmp(name, "COPYING", 7) == 0)
        return true;
    if (strncasecmp(name, "LICENSE", 7) == 0)
        return true;
    const char *ext = strrchr(name, '.');
    if (ext == NULL || emu == NULL)
        return true;
    if (emu->extensions.size() != 0) {
        for (int i = 0; i < emu->extensions.size(); i++) {
            if (strncasecmp(ext + 1, emu->extensions[i].c_str(), emu->extensions[i].size()) == 0)
                return false;
        }
    } else {
        if (ext && strncmp(ext, ".db", 3) == 0)
            return true;
        if (ext && strncmp(ext, ".sav", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".svs", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".gz", 3) == 0)
            return true;
        if (ext && strncmp(ext, ".gqs", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".png", 4) == 0)
            return true;
        if (ext && strncmp(ext, ".bin", 4) == 0) {
            std::string bin(path);
            size_t dot = bin.rfind(".");
            std::string cue = bin.substr(0, dot) + ".cue";
            if (access(cue.c_str(), 0) == 0) {
                printf("%s exists, hide %s\n", cue.c_str(), bin.c_str());
                return true;
            }
        }
        return false;
    }
    return true;
}

// 0x21d50: add the ROMs under dir to <table>_roms (recursing once into subfolders, which get a
// type 1 row); returns the rows added here.
int scanRomDir(std::string dir, std::string imgDir, EmuInfo *emu, const char *table, bool sub, sqlite3 *db,
               char *path, char *name, int nameSize, char *sql, int sqlSize, unsigned long *cost, int *added)
{
    std::string disp;
    std::string romPath;
    std::string imgPath;
    size_t slash = dir.rfind("/");
    std::string ppath(dir);
    int count = 0;
    if (slash != -1)
        ppath = dir.substr(slash + 1);
    puts("handleDir start");
    DIR *d;
    if ((d = opendir(dir.c_str())) != NULL) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            sprintf(path, "%s/%s", dir.c_str(), ent->d_name);
            if (ent->d_type == DT_REG) {
                if (isHiddenRomFile(emu, path, ent->d_name, emu->hideBios))
                    continue;
                strncpy(name, ent->d_name, nameSize - 1);
                char *dot = strrchr(name, '.');
                if (dot)
                    *dot = '\0';
                unsigned long t = milliseconds();
                disp = name;
                romPath = path;
                sprintf(path, "%s/%s.png", imgDir.c_str(), name);
                imgPath = path;
                std::string en;
                std::string ch;
                char *py = NULL;
                char *pyEn = NULL;
                char *pyCh = NULL;
                if (emu->hasCpinyin) {
                    const char *enName = NULL;
                    const char *chName = NULL;
                    enName = GetGameName("en.lang", name);
                    chName = GetGameName("ch.lang", name);
                    if (enName)
                        pyEn = pinyinInitials(enName);
                    if (chName)
                        pyCh = pinyinInitials(chName);
                    printf("dispname=%s,endisp=%s,chdisp=%s,pyen=%s,pych=%s\n", name, enName, chName, pyEn,
                           pyCh);
                    if (pyEn == NULL || pyCh == NULL) {
                        py = pinyinInitials(disp.c_str());
                        if (pyEn == NULL)
                            pyEn = py;
                        if (pyCh == NULL)
                            pyCh = py;
                    }
                } else {
                    py = pinyinInitials(disp.c_str());
                    pyCh = py;
                    pyEn = pyCh;
                }
                en = pyEn;
                ch = pyCh;
                printf("inset to db shortname %d, name %s, enstr %s, chstr %s\n", emu->hasCpinyin, disp.c_str(),
                       en.c_str(), ch.c_str());
                snprintf(sql, sqlSize,
                         "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,?,?,?,?);",
                         table);
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                    sqlite3_bind_text(stmt, 1, disp.c_str(), disp.length(), NULL);
                    sqlite3_bind_text(stmt, 2, romPath.c_str(), romPath.length(), NULL);
                    sqlite3_bind_text(stmt, 3, imgPath.c_str(), imgPath.length(), NULL);
                    sqlite3_bind_int(stmt, 4, 0);
                    if (!sub)
                        sqlite3_bind_text(stmt, 5, ".", 1, NULL);
                    else
                        sqlite3_bind_text(stmt, 5, ppath.c_str(), ppath.length(), NULL);
                    sqlite3_bind_text(stmt, 6, en.c_str(), en.length(), NULL);
                    sqlite3_bind_text(stmt, 7, ch.c_str(), ch.length(), NULL);
                    int rc = sqlite3_step(stmt);
                    if (rc == 101 || rc == 100) {
                        (*added)++;
                        count++;
                    } else {
                        const char *err = sqlite3_errmsg(db);
                        fprintf(stderr, "db exec at %d error : %s\n", 3908, sqlite3_errmsg(db));
                    }
                    sqlite3_finalize(stmt);
                } else {
                    fprintf(stderr, "db exec at %d error : %s\n", 3912, sqlite3_errmsg(db));
                }
                if (py)
                    free(py);
                *cost += milliseconds() - t;
                g_progressCur++;
            } else if (ent->d_type == DT_DIR && !sub && strncmp(ent->d_name, ".", 1) != 0) {
                std::string subdir = dir + "/" + ent->d_name;
                if (scanRomDir(subdir, imgDir, emu, table, true, db, path, name, nameSize, sql, sqlSize, cost, added) > 0) {
                    sprintf(path, "%s/%s", dir.c_str(), ent->d_name);
                    snprintf(sql, sqlSize,
                             "INSERT INTO %s_roms (disp, path, imgpath, type, ppath, pinyin, cpinyin) VALUES(?,?,?,1,'.','','');",
                             table);
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == 0) {
                        sqlite3_bind_text(stmt, 1, ent->d_name, strlen(ent->d_name), NULL);
                        sqlite3_bind_text(stmt, 2, path, strlen(path), NULL);
                        sqlite3_bind_text(stmt, 3, path, strlen(path), NULL);
                        int rc = sqlite3_step(stmt);
                        if (rc == 101 || rc == 100) {
                            (*added)++;
                            count++;
                        } else {
                            const char *err = sqlite3_errmsg(db);
                            fprintf(stderr, "db exec at %d error : %s\n", 3957, sqlite3_errmsg(db));
                        }
                        sqlite3_finalize(stmt);
                    } else {
                        fprintf(stderr, "db exec at %d error : %s\n", 3961, sqlite3_errmsg(db));
                    }
                }
            }
        }
        closedir(d);
        printf("closedir %s %p\n", dir.c_str(), d);
    }
    printf("handleDir end newly add %d roms, total %d roms now\n", count, *added);
    return count;
}

// 0x2291c: rebuild <table>_roms from the emulator's ROM folder.
int rebuildRomCache(EmuInfo *emu, int arg, const char *table, sqlite3 *db)
{
    if (db == NULL) {
        puts("readFromFS nil db");
        return 0;
    }
    char name[128];
    char *err;
    int added = 0;
    int rc = -1;
    int ret = 0;
    char *sql = NULL;
    const int bufSize = 512;
    char *path = (char *)malloc(emu->path.size() + 256);
    unsigned long cost = 0;
    int unused = 0;
    unsigned long t = milliseconds();
    sql = (char *)malloc(bufSize);
    snprintf(sql, bufSize, "DROP TABLE %s_roms;", table);
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != 0) {
        fprintf(stderr, "db exec at %d error: %s\n", 4003, err);
        sqlite3_free(err);
    }
    snprintf(sql, bufSize,
             "CREATE TABLE %s_roms (id INTEGER PRIMARY KEY AUTOINCREMENT,disp TEXT NOT NULL,path TEXT NOT NULL,imgpath TEXT NOT NULL,type INTEGER DEFAULT 0,ppath TEXT NOT NULL,pinyin TEXT NOT NULL,cpinyin TEXT NOT NULL);",
             table);
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != 0) {
        fprintf(stderr, "db exec at %d error: %s\n", 4018, err);
        sqlite3_free(err);
    }
    printf("read from %s\n", emu->path.c_str());
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    ret = scanRomDir(emu->path, emu->imagePath, emu, table, false, db, path, name, 128, sql, bufSize, &cost, &added);
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    printf("%d items added cost %ldms\n", added, cost / 1000);
    sync();
    free(path);
    if (sql)
        free(sql);
    t = milliseconds() - t;
    printf("total cost %ldms\n", t / 1000);
    return ret;
}

// 0x22c88: directory filter for countDirectoryItems: a known ROM type or a .zip, no BIOS files.
bool isRomFile(const char *name, const char *dir)
{
    if (strncmp(name, "[BIOS]", 6) == 0)
        return false;
    if (findEmuForFile(name) >= 0)
        return true;
    if (strncmp(name, "neogeo.zip", 10) == 0)
        return false;
    if (strncmp(name, "gngeo_data.zip", 14) == 0)
        return false;
    const char *ext = strrchr(name, '.');
    if (ext && strncasecmp(ext, ".zip", 4) == 0)
        return true;
    return false;
}

// 0x22d90: open the emulator's ROM cache (<path>/<name>_cache6.db) and count the rows for a folder or
// a search; an empty cache is rebuilt from gamelist.xml, else from the file system.
int prepareRomCache(EmuInfo *emu, int arg, std::string dir, std::string search, char *buf, int size)
{
    int count = 0;
    int rc = -1;
    int t = 0;
    char *base = NULL;
    char *path = strdup(emu->path.c_str());
    base = romFolderName(path);
    sqlite3 *db;
    sqlite3_stmt *stmt;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec;
    snprintf(buf, size, "%s/%s_cache6.db", emu->path.c_str(), base);
    rc = sqlite3_open(buf, &db);
    if (rc == 0) {
        if (search.length() == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE ppath='%s'", base,
                     dir.length() ? dir.c_str() : ".");
        else if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", base,
                     search.c_str());
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", base,
                     search.c_str());
        if (sqlite3_prepare_v2(db, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                count = sqlite3_column_int(stmt, 0);
                if (count > 0)
                    goto done;
            }
        } else {
            printf("db exec at %d fail: %s\n", 4100, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    } else {
        printf("open db %s fail: %s\n", buf, sqlite3_errmsg(db));
        snprintf(buf, size, "%s/%s_cache6.db", emu->path.c_str(), base);
        unlink(buf);
        rc = sqlite3_open(buf, &db);
        printf("open again %s : %s\n", buf, sqlite3_errmsg(db));
    }
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec - t;
    printf("count %d item from db cost %d ms ppath %s\n", count, t, dir.c_str());
    if (count == 0 && search.length() == 0) {
        count = importGameList(emu->gameList.c_str(), arg, base, rc == 0 ? db : NULL);
        if (count > 0)
            goto done;
        if (g_progressTotal == 0)
            g_progressTotal = countDirectoryItems(emu->path, isRomFile);
        count = rebuildRomCache(emu, arg, base, rc == 0 ? db : NULL);
    }
done:
    if (rc == 0) {
        printf("rc is %d , close db %p\n", rc, db);
        sqlite3_close(db);
    }
    free(path);
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000000 + tv.tv_usec - t;
    printf("prepare db in phase2 cost %d ms\n", t);
    return count;
}

// 0x23318: the ROM list of one emulator folder (or a search in it), backed by its cache database.
DBCachedTextMenu *buildRomList(EmuInfo *emu, int arg, std::string *dir, std::string *search)
{
    DBCachedTextMenu *menu = new DBCachedTextMenu(emu->label.c_str(), emu, arg, *search);
    printf("create_rom_browser_menu %p, search %s\n", menu, search->c_str());
    int count = 0;
    const int bufSize = 512;
    char *buf = (char *)malloc(bufSize);
    char *path = strdup(emu->path.c_str());
    char *base = romFolderName(path);
    count = prepareRomCache(emu, arg, *dir, *search, buf, bufSize);
    menu->setTotal(count);
    snprintf(buf, bufSize, "%s/%s_cache6.db", emu->path.c_str(), base);
    menu->open(buf, base, dir->length() ? dir->c_str() : ".");
    free(buf);
    free(path);
    return menu;
}
// =================================================================================================
// 0x23548-0x2849c: ROM cache rows, icon menus and settings rows.
// =================================================================================================


int removeRecent(std::list<RecentItem> *, const char *);
extern SDL_Surface *g_gameIcon; // 0x1821f8
extern SDL_Surface *g_folderIcon; // 0x1821f4

// 0x23548: background job of the ROM lists: loads rows [offset, offset+len) of a folder (or of a
// search, by pinyin or Chinese pinyin) from the cache database into the items cache, skipping rows
// already cached; each row is a game (type 0) or a subfolder, with its favorite mark, and its
// image path goes to the data cache.
void DBCacheWorker::run()
{
    if (db != NULL) {
        std::string key;
        int i = 0;
        for (i = 0; i < len; i++) {
            snprintf(sqlBuffer, 512, "%d", offset + i);
            key = table + std::string("_") + sqlBuffer;
            void *cached = items->get(key);
            if (cached == NULL) break;
        }
        if (i < len) {
            FILE *f = fopen("/mnt/SDCARD/Roms/favourite.json", "r");
            TextItem *item = NULL;
            if (search.length() == 0) {
                snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE ppath='%s' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), dir.c_str(), offset + i, len - i);
            } else {
                if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
                    snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len - i);
                else
                    snprintf(sqlBuffer, 512, "SELECT * FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%' ORDER BY type DESC, disp LIMIT %d,%d;", dbName.c_str(), search.c_str(), offset + i, len - i);
                printf("mDBStrBuf = %s\n", sqlBuffer);
            }
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sqlBuffer, -1, &stmt, NULL) == 0) {
                for (; i < len; i++) {
                    int rc;
                    if ((rc = sqlite3_step(stmt)) == 100) {
                        const char *disp = (const char *)sqlite3_column_text(stmt, 1);
                        const char *path = (const char *)sqlite3_column_text(stmt, 2);
                        const char *image = (const char *)sqlite3_column_text(stmt, 3);
                        int type = sqlite3_column_int(stmt, 4);
                        const char *parent = (const char *)sqlite3_column_text(stmt, 5); // Retain the unused column read.
                        const char *pinyin = (const char *)sqlite3_column_text(stmt, 6);
                        const char *cpinyin = (const char *)sqlite3_column_text(stmt, 7);
                        const char *label = NULL;
                        if (emu->hasCpinyin) label = GetGameName(getLanguageFile(), disp);
                        if (label == NULL) label = disp;
                        if (sqlite3_column_type(stmt, 1) == 3 && sqlite3_column_type(stmt, 2) == 3 && sqlite3_column_type(stmt, 4) == 1 && sqlite3_column_type(stmt, 5) == 3) {
                            if (type == 0) {
                                bool preview = search.length() != 0 && previewSearchResults;
                                item = new TextItem(label, preview ? NULL : g_gameIcon, new EmuAction(emu, label, path, image, rowWindowId, true, type), true);
                                item->subtitle = emu->label;
                                item->favorite = isFavorite(NULL, f, path);
                                item->showThumbnail = preview;
                            } else {
                                EmuAction *action = new EmuAction(emu, label, path, image, rowWindowId, false, type);
                                action->folderArg = disp;
                                item = new TextItem(label, g_folderIcon, action, true);
                            }
                            if (emu->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
                                item->displayLabel = cpinyin;
                            else
                                item->displayLabel = pinyin;
                            snprintf(sqlBuffer, 512, "%d", offset + i);
                            key = table + std::string("_") + sqlBuffer;
                            items->put(key, item);
                            printf("put %s to cache , type=%d, path =%s, py=%s\n", key.c_str(), type, path, item->displayLabel.c_str());
                        }
                        if (sqlite3_column_type(stmt, 3) == 3 && image[0] != 0) data->put(key, strdup(image));
                    } else {
                        printf("get item %d, db step return %d\n", offset + i, rc);
                        break;
                    }
                }
                sqlite3_finalize(stmt);
            } else
                printf("get item %d, exec fail : %s\n", offset + i, sqlite3_errmsg(db));
            if (f) fclose(f);
        }
    }
}
// 0x2407c: the rows are in: ask for a redraw.
void DBCacheWorker::finished()
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = EVENT_REDRAW;
    SDL_PushEvent(&event);
}

// 0x240b4: page up: move as usual, then queue the rows that came into view.
int DBCachedTextMenu::pageUp()
{
    int result = ListMenu::pageUp();
    prefetchPrevious();
    return result;
}

// 0x240ec: the same for page down.
int DBCachedTextMenu::pageDown()
{
    int result = ListMenu::pageDown();
    prefetchNext();
    return result;
}

// 0x24128: the same for down.
int DBCachedTextMenu::moveDown()
{
    int result = ListMenu::moveDown();
    prefetchNext();
    return result;
}

// 0x24164: the same for up.
int DBCachedTextMenu::moveUp()
{
    int result = ListMenu::moveUp();
    prefetchPrevious();
    return result;
}

// 0x241a0: queue the six rows from the cursor down.
void DBCachedTextMenu::prefetchNext()
{
    if (cacheDb) {
        int offset = cur;
        int len = 6;
        if (offset + len > total) len = total - offset;
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, offset, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
}

// 0x242dc: queue the rows above the cursor (up to five, and the cursor row).
void DBCachedTextMenu::prefetchPrevious()
{
    if (cacheDb) {
        int offset = cur - 5;
        int len = 6;
        if (offset < 0) {
            offset = 0;
            len = cur + 1;
        }
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, offset, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
}

// 0x24410: the row at index if it has been loaded, else NULL (not loaded yet).
void *DBCachedTextMenu::getItem(int index)
{
    snprintf(keyBuffer, 640, "%d", index);
    std::string key = dbName + std::string("_") + keyBuffer;
    void *item = items.get(key);
    if (item) return item;
    return NULL;
}

// 0x24584: a row's image path if loaded, else NULL.
void *DBCachedTextMenu::getItemData(int index)
{
    snprintf(keyBuffer, 640, "%d", index);
    std::string key = dbName + std::string("_") + keyBuffer;
    void *item = data.get(key);
    if (item) return item;
    return NULL;
}

// 0x246f8: back to the list: restore the position and load the visible page.
void DBCachedTextMenu::restorePosition(int savedCur, int savedStart, int savedEnd)
{
    ListMenu::restorePosition(savedCur, savedStart, savedEnd);
    int len = pageEnd - pageStart + 1;
    if (pageEnd == 0) len = 6;
    DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, pageStart, len, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
    workers.addWorker(worker);
}

// 0x24834: opens the cache database at path for the table prefix and folder, and loads the first
// rows.
int DBCachedTextMenu::open(const char *path, const char *table, const char *folder)
{
    if (cacheDb) {
        sqlite3_close(cacheDb);
        cacheDb = NULL;
    }
    int rc = sqlite3_open(path, &cacheDb);
    name = path;
    tablePrefix = table;
    dir = folder;
    if (rc != 0)
        printf("open db %s fail, db is %p\n", path, cacheDb);
    else {
        int len = pageEnd - pageStart + 1;
        if (pageEnd == 0) len = 5;
        DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, 0, 6, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
        workers.addWorker(worker);
    }
    return rc;
}

// 0x24a14: deletes the ROM at index: the file itself, its database row, its favorite and its
// Recent entry; then recounts the folder and reloads the page.
void DBCachedTextMenu::removeItem(int index)
{
    TextItem *item = (TextItem *)getItem(index);
    if (item && item->action) {
        EmuAction *action = (EmuAction *)item->action;
        if (action->rompath.length() != 0) {
            unlink(action->rompath.c_str());
            snprintf(keyBuffer, 640, "DELETE FROM %s_roms WHERE path='%s'", tablePrefix.c_str(), action->rompath.c_str());
            sqlite3_stmt *stmt;
            char *err;
            if (sqlite3_exec(cacheDb, keyBuffer, NULL, NULL, &err) == 0) {
                removeFavorite(item->label.c_str(), "/mnt/SDCARD/Roms/favourite.json");
                removeRecent(&g_recentGames, action->rompath.c_str());
                snprintf(keyBuffer, 640, "SELECT COUNT(*) FROM %s_roms WHERE ppath='%s'", tablePrefix.c_str(), dir.length() == 0 ? "." : dir.c_str());
                if (sqlite3_prepare_v2(cacheDb, keyBuffer, -1, &stmt, NULL) == 0) {
                    if (sqlite3_step(stmt) == 100) {
                        int n = sqlite3_column_int(stmt, 0);
                        setTotal(n);
                        items.clear();
                        data.clear();
                        restorePosition(cur, pageStart, pageEnd);
                    }
                }
                sqlite3_finalize(stmt);
            } else {
                printf("db exec at %d fail: %s\n", 0x112b, err);
                sqlite3_free(err);
            }
        }
    }
}

// 0x24d28: free callbacks of the row cache (TextItem) and the data cache (strings).
static void deleteCachedTextItem(SDL_Surface *value)
{
    TextItem *item = (TextItem *)value;
    delete item;
}

// 0x24d6c
static void freeCachedData(SDL_Surface *value)
{
    void *data = value;
    free(data);
}

// 0x24d98: filters the list by a search string (pinyin initials) if anything matches: remembers the
// position in the folder, then shows the first matches.
void DBCachedTextMenu::setSearch(const char *search)
{
    printf("search %s\n", search);
    searchText = search;
    if (searchText.length() != 0) {
        const int size = 512;
        char *buf = (char *)malloc(size);
        sqlite3_stmt *stmt;
        if (emuInfo->hasCpinyin && strcmp(getLanguageFile(), "ch.lang") == 0)
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND cpinyin like '%%%s%%'", tablePrefix.c_str(), search);
        else
            snprintf(buf, size, "SELECT COUNT(*) FROM %s_roms WHERE type=0 AND pinyin like '%%%s%%'", tablePrefix.c_str(), search);
        if (sqlite3_prepare_v2(cacheDb, buf, -1, &stmt, NULL) == 0) {
            if (sqlite3_step(stmt) == 100) {
                int n = sqlite3_column_int(stmt, 0);
                if (n > 0) {
                    printf("search %s found %d items\n", search, n);
                    if (dir.length() != 0 && dir.compare(".") != 0)
                        setRomListPosition(&g_romListPositions, emuInfo->path + "/" + dir, cur, pageStart, pageEnd);
                    else
                        setRomListPosition(&g_romListPositions, std::string(emuInfo->path), cur, pageStart, pageEnd);
                    setTotal(n);
                    items.clear();
                    data.clear();
                    cur = 0;
                    pageStart = 0;
                    pageEnd = 0;
                    DBCacheWorker *worker = new DBCacheWorker(cacheDb, tablePrefix, dbName, 0, 5, listWindowId, &items, &data, emuInfo, &dir, &searchText, false);
                    workers.addWorker(worker);
                }
            }
        } else
            printf("db exec at %d fail: %s\n", 0x1160, sqlite3_errmsg(cacheDb));
        sqlite3_finalize(stmt);
        free(buf);
    }
}

// 0x25240: a ROM list for one emulator (label, emu), whose rows' actions carry the window id arg;
// three background loaders.
DBCachedTextMenu::DBCachedTextMenu(const char *label, EmuInfo *emu, int arg, std::string search)
    : TextMenuDrawer(false, true, 6, NULL), workers(3), items(deleteCachedTextItem), data(freeCachedData)
{
    cacheDb = NULL;
    total = 0;
    dbName = label;
    listWindowId = arg;
    emuInfo = emu;
    searchText = search;
}

// 0x2540c
DBCachedTextMenu::~DBCachedTextMenu()
{
    if (cacheDb) sqlite3_close(cacheDb);
}

// 0x25510: one text row: label, icon (value), action (deleted with the row when ownsAction).
TextItem::TextItem(const char *text, SDL_Surface *surface, Action *act, bool owns)
{
    label = text;
    value = surface;
    action = act;
    ownsAction = owns;
    favorite = false;
    removed = false;
    showThumbnail = false;
    thumbnailSurface = NULL;
}

// 0x25620
TextItem::~TextItem()
{
    if (action && ownsAction) delete action;
    if (thumbnailSurface) SDL_FreeSurface(thumbnailSurface);
}
// 0x256f0: a main-menu style icon with a language-string label; menu is the window id it opens and
// arg its parameter (an EmuInfo for emulators). Note: callers pass the focused image as `normal`
// and the normal one as `selected`; drawList uses them that way round.
IconMenuItem::IconMenuItem(int id, const char *normal, const char *selected, SDL_Surface *bx, SDL_Surface *by, int menu, void *arg, bool own)
{
    labelId = id;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    if (!iconSurface) printf("load icon %s fail\n", normal);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    if (!iconSelectedSurface) printf("load icon %s fail\n", selected);
    icon = normal;
    iconSelected = selected;
    background = bx;
    backgroundSelected = by;
    param = arg;
    ownsParam = own;
    menuId = menu;
}

// 0x258d4: the same with a literal label (emulator and RetroArch icons).
IconMenuItem::IconMenuItem(const char *text, const char *normal, const char *selected, SDL_Surface *bx, SDL_Surface *by, int menu, void *arg, bool own)
{
    label = text;
    labelId = -1;
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, normal, g_backBuffer, false);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, selected, g_backBuffer, false);
    icon = normal;
    iconSelected = selected;
    background = bx;
    backgroundSelected = by;
    param = arg;
    ownsParam = own;
    menuId = menu;
}

// 0x25a8c
IconMenuItem::~IconMenuItem()
{
    if (ownsParam && param) free((void *)param);
    if (iconSurface) SDL_FreeSurface(iconSurface);
    if (iconSelectedSurface) SDL_FreeSurface(iconSelectedSurface);
}

// 0x25b5c: reload both icons from the current theme.
void IconMenuItem::reload()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
    if (iconSelectedSurface) SDL_FreeSurface(iconSelectedSurface);
    iconSelectedSurface = loadSkinImage(g_themePath, g_skinSubdir, iconSelected.c_str(), g_backBuffer, false);
}

// 0x25c68: the font of icon labels.
static TTF_Font *defaultIconFont()
{
    return g_iconFont;
}

Theme *findTheme(std::vector<Theme> *, const char *);
// 0x25c8c: a grid of icons: rows x columns per page, optional separator lines (stripes) and page
// dots, margins, and optionally the theme's grid offsets.
IconMenu::IconMenu(int rows, int columns, bool stripes, bool dots, int marginX, int marginY, bool themeOffsets) : ListMenu(true)
{
    gridColumns = columns;
    gridRows = rows;
    showStripes = stripes;
    showPageDots = dots;
    iconCropWidth = 0;
    iconCropHeight = 0;
    fontFn = defaultIconFont;
    labelOffsetY = 0;
    gridMarginX = marginX;
    gridMarginY = marginY;
    Theme *theme = findTheme(&g_themeList, g_themePath);
    useThemeOffsets = themeOffsets;
    if (themeOffsets && theme) {
        iconOffsetX = theme->gridIconOffsetX;
        iconOffsetY = theme->gridIconOffsetY;
        textOffsetY = theme->gridTextOffsetY;
    } else {
        iconOffsetX = 0;
        iconOffsetY = 0;
        textOffsetY = 0;
    }
}

// 0x25e48
IconMenu::~IconMenu()
{
    puts(__func__);
    freeItems();
    items.clear();
}

extern SDL_Surface *g_vertLine; // 0x1821cc
extern SDL_Surface *g_dotActive; // 0x1821a0
extern SDL_Surface *g_dotNormal; // 0x1821a4
extern SDL_Color g_textColor; // 0x182298
// 0x25efc: draws the visible page of the grid: separator lines, each icon with its background
// (crop, offsets), its label, the page dots and the "page/total" count.
void IconMenu::drawList(SDL_Surface *dst)
{
    SDL_Surface *surface, *background;
    int x, y, width, height;
    int i;
    unsigned j;
    TTF_Font *font;
    int index, row, column;
    IconMenuItem *item;
    SDL_Rect rect, src;
    SDL_Color color = g_textColor;
    if (gridMarginX > 0) {
        width = (640 - gridMarginX - gridMarginX) / gridColumns;
        x = gridMarginX;
    } else {
        width = 640 / gridColumns;
        x = 0;
    }
    if (gridMarginY > 0) {
        height = (360 - gridMarginY - gridMarginY) / gridRows;
        y = gridMarginY + 65;
    } else {
        height = 360 / gridRows;
        y = 60;
    }
    if (items.size() == 0) return;
    {
        font = fontFn();
        if (pageStart == 0 && pageEnd == 0) {
            pageEnd = gridRows * gridColumns - 1;
            cur = pageStart;
        }
        if (cur < pageStart) {
            if (gridRows > 1)
                pageStart = pageStart - gridRows * gridColumns;
            else
                pageStart = pageStart - gridRows;
            if (pageStart < 0) pageStart = 0;
            pageEnd = pageStart + gridRows * gridColumns - 1;
        }
        if (cur > pageEnd) {
            if (gridRows > 1)
                pageStart = pageEnd + 1;
            else {
                if (pageStart + gridRows + gridRows * gridColumns - 1 < items.size()) pageStart = pageStart + gridRows;
            }
            pageEnd = pageStart + gridRows * gridColumns - 1;
        }
        if ((unsigned)pageStart >= items.size()) pageStart = items.size() - 1;
        if ((unsigned)pageEnd >= items.size()) pageEnd = items.size() - 1;
        if (showStripes && g_vertLine) {
            rect.y = 60;
            for (i = 0; i < gridColumns - 1; i++) {
                rect.x = (i + 1) * width;
                SDL_UpperBlit(g_vertLine, NULL, dst, &rect);
            }
            if (g_listStripe) {
                rect.x = 0;
                for (i = 0; i < gridRows - 1; i++) {
                    rect.y = (i + 1) * height + 60;
                    SDL_UpperBlit(g_listStripe, NULL, dst, &rect);
                }
            }
        }
        for (i = pageStart; i <= pageEnd; i++) {
            index = i - pageStart;
            row = index / gridColumns;
            column = index - row * gridColumns;
            item = (IconMenuItem *)items.at(i);
            if (i == cur) {
                surface = item->iconSurface;
                background = item->background;
                color = g_textColor;
            } else {
                surface = item->iconSelectedSurface;
                background = item->backgroundSelected;
                color = g_selectedColor;
            }
            if (background) {
                rect.x = column * width + x + (width - background->w) / 2 + 2;
                rect.y = row * height + y + (height - background->h) / 2 + 2;
                SDL_UpperBlit(background, NULL, dst, &rect);
            }
            if (surface) {
                if (iconCropWidth > 0 && iconCropHeight > 0) {
                    rect.x = column * width + x + (width - iconCropWidth) / 2;
                    rect.y = row * height + y + 15;
                    src.x = (surface->w - iconCropWidth) / 2;
                    src.y = (surface->h - iconCropHeight) / 2;
                    src.w = iconCropWidth;
                    src.h = iconCropHeight;
                    if (useThemeOffsets) {
                        if (iconOffsetX) rect.x = iconOffsetX + column * width + (width - iconCropWidth) / 2;
                        if (iconOffsetY) rect.y = iconOffsetY + row * height;
                    }
                    SDL_UpperBlit(surface, &src, dst, &rect);
                    rect.x = column * width + x;
                    rect.y = row * height + y + iconCropHeight + 21;
                } else {
                    rect.x = column * width + x + (width - surface->w) / 2;
                    rect.y = row * height + y + (height - surface->h) / 2 - 10;
                    if (useThemeOffsets) {
                        if (iconOffsetX) rect.x = iconOffsetX + column * width + (width - surface->w) / 2;
                        if (iconOffsetY) rect.y = iconOffsetY + row * height;
                    }
                    SDL_UpperBlit(surface, NULL, dst, &rect);
                    rect.x = column * width + x;
                    if (labelOffsetY)
                        rect.y = row * height + y + height - labelOffsetY;
                    else
                        rect.y = row * height + y + height * 3 / 5;
                }
            } else {
                rect.x = column * width + x;
                if (labelOffsetY)
                    rect.y = row * height + y + height - labelOffsetY;
                else
                    rect.y = row * height + y + height * 3 / 5;
            }
            rect.w = width;
            rect.h = 18;
            if (useThemeOffsets && textOffsetY) rect.y = textOffsetY;
            if (item->label.size() != 0)
                drawText(dst, font, rect, item->label.c_str(), color, 1, i == cur);
            else
                drawText(dst, font, rect, languageString(item->labelId), color, 1, i == cur);
        }
        if (showPageDots && g_dotActive && g_dotNormal) {
            y = 390;
            width = items.size() * (g_dotActive->w + 5);
            x = (640 - width) / 2;
            rect.y = y;
            rect.w = g_dotActive->w;
            rect.h = g_dotActive->h;
            for (j = 0; j < items.size(); j++) {
                rect.x = (g_dotActive->w + 5) * j + x;
                if (j == cur)
                    SDL_UpperBlit(g_dotActive, NULL, dst, &rect);
                else
                    SDL_UpperBlit(g_dotNormal, NULL, dst, &rect);
            }
        }
        if (gridRows > 1) drawPageCount(dst, cur / (gridRows * gridColumns) + 1, (items.size() + gridRows * gridColumns - 1) / (gridRows * gridColumns));
    }
}

// 0x26fa0: the item at index.
void *IconMenu::getItem(int index)
{
    if (items.size() != 0) return items.at(index);
    return NULL;
}

// 0x27014: before a global search: counts the ROMs of every emulator and RetroArch app that has no
// cache yet (for the progress count) and builds those caches.
static int prepareGlobalRomCache()
{
    int total = 0;
    // The original leaves this buffer allocated; retain that behavior.
    const int size = 512;
    char *buf = (char *)malloc(size);
    char *path = NULL;
    char *base = NULL;
    int count;
    int n;
    std::vector<int> countsB, countsA;
    for (unsigned i = 0; i < g_rappFolders.size(); i++) {
        path = strdup(g_rappFolders[i].path.c_str());
        base = romFolderName(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_rappFolders[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)
            n = countDirectoryItems(g_rappFolders[i].path, isRomFile);
        else
            n = 0;
        countsB.push_back(n);
        total += n;
    }
    for (unsigned i = 0; i < g_emuFolders.size(); i++) {
        path = strdup(g_emuFolders[i].path.c_str());
        base = romFolderName(path);
        snprintf(buf, size, "%s/%s_cache6.db", g_emuFolders[i].path.c_str(), base);
        free(path);
        if (access(buf, 0) != 0)
            n = countDirectoryItems(g_emuFolders[i].path, isRomFile);
        else
            n = 0;
        countsA.push_back(n);
        total += n;
    }
    g_progressTotal = total;
    int unused = 0;
    for (unsigned i = 0; i < g_rappFolders.size(); i++) {
        n = countsB[i];
        if (n != 0) {
            count = prepareRomCache(&g_rappFolders[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_rappFolders[i].path.c_str(), count);
        }
    }
    for (unsigned i = 0; i < g_emuFolders.size(); i++) {
        n = countsA[i];
        if (n != 0) {
            count = prepareRomCache(&g_emuFolders[i], 24, ".", "", buf, size);
            printf("db %s %d items\n", g_emuFolders[i].path.c_str(), count);
        }
    }
    g_progressTotal = 0;
    return total;
}

// 0x27724: opens what the selected icon leads to: its menu window, or for the search icon
// (WIN_SEARCH) builds the missing caches behind the busy screen and opens the keyboard.
int IconMenu::onSelect()
{
    MenuWindow *window;
    IconMenuItem *item = (IconMenuItem *)getItem(cur);
    if (item != NULL) {
        if (item->menuId == WIN_SEARCH) {
            bool unused = false;
            int count = 0;
            g_busy = 1;
            BusyData busy;
            busy.type = WIN_SEARCH;
            SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &busy);
            count = prepareGlobalRomCache();
            TrimusWindow *ime = new ImeWindow(STR_SEARCH, new GlobalSearchAction, STR_SEARCH);
            g_busy = 0;
            SDL_WaitThread(thread, NULL);
            g_windowStack->push_back((TrimusWindow *)ime);
        } else {
            window = NULL;
            if (item->labelId >= 0)
                window = new MenuWindow(item->labelId, item->menuId, (void *)item->param);
            else
                window = new MenuWindow(item->label.c_str(), item->menuId, (void *)item->param);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
        }
        return 0;
    } else
        return -1;
}

// 0x2797c: deletes every item.
void IconMenu::freeItems()
{
    puts("IconMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        IconMenuItem *item = (IconMenuItem *)items.at(i);
        delete item;
    }
}

// 0x27a30
void IconMenu::addItem(void *item)
{
    items.push_back(item);
}

// 0x27a68: after a theme change: reload the icons and the theme's grid offsets.
void IconMenu::reloadTheme()
{
    if (loadedThemePath.compare(g_themePath) != 0) {
        for (int i = 0; i < items.size(); i++) {
            IconMenuItem *item = (IconMenuItem *)items.at(i);
            item->reload();
        }
        loadedThemePath = g_themePath;
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (useThemeOffsets && theme) {
            iconOffsetX = theme->gridIconOffsetX;
            iconOffsetY = theme->gridIconOffsetY;
            textOffsetY = theme->gridTextOffsetY;
        }
    }
}

// 0x27bc4: grid navigation: up/down within the column, left/right to the previous/next item across
// rows and pages (wrapping); A/Y open, B/X close, MENU passes through.
int IconMenu::handleKey()
{
    int key = handleGlobalInput(0);
    switch (key) {
        case BUTTON_UP: {
            int index = cur - pageStart;
            if (index >= gridColumns)
                cur = pageStart + index - gridColumns;
            else
                cur = pageStart + (gridRows - 1) * gridColumns + index % gridColumns;
            if (cur < pageStart) cur = pageStart;
            if (cur > pageEnd) cur = pageEnd;
            break;
        }
        case BUTTON_DOWN: {
            int index = cur - pageStart;
            if (index < (gridRows - 1) * gridColumns)
                cur = pageStart + index + gridColumns;
            else
                cur = pageStart + index % gridColumns;
            if (cur < pageStart) cur = pageStart;
            if (cur > pageEnd) cur = pageEnd;
            break;
        }
        case BUTTON_LEFT: {
            int previous = cur;
            if (cur % gridColumns != 0)
                previous--;
            else
                previous -= (gridColumns * (gridRows - 1) + 1);
            if (previous >= 0)
                cur = previous;
            else {
                int n = count();
                if (gridRows == 1) {
                    cur = n - 1;
                    pageEnd = cur;
                    pageStart = cur - gridColumns + 1;
                } else {
                    cur = n - 1;
                    pageEnd = cur;
                    pageStart = ((cur + gridRows * gridColumns - 1) / (gridRows * gridColumns)) * (gridRows * gridColumns);
                }
            }
            break;
        }
        case BUTTON_RIGHT: {
            int next = cur;
            if (cur % gridColumns == gridColumns - 1 && gridRows > 1)
                next += gridColumns * (gridRows - 1) + 1;
            else
                next++;
            int n = count();
            if (next < n)
                cur = next;
            else if (gridRows == 1) {
                cur = 0;
                pageStart = 0;
                pageEnd = gridRows * gridColumns - 1;
            } else {
                int start = ((cur + gridColumns * gridRows - 1) / (gridColumns * gridRows)) * (gridColumns * gridRows);
                int row = (cur % (gridRows * gridColumns) + (gridColumns - 1)) / gridColumns;
                int total = count();
                if (total > start) {
                    int remaining = total - start;
                    int rows = (gridColumns - 1 + remaining) / gridColumns;
                    if (rows < row) {
                        cur = (rows - 1) * gridColumns + start;
                        pageStart = start;
                        pageEnd = total - 1;
                    }
                } else {
                    cur = (row - 1) * gridColumns;
                    pageStart = 0;
                    pageEnd = gridRows * gridColumns - 1;
                }
            }
            break;
        }
        case BUTTON_A:
        case BUTTON_Y: return onSelect();
        case BUTTON_B:
        case BUTTON_X: return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        case BUTTON_HOTKEY1: break;
        case BUTTON_HOTKEY2: break;
        default: break;
    }
    return 0;
}

// 0x282d0: one settings row: label, icon and the number of its handler (what A or left/right do;
// see SettingMenu).
SettingMenuItem::SettingMenuItem(int id, const char *path, int arg)
{
    labelId = id;
    if (path != NULL)
        iconSurface = loadSkinImage(g_themePath, g_skinSubdir, path, g_backBuffer, false);
    else
        iconSurface = NULL;
    handler = arg;
    if (path) icon = path;
}

// 0x283b8
SettingMenuItem::~SettingMenuItem()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
}

// 0x28408: reload the icon from the current theme.
void SettingMenuItem::reload()
{
    if (iconSurface) SDL_FreeSurface(iconSurface);
    iconSurface = loadSkinImage(g_themePath, g_skinSubdir, icon.c_str(), g_backBuffer, false);
}
// =================================================================================================
// 0x2849c-0x2b788: The settings and device-information menus, display panel setup.
// =================================================================================================
#include "KeySettingWindow.h"
#include "KeyTestWindow2.h"
#include "WifiMenus.h"
extern "C" {
void setKeymap(const char *);
void setLanguageFile(const char *);
int MI_SYS_ReadUuid(unsigned long long *);
}

int removeAllDBFiles();
int removeEmulatorConfigs();
float maxCpuMHz();
int memoryMB();
std::string sdCardSpace();
// 0x2849c: the settings list (WIN_SETTING and WIN_FIXES): rows with an icon, a label and, for
// adjustable settings, the value between left/right arrows.
SettingMenu::SettingMenu() : TextMenu(true, true, 6, NULL)
{
    volumePreviewTimer = NULL;
    settingLeftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    rightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x28614: closing settings saves system.json (and applies a pending volume preview).
SettingMenu::~SettingMenu()
{
    freeListItem();
    saveSystemJson(g_keyShm, g_themePath, g_fontSize);
    g_settingsDirty = false;
    if (volumePreviewTimer) {
        SDL_RemoveTimer(volumePreviewTimer);
        setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    }
    if (settingLeftArrow) SDL_FreeSurface(settingLeftArrow);
    if (rightArrow) SDL_FreeSurface(rightArrow);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2877c: after a theme change: reload the row icons, arrows and highlight.
void SettingMenu::reloadTheme()
{
    if (themePath.compare(g_themePath) != 0) {
        for (int i = 0; i < items.size(); i++) {
            SettingMenuItem *item = (SettingMenuItem *)items[i];
            item->reload();
        }
        if (settingLeftArrow) SDL_FreeSurface(settingLeftArrow);
        if (rightArrow) SDL_FreeSurface(rightArrow);
        if (selectedBackground) SDL_FreeSurface(selectedBackground);
        settingLeftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
        rightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x28994: draws the visible rows: icon and label, and for adjustable settings the current value
// (volume, brightness, music volume, sleep timer, audio fix) between arrows; the Wi-Fi row shows
// the IP address when connected.
void SettingMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0};
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (row < 5 && g_listStripe) {
            dest.x = 0;
            dest.y = (row + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        }
        if (rowBackground) {   // never set: always NULL
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->iconSurface) {
            area.y = (60 - item->iconSurface->h) / 2 + y;
            SDL_UpperBlit(item->iconSurface, NULL, dst, &area);
            area.x = area.x + 60;
        }
        area.y = y;
        area.w = rect.w - (settingLeftArrow ? settingLeftArrow->w : 0) - (rightArrow ? rightArrow->w : 0) - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, shadowSelected ? i == cur : false);
        area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - 240;
        area.w = 200;
        int type = item->handler;
        int mode;
        WifiStatus *status;
        int level, timeout;
        if (type == SETTING_VOLUME || type == SETTING_BRIGHTNESS || type == SETTING_BGM_VOLUME || type == SETTING_HIBERNATE || type == SETTING_UNUSED_14 || type == SETTING_AUDIOFIX) {
            if (type == SETTING_VOLUME) {
                level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
                snprintf(text, 32, "%02d/%02d", level, 20);
            } else if (type == SETTING_BRIGHTNESS) {
                level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
                snprintf(text, 32, "%02d/%02d", level, 10);
            } else if (type == SETTING_BGM_VOLUME)
                snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME), 20);
            else if (type == SETTING_HIBERNATE) {
                timeout = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
                if (timeout < 5 || timeout > 30)
                    strncpy(text, languageString(STR_NEVER), 32);
                else
                    snprintf(text, 32, languageString(STR_N_MIN_LATER), timeout);
            } else if (type == SETTING_AUDIOFIX) {
                timeout = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
                if (timeout != 0)
                    strncpy(text, languageString(STR_ON), 32);
                else
                    strncpy(text, languageString(STR_OFF), 32);
            }
            area.y += 2;
            drawText(dst, g_imeFont, area, text, color, 1, false);
            if (settingLeftArrow) {
                area.y = (60 - settingLeftArrow->h) / 2 + y;
                area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - settingLeftArrow->w - 240;
                SDL_UpperBlit(settingLeftArrow, NULL, dst, &area);
            }
            if (rightArrow) {
                area.y = (60 - rightArrow->h) / 2 + y;
                area.x = rect.w + x - rightArrow->w - 40;
                SDL_UpperBlit(rightArrow, NULL, dst, &area);
            }
        } else if (type == SETTING_WIFI) {
            mode = wifiMode();
            if (mode == WIFI_MODE_STATION) {
                status = getWifiStatus();
                if (status->state == WIFI_STATE_COMPLETED) {
                    std::string label(status->ip);
                    if (label.size() != 0) {
                        area.x = rect.w + x - (rightArrow ? rightArrow->w : 0) - 240;
                        area.w = rightArrow ? rightArrow->w + 200 : 200;
                        area.y += 2;
                        drawText(dst, g_imeFont, area, label.c_str(), color, 4, false);
                    }
                }
            }
        }
        i++;
        row++;
    }
}

// 0x294f8: timer, 150 ms after the volume last changed: apply it and play the "enter" sound as a
// preview.
static unsigned previewVolume(unsigned interval, void *param)
{
    if (g_enterSound) {
        puts("Mix_PlayChannel enter.wav");
        if (Mix_PlayChannel(0, g_enterSound, 0) == -1) printf("Mix_PlayChunk: %s\n", SDL_GetError());
    }
    setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    return 0;
}

// 0x295a8: left on an adjustable setting: one step down (volume and music 0-20, brightness 1-10),
// or the next sleep timeout (0, 5, 15, 30 min) or the audio fix switch; marks settings changed.
void SettingMenu::decrease()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case SETTING_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_VOLUME, level);
            SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
            if (volumePreviewTimer) {
                SDL_RemoveTimer(volumePreviewTimer);
                Mix_HaltChannel(0);
            }
            volumePreviewTimer = SDL_AddTimer(150, previewVolume, NULL);
            break;
        case SETTING_BRIGHTNESS:
            level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
            if (level > 1) level--;
            SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, level);
            setBacklight(level);
            break;
        case SETTING_BGM_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, level);
            Mix_VolumeMusic((level << 7) / 20);
            Mix_Volume(1, (level << 7) / 20);
            break;
        case SETTING_HIBERNATE:
            level = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
            if (level == 0)
                level = 5;
            else if (level == 5)
                level = 15;
            else if (level == 15)
                level = 30;
            else
                level = 0;
            SetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY, level);
            break;
        case SETTING_AUDIOFIX:
            level = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
            if (level == 0) {
                level = 1;
                system("rm -f /tmp/no_audioserver");
            } else {
                level = 0;
                system("touch /tmp/no_audioserver");
            }
            SetKeyShm(g_keyShm, MONITOR_AUDIOFIX, level);
            break;
        default: break;
    }
}

// 0x298ec: right: one step up; otherwise as decrease.
void SettingMenu::increase()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int level;
    int selector = item->handler;
    switch (selector) {
        case SETTING_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_VOLUME);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_VOLUME, level);
            SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
            if (volumePreviewTimer) {
                SDL_RemoveTimer(volumePreviewTimer);
                Mix_HaltChannel(0);
            }
            volumePreviewTimer = SDL_AddTimer(150, previewVolume, NULL);
            break;
        case SETTING_BRIGHTNESS:
            level = GetKeyShm(g_keyShm, MONITOR_BRIGHTNESS);
            if (level < 10) level++;
            SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, level);
            setBacklight(level);
            break;
        case SETTING_BGM_VOLUME:
            level = GetKeyShm(g_keyShm, MONITOR_BGM_VOLUME);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, level);
            Mix_VolumeMusic((level << 7) / 20);
            Mix_Volume(1, (level << 7) / 20);
            break;
        case SETTING_HIBERNATE:
            level = GetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY);
            if (level == 0)
                level = 5;
            else if (level == 5)
                level = 15;
            else if (level == 15)
                level = 30;
            else
                level = 0;
            SetKeyShm(g_keyShm, MONITOR_HIBERNATE_DELAY, level);
            break;
        case SETTING_AUDIOFIX:
            level = GetKeyShm(g_keyShm, MONITOR_AUDIOFIX);
            if (level == 0) {
                level = 1;
                system("rm -f /tmp/no_audioserver");
            } else {
                level = 0;
                system("touch /tmp/no_audioserver");
            }
            SetKeyShm(g_keyShm, MONITOR_AUDIOFIX, level);
            break;
        default: break;
    }
}

// 0x29c30: reboot into the flashing mode (unused).
static void rebootToEfex()
{
    system("reboot efex");
}

// 0x29c4c: "Refresh Roms": delete the ROM caches and the emulator settings.
static int resetEmulatorSettings(void *arg)
{
    removeAllDBFiles();
    removeEmulatorConfigs();
    return 1;
}

// 0x29c74: factory reset: also deletes favorites, system.json and the saved list positions,
// restores the default volume, brightness and colors, clears Recent, the keymap and the language.
static int resetAllSettings(void *arg)
{
    resetEmulatorSettings(NULL);
    unlink("/mnt/SDCARD/Roms/favourite.json");
    unlink("/mnt/SDCARD/system.json");
    unlink("/appconfigs/romwinidx.json");
    g_romListPositions.clear();
    SetKeyShm(g_keyShm, MONITOR_VOLUME, 20);
    SetKeyShm(g_keyShm, MONITOR_BGM_VOLUME, 8);
    SetKeyShm(g_keyShm, MONITOR_MUTE, 0);
    SetKeyShm(g_keyShm, MONITOR_BRIGHTNESS, 7);
    SetKeyShm(g_keyShm, MONITOR_LUMINATION, 7);
    SetKeyShm(g_keyShm, MONITOR_HUE, 10);
    SetKeyShm(g_keyShm, MONITOR_SATURATION, 10);
    SetKeyShm(g_keyShm, MONITOR_CONTRAST, 10);
    setVolumeTinymix(GetKeyShm(g_keyShm, MONITOR_VOLUME));
    setLcdColor(7, 10, 10, 10);
    clearRecent(&g_recentGames);
    setKeymap(NULL);
    setLanguageFile(NULL);
    sync();
    initAudioSettings(g_keyShm);
    return 1;
}

// 0x29dc8: A on a row: opens what it stands for (device info, fixes, IO test, ADC info, language,
// key setting, color, themes, Wi-Fi, hotspot) or asks to confirm a reset or power-off.
void SettingMenu::select()
{
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return;
    g_settingsDirty = true;
    int type = item->handler;
    switch (type) {
        case SETTING_DEVICE_INFO: {
            MenuWindow *window = new MenuWindow(STR_DEVICE_INFO, WIN_DEVICE_INFO, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_FIXES: {
            MenuWindow *window = new MenuWindow(STR_FIXES, WIN_FIXES, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_IO_TEST: {
            KeyTestWindow2 *window = new KeyTestWindow2;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_ADC_INFO: {
            ADCInfoWindow *window = new ADCInfoWindow(STR_ADC_INFO, WIN_ADC_INFO);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_REFRESH_ROMS: {
            Action *action = new GeneralAction(-1, resetEmulatorSettings, NULL);
            Action *progress = new PopupProgressAction(STR_REFRESH, action);
            DialogWindow *window = new DialogWindow(STR_REFRESH_ROMS_2, STR_REFRESH_ROMS_TEXT, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_FACTORY_RESET: {
            Action *action = new GeneralAction(-1, resetAllSettings, NULL);
            Action *progress = new PopupProgressAction(STR_RESET, action);
            DialogWindow *window = new DialogWindow(STR_RESUME_DEFAULT_SETTING, STR_RESUME_SETTINGS_TEXT, false, progress);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_LANGUAGE: {
            MenuWindow *window = new MenuWindow(STR_LANGUAGE, WIN_LANGUAGE, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_KEYMAP: {
            KeySettingWindow *window = new KeySettingWindow;
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_COLOR: {
            MenuWindow *window = new MenuWindow(STR_COLOR, WIN_COLOR, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_THEME: {
            MenuWindow *window = new MenuWindow(STR_THEMES, WIN_THEME, (void *)NULL);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_POWER_OFF: {
            DialogWindow *window = new DialogWindow(STR_POWER_OFF, STR_SHUT_DOWN_TEXT, false, new GeneralAction(-1, powerOff, NULL));
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_WIFI: {
            WifiMenu *menu = new WifiMenu;
            MenuWindow *window = new MenuWindow(STR_WIFI, WIN_WIFI, (ListMenu *)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        case SETTING_HOTSPOT: {
            HotspotMenu *menu = new HotspotMenu;
            MenuWindow *window = new MenuWindow(STR_HOTSPOT, WIN_HOTSPOT, (ListMenu *)menu);
            window->onShow();
            g_windowStack->push_back((TrimusWindow *)window);
            return;
        }
        default: return;
    }
}

// 0x2a5d4: one button: up/down move, left/right adjust, A/Y select, B/X close, MENU passes through.
int SettingMenu::handleKey()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_UP:
            puts("key_up");
            moveUp();
            break;
        case BUTTON_DOWN:
            puts("key_down");
            moveDown();
            break;
        case BUTTON_LEFT:
            decrease();
            break;
        case BUTTON_RIGHT:
            increase();
            break;
        case BUTTON_A:
        case BUTTON_Y:
            puts("a");
            select();
            break;
        case BUTTON_B:
        case BUTTON_X:
            puts("b or c");
            return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        default: break;
    }
    return -1;
}

// 0x2a6ec: deletes every row.
void SettingMenu::freeListItem()
{
    puts("SettingMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        delete item;
    }
    items.clear();
}

struct DeviceInfoItem {
    DeviceInfoItem(const char *, const char *);
    ~DeviceInfoItem()
    {
    }
    std::string label, value;
};
// 0x2a7b0: one "label: value" row of the device information.
DeviceInfoItem::DeviceInfoItem(const char *labelText, const char *valueText)
{
    label = labelText;
    value = valueText;
}

// 0x2a83c: the device information list (WIN_DEVICE_INFO).
DeviceInfoMenu::DeviceInfoMenu() : TextMenu(true, true, 6, NULL)
{
    firmwareVersion = NULL;
    loadInfo();
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    themePath = g_themePath;
}

// 0x2a934
DeviceInfoMenu::~DeviceInfoMenu()
{
    freeListItem();
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
    if (firmwareVersion) free(firmwareVersion);
}

// 0x2aa08: after a theme change: reload the highlight.
void DeviceInfoMenu::reloadTheme()
{
    if (themePath.compare(g_themePath) != 0) {
        if (selectedBackground) SDL_FreeSurface(selectedBackground);
        selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}

// 0x2aae4: draws the visible rows: label on the left, value on the right.
void DeviceInfoMenu::drawList(SDL_Surface *dst)
{
    TTF_Font *font = g_imeFont;
    int height = TTF_FontHeight(font);
    SDL_Rect area;
    SDL_Color color;
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = 0;
    while (i < 5 && g_listStripe) {
        dest.x = 0;
        dest.y = (i + 2) * 60;
        SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        i++;
    }
    i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        DeviceInfoItem *item = (DeviceInfoItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (rowBackground) {
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        area.y = (60 - height) / 2 + y;
        area.w = rect.w - 40;
        area.h = 60;
        int width = drawText(dst, font, area, item->label.c_str(), color, 0, shadowSelected ? i == cur : false);
        area.x = area.x + width;
        area.w = rect.w - area.x - 20;
        drawText(dst, font, area, item->value.c_str(), color, 2, false);
        i++;
        row++;
    }
}

// 0x2b028: the chip's unique id (MI_SYS_ReadUuid) as 16 hex digits; left empty if it cannot be
// read.
static void formatDeviceUuid(char *out, size_t size)
{
    int rc = 0xa009201f;
    struct Words {
        unsigned char data[8];
    };
    Words words;
    memset(&words, 0, 8);
    union {
        unsigned long long value;
        Words parts;
    } uuid;
    uuid.value = 0;
    rc = MI_SYS_ReadUuid(&uuid.value);
    words = uuid.parts;
    if (rc == 0) snprintf(out, size, "%02X%02X%02X%02X%02X%02X%02X%02X", words.data[7], words.data[6], words.data[5], words.data[4], words.data[3], words.data[2], words.data[1], words.data[0]);
}

// 0x2b0f8: fills the rows: model MY354, serial (chip id), firmware version (miyoo_version from the
// U-Boot environment), CPU clock, memory, SD card use, website.
void DeviceInfoMenu::loadInfo()
{
    char uuid[32] = {
        0};
    char buf[16];
    formatDeviceUuid(uuid, 32);
    char *version = NULL;
    if (firmwareVersion) {
        free(firmwareVersion);
        firmwareVersion = NULL;
    }
    system("/etc/fw_printenv miyoo_version > /tmp/miyoo_version");
    FILE *f = fopen("/tmp/miyoo_version", "r");
    if (f) {
        puts("loadContent");
        readWholeFile("/tmp/miyoo_version", f, &firmwareVersion);
        puts("loadContent end");
        fclose(f);
        version = strstr(firmwareVersion, "\n");
        if (version) *version = 0;
        version = strstr(firmwareVersion, "miyoo_version=");
        if (version) version += 14;
    }
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_MODE), "MY354"));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_SERIAL), uuid));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_VERSION), version ? version : "202110071805"));
    snprintf(buf, 16, "%dMHz", (int)maxCpuMHz());
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_CPU), buf));
    snprintf(buf, 16, "%dM", memoryMB());
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_MEMORY), buf));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_SD_CARD), sdCardSpace().c_str()));
    addItem((TextItem *)new DeviceInfoItem(languageString(STR_WEBSITE), "www.lomiyoo.com"));
}

// 0x2b504: deletes every row.
void DeviceInfoMenu::freeListItem()
{
    for (int i = 0; i < items.size(); i++) {
        DeviceInfoItem *item = (DeviceInfoItem *)items[i];
        delete item;
    }
    items.clear();
}

struct PanelInfo {
    unsigned char raw[84];
    unsigned short timing[16];
    unsigned short minDclk, spreadStep, spreadSpan;
    unsigned char pwmPeriodLow, pwmPeriodHigh, pwmDuty, deinterlace;
    int aspect;
    unsigned short lvdsSwap;
    int tiBitMode, outputFormat;
    unsigned char swapOddRg, swapEvenRg, swapOddGb, swapEvenGb, doubleClock;
    unsigned maxSet, minSet;
    int outputTiming;
    unsigned char noiseDither;
    int channels[5];
};
static_assert(sizeof(PanelInfo) == 188, "MI_PANEL_ParamConfig_t storage");
// SDK-compatible full object; timing[] retains the prefix used by adjustColor.
PanelInfo g_panelInfo = {
    {
        75,
        49,
        48,
        49,
        95,
        73,
        77,
        50,
        66,
        86,
        76,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        11,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        1,
        1,
        0,
        30,
        0,
        144,
        1,
        80,
        0,
        30,
        0,
    },
    {20, 22, 5, 13, 40, 18, 640, 480, 0, 780, 0, 0, 515, 0, 0, 26},
    0,
    0,
    0,
    160,
    255,
    80,
    0,
    0,
    0,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    1868942,
    1633113,
    1,
    0,
    {2, 4, 3, 1, 0}}; // 0x17e48c

// 0x17e548: MIPI command stream, including delays and the terminating marker.
unsigned char g_panelCommands[] = {
    0x01,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x50, // delay (ms)
    0xb9,
    0x03,
    0xf1,
    0x12,
    0x83, // command, length, payload
    0xb1,
    0x05,
    0x00,
    0x00,
    0x00,
    0xda,
    0x80, // command, length, payload
    0xb2,
    0x03,
    0x00,
    0x13,
    0x70, // command, length, payload
    0xb3,
    0x0a,
    0x10,
    0x10,
    0x28,
    0x28,
    0x03,
    0xff,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xb4,
    0x01,
    0x80, // command, length, payload
    0xb5,
    0x02,
    0x0a,
    0x0a, // command, length, payload
    0xb6,
    0x02,
    0x87,
    0x87, // command, length, payload
    0xb8,
    0x04,
    0x26,
    0x22,
    0xf0,
    0x63, // command, length, payload
    0xba,
    0x1b,
    0x31,
    0x81,
    0x05,
    0xf9,
    0x0e,
    0x0e,
    0x20,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x44,
    0x25,
    0x00,
    0x91,
    0x0a,
    0x00,
    0x00,
    0x02,
    0x4f,
    0xd1,
    0x00,
    0x00,
    0x37, // command, length, payload
    0xbc,
    0x01,
    0x47, // command, length, payload
    0xbf,
    0x03,
    0x02,
    0x11,
    0x00, // command, length, payload
    0xc0,
    0x09,
    0x73,
    0x73,
    0x50,
    0x50,
    0x00,
    0x00,
    0x12,
    0x50,
    0x00, // command, length, payload
    0xc1,
    0x0c,
    0x53,
    0x00,
    0x32,
    0x32,
    0x77,
    0xd1,
    0xcc,
    0xcc,
    0x77,
    0x77,
    0x33,
    0x33, // command, length, payload
    0xc6,
    0x06,
    0x82,
    0x00,
    0xbf,
    0xff,
    0x00,
    0xff, // command, length, payload
    0xc7,
    0x06,
    0xb8,
    0x00,
    0x0a,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xc8,
    0x04,
    0x10,
    0x40,
    0x1e,
    0x02, // command, length, payload
    0xcc,
    0x01,
    0x0b, // command, length, payload
    0x36,
    0x01,
    0xc0, // command, length, payload
    0xe0,
    0x22,
    0x00,
    0x06,
    0x0a,
    0x2a,
    0x3d,
    0x3f,
    0x3b,
    0x37,
    0x06,
    0x0b,
    0x0c,
    0x10,
    0x11,
    0x10,
    0x13,
    0x12,
    0x18,
    0x00,
    0x06,
    0x0a,
    0x2a,
    0x3d,
    0x3f,
    0x3b,
    0x37,
    0x06,
    0x0b,
    0x0c,
    0x10,
    0x11,
    0x10,
    0x13,
    0x12,
    0x18, // command, length, payload
    0xe3,
    0x0e,
    0x07,
    0x07,
    0x0b,
    0x0b,
    0x0b,
    0x0b,
    0x00,
    0x00,
    0x00,
    0x00,
    0xff,
    0x00,
    0xc0,
    0x10, // command, length, payload
    0xe9,
    0x3f,
    0xc8,
    0x10,
    0x02,
    0x00,
    0x00,
    0xb0,
    0xb1,
    0x11,
    0x31,
    0x23,
    0x28,
    0x80,
    0xb0,
    0xb1,
    0x27,
    0x08,
    0x00,
    0x04,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x02,
    0x00,
    0x00,
    0x00,
    0x88,
    0x88,
    0xba,
    0x60,
    0x24,
    0x08,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0xba,
    0x71,
    0x35,
    0x18,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xea,
    0x3d,
    0x97,
    0x0a,
    0x82,
    0x02,
    0x03,
    0x07,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x81,
    0x88,
    0xba,
    0x17,
    0x53,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x80,
    0x88,
    0xba,
    0x06,
    0x42,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x88,
    0x23,
    0x00,
    0x00,
    0x01,
    0xa4,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // command, length, payload
    0xef,
    0x03,
    0xff,
    0xff,
    0x01, // command, length, payload
    0x11,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x96, // delay (ms)
    0x29,
    0x01,
    0x00, // command, length, payload
    0xfe,
    0xfe,
    0x14, // delay (ms)
    0xff,
    0xff, // end
};
static_assert(sizeof(g_panelCommands) == 341, "panel command stream");
struct PanelMipiConfig {
    unsigned char hsTrail, hsPrepare, hsZero, clkHsPrepare, clkHsExit, clkTrail;
    unsigned char clkZero, clkHsPost, dataHsExit, contDet, lpx, taGet, taSure, taGo;
    unsigned short hActive, hPulse, hBackPorch, hFrontPorch;
    unsigned short vActive, vPulse, vBackPorch, vFrontPorch, bllp, fps;
    int laneMode, format, controlMode;
    unsigned char *commands;
    unsigned commandSize;
    unsigned char syncCalibrate;
    unsigned short virtualHsyncStart, virtualHsyncEnd, vsyncRef, dataClockSkew;
    unsigned char polarity[5];
    int packetType;
};
static_assert(sizeof(PanelMipiConfig) == 76, "MI_PANEL_MipiDsiConfig_t storage");
PanelMipiConfig g_panelMipiConfig = { // 0x17e6a0
                                     3,
                                     2,
                                     5,
                                     2,
                                     4,
                                     4,
                                     10,
                                     10,
                                     4,
                                     0,
                                     16,
                                     26,
                                     22,
                                     50,
                                     640,
                                     20,
                                     22,
                                     120,
                                     480,
                                     5,
                                     13,
                                     17,
                                     0,
                                     60,
                                     2,
                                     3,
                                     1,
                                     g_panelCommands,
                                     sizeof(g_panelCommands),
                                     1,
                                     431,
                                     441,
                                     32978,
                                     9,
                                     {0, 0, 0, 0, 0},
                                     0};
bool g_customDisplayTiming = true; // 0x17e6ec
struct DisplayAttr {
    unsigned background;
    int interface, timing;
    bool userSync;
    char pad[3];
    unsigned short sync[14];
    unsigned frameRate;
};
extern "C" int MI_DISP_GetPubAttr(int, DisplayAttr *);
extern "C" int MI_DISP_SetPubAttr(int, DisplayAttr *);
// 0x2b5bc: programs the display controller with the panel timing from g_panelInfo (sync widths,
// porches, frame rate).
void applyPanelTiming()
{
    DisplayAttr attr;
    char reserved[16]; // Retain the original unused stack storage.
    memset(&attr, 0, 48);
    MI_DISP_GetPubAttr(0, &attr);
    attr.userSync = true;
    attr.sync[0] = g_panelInfo.timing[7];
    attr.sync[1] = g_panelInfo.timing[3];
    attr.sync[2] = g_panelInfo.timing[12] - (g_panelInfo.timing[2] + g_panelInfo.timing[7] + g_panelInfo.timing[3]);
    attr.sync[3] = g_panelInfo.timing[6];
    attr.sync[4] = g_panelInfo.timing[1];
    attr.sync[5] = g_panelInfo.timing[9] - (g_panelInfo.timing[0] + g_panelInfo.timing[6] + g_panelInfo.timing[1]);
    attr.sync[7] = 0;
    attr.sync[8] = 0;
    attr.sync[9] = 0;
    attr.sync[10] = g_panelInfo.timing[0];
    attr.sync[11] = g_panelInfo.timing[2];
    attr.frameRate = g_panelInfo.timing[15] * 1000000 / (g_panelInfo.timing[9] * g_panelInfo.timing[12]);
    attr.timing = 32;
    attr.interface = 6;
    attr.background = 0xff000000;
    MI_DISP_SetPubAttr(0, &attr);
}
// =================================================================================================
// 0x2b788-0x2e424: Color, language, theme and file-browser menus.
// =================================================================================================
extern bool g_customDisplayTiming; // 0x17e6ec
extern Theme g_skin; // 0x181974
Theme *findTheme(std::vector<Theme> *, const char *);
unsigned sdCardFreeMB();
extern "C" {
struct ZipFile;
struct ZipEntry;
ZipFile *zipOpen(const char *); // 0x14d8b0
ZipEntry *zipOpenEntry(ZipFile *, const char *, unsigned); // 0x14d418
void zipCloseEntry(ZipEntry *); // 0x14d858
void zipClose(ZipFile *); // 0x14da1c
}

// 0x2b788: the display color menu (WIN_COLOR): lightness, hue, saturation and contrast, each 0-20.
ColorMenu::ColorMenu() : TextMenu(false, true, 6, NULL)
{
    loadItems();
    leftArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-left-arrow-24.png", g_backBuffer, false);
    colorRightArrow = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-right-arrow-24.png", g_backBuffer, false);
    selectedBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
}

// 0x2b8bc (D0 0x2b96c)
ColorMenu::~ColorMenu()
{
    freeListItem();
    if (leftArrow) SDL_FreeSurface(leftArrow);
    if (colorRightArrow) SDL_FreeSurface(colorRightArrow);
    if (selectedBackground) {
        SDL_FreeSurface(selectedBackground);
        selectedBackground = NULL;
    }
}

// 0x2b9a0: draws the four rows with their values between left/right arrows.
void ColorMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    char text[32] = {
        0};
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= items.size() - 1) pageEnd = items.size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (row < 5 && g_listStripe) {
            dest.x = 0;
            dest.y = (row + 2) * 60;
            SDL_UpperBlit(g_listStripe, NULL, dst, &dest);
        }
        if (rowBackground) {
            dest.x = x;
            dest.y = (60 - rowBackground->h) / 2 + y;
            area.x = 0;
            area.y = 0;
            area.w = rect.w;
            area.h = rect.h;
            SDL_UpperBlit(rowBackground, &area, dst, &dest);
        }
        if (i == cur) {
            if (selectedBackground) {
                dest.x = x;
                dest.y = (60 - selectedBackground->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectedBackground, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        if (item->iconSurface) {
            area.y = (60 - item->iconSurface->h) / 2 + y;
            SDL_UpperBlit(item->iconSurface, NULL, dst, &area);
            area.x = area.x + item->iconSurface->w + 5;
        }
        area.y = y;
        area.w = rect.w - (leftArrow ? leftArrow->w : 0) - (colorRightArrow ? colorRightArrow->w : 0) - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, languageString(item->labelId), color, 3, shadowSelected ? i == cur : false);
        area.x = rect.w + x - (colorRightArrow ? colorRightArrow->w : 0) - 240;
        area.w = 200;
        int type = item->handler;
        int level;
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            snprintf(text, 32, "%02d/%02d", level, 20);
        } else if (type == 3)
            snprintf(text, 32, "%02d/%02d", GetKeyShm(g_keyShm, MONITOR_SATURATION), 20);
        else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            snprintf(text, 32, "%02d/%02d", level, 20);
        }
        area.y += 2;
        drawText(dst, g_imeFont, area, text, color, 1, false);
        if (leftArrow) {
            area.y = (60 - leftArrow->h) / 2 + y;
            area.x = rect.w + x - (colorRightArrow ? colorRightArrow->w : 0) - leftArrow->w - 240;
            SDL_UpperBlit(leftArrow, NULL, dst, &area);
        }
        if (colorRightArrow) {
            area.y = (60 - colorRightArrow->h) / 2 + y;
            area.x = rect.w + x - colorRightArrow->w - 40;
            SDL_UpperBlit(colorRightArrow, NULL, dst, &area);
        }
        i++;
        row++;
    }
}

// 0x2c274: left/right change the selected value and apply all four to the panel (the first change
// also reprograms the panel timing); closing shows a "reboot to apply changes" note.
int ColorMenu::handleKey()
{
    int level;
    int key = ListMenu::handleKey();
    SettingMenuItem *item = (SettingMenuItem *)items[cur];
    if (item == NULL) return 0;
    int type = item->handler;
    if (key == LIST_LEFT) {
        if (g_customDisplayTiming) {
            applyPanelTiming();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_LUMINATION, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_HUE, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, MONITOR_SATURATION);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_SATURATION, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            if (level > 0) level--;
            SetKeyShm(g_keyShm, MONITOR_CONTRAST, level);
        }
        setLcdColor(GetKeyShm(g_keyShm, MONITOR_LUMINATION), GetKeyShm(g_keyShm, MONITOR_HUE), GetKeyShm(g_keyShm, MONITOR_SATURATION), GetKeyShm(g_keyShm, MONITOR_CONTRAST));
        return 0;
    } else if (key == LIST_RIGHT) {
        if (g_customDisplayTiming) {
            applyPanelTiming();
            g_customDisplayTiming = false;
        }
        if (type == 1) {
            level = GetKeyShm(g_keyShm, MONITOR_LUMINATION);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_LUMINATION, level);
        } else if (type == 2) {
            level = GetKeyShm(g_keyShm, MONITOR_HUE);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_HUE, level);
        } else if (type == 3) {
            level = GetKeyShm(g_keyShm, MONITOR_SATURATION);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_SATURATION, level);
        } else if (type == 4) {
            level = GetKeyShm(g_keyShm, MONITOR_CONTRAST);
            if (level < 20) level++;
            SetKeyShm(g_keyShm, MONITOR_CONTRAST, level);
        }
        setLcdColor(GetKeyShm(g_keyShm, MONITOR_LUMINATION), GetKeyShm(g_keyShm, MONITOR_HUE), GetKeyShm(g_keyShm, MONITOR_SATURATION), GetKeyShm(g_keyShm, MONITOR_CONTRAST));
        return 0;
    } else if (key == LIST_CLOSE) {
        NotifyWindow *window = new NotifyWindow(languageString(STR_REBOOT_TO_APPLY_CHANGES), languageString(STR_OK), "skin/bg-button-f.png", NULL);
        pushWindow(window);
        return key;
    }
    return key;
}

// 0x2c73c: A does nothing here.
int ColorMenu::onSelect()
{
    return 0;
}

// 0x2c760: deletes every row.
void ColorMenu::freeListItem()
{
    puts("ColorMenu::freeListItem");
    for (int i = 0; i < items.size(); i++) {
        SettingMenuItem *item = (SettingMenuItem *)items[i];
        delete item;
    }
    items.clear();
}

// 0x2c824: the four rows (their handler numbers 1-4 are local to this menu).
void ColorMenu::loadItems()
{
    addItem((TextItem *)new SettingMenuItem(STR_LIGHTNESS, NULL, 1));
    addItem((TextItem *)new SettingMenuItem(STR_HUE, NULL, 2));
    addItem((TextItem *)new SettingMenuItem(STR_SATURATION, NULL, 3));
    addItem((TextItem *)new SettingMenuItem(STR_CONTRAST, NULL, 4));
}

// 0x2c950: one row per language offered (languageChoices).
LanguageMenu::LanguageMenu(std::vector<LanguageChoice> choices) : TextMenu(false, true, 6, NULL)
{
    languages = choices;
    for (int i = 0; i < languages.size(); i++) addItem(new TextItem(languages[i].name.c_str(), NULL, NULL, true));
}

// 0x2caa8 (D0 0x2caf4)
LanguageMenu::~LanguageMenu()
{
}

// 0x2cb28: switch to the selected language and reopen the theme's fonts for it.
int LanguageMenu::onSelect()
{
    if ((unsigned)cur < languages.size()) {
        setLanguageFile(languages[cur].file.c_str());
        Theme *theme = findTheme(&g_themeList, g_themePath);
        if (theme)
            changeLanguageFonts(theme, &g_fontSize);
        else
            changeLanguageFonts(&g_skin, &g_fontSize);
        return 1;
    }
    return -1;
}

// 0x2cc14: the built-in theme ("Default") and one row per installed theme.
ThemeMenu::ThemeMenu() : TextMenu(false, true, 6, NULL)
{
    addItem(new TextItem(languageString(STR_DEFAULT), NULL, NULL, true));
    for (int i = 0; i < g_themeList.size(); i++) addItem(new TextItem(g_themeList[i].name.c_str(), NULL, NULL, true));
}

// 0x2cd80 (D0 0x2cdbc)
ThemeMenu::~ThemeMenu()
{
}

// 0x2cdf0: switch to the selected theme: reload fonts, images and sounds and restart the music.
int ThemeMenu::onSelect()
{
    if ((unsigned)cur < g_themeList.size() + 1) {
        if (g_themePath) free(g_themePath);
        int unused = 0; // Retained original stack local.
        if (cur > 0) {
            g_themePath = strdup(g_themeList[cur - 1].root.c_str());
            changeLanguageFonts(&g_themeList[cur - 1], &g_fontSize);
            freeTheme();
            loadTheme(&g_themeList[cur - 1], &g_fontSize);
            Mix_PlayMusic(g_themeMusic, -1);
        } else {
            g_themePath = strdup(g_skinSubdir);
            changeLanguageFonts(&g_skin, &g_fontSize);
            freeTheme();
            loadTheme(&g_skin, &g_fontSize);
            Mix_PlayMusic(g_themeMusic, -1);
        }
        return 1;
    }
    return -1;
}

// Legacy suffix table retained in the target even though fileType reads configured extensions.
// Original writable pointer table: 0x17e6f0-0x17e764.
const char *g_legacyExtensions[] = {
    ".smc", ".fig", ".sfc", ".gd3", ".gd7", ".dx2", ".bsx", ".swc",
    ".gen", ".md", ".smd", ".32x", ".sms", ".rom", ".gg", ".sg",
    ".nes", ".fds", ".unf", ".unif", ".gba", ".agb", ".gbz", ".pbp",
    ".iso", ".cue", ".gb", ".gbc", ".pce"};

// 0x2cfc0: index of the emulator (Emu folder) whose extension list contains this file's extension,
// or -1.
int findEmuForFile(const char *name)
{
    const char *suffix = strrchr(name, '.');
    if (suffix == NULL) return -1;
    {
        for (int i = 0; i < g_emuFolders.size(); i++) {
            for (int j = 0; j < g_emuFolders[i].extensions.size(); j++) {
                if (strncasecmp(suffix + 1, g_emuFolders[i].extensions[j].c_str(), g_emuFolders[i].extensions[j].size()) == 0) return i;
            }
        }
    }
    return -1;
}

// 0x2d158: confirmed system update: write the zip's path to /tmp/.try_upgrade_file and quit MainUI
// (result 3), so the shell script can run the update.
static int requestUpgrade(void *arg)
{
    const char *path = (const char *)arg;
    FILE *file = fopen("/tmp/.try_upgrade_file", "w+");
    if (file) {
        fputs(path, file);
        fclose(file);
    }
    return 3;
}

// 0x2d1bc: A on a file-browser row: a TrimuiUpdate*.zip asks to update the system (needs 500 MB
// free), another file starts in the emulator that handles its extension; a folder posts
// EVENT_OPEN_DIRECTORY.
int FileAction::execute(void *param)
{
    TextItem *item = (TextItem *)param;
    if (kind == FILE_KIND_FILE) {
        const char *suffix = strrchr(path.c_str(), '.');
        if (suffix == NULL) {
            printf("openfile_action no suffix %s\n", path.c_str());
            return -1;
        }
        if (strncasecmp(item->label.c_str(), "TrimuiUpdate", 12) == 0 && strncasecmp(suffix, ".zip", 4) == 0) {
            int freeMB = sdCardFreeMB();
            if (freeMB >= 500) {
                strcpy(g_launchCmd, path.c_str());
                DialogWindow *window = new DialogWindow(STR_UPGRADE, STR_UPDATE_SYSTEM, false, new GeneralAction(-1, requestUpgrade, g_launchCmd));
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            } else {
                snprintf(g_launchCmd, 0x800, languageString(STR_SD_SPACE_LOW), freeMB);
                NotifyWindow *window = new NotifyWindow(g_launchCmd, languageString(STR_OK), "skin/bg-button-f.png", NULL);
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            }
        } else {
            int type = findEmuForFile(path.c_str());
            if (type >= 0 && (unsigned)type < g_emuFolders.size()) return launchRom(item->label.c_str(), &g_emuFolders[type], path.c_str(), NULL, WIN_ROM_LIST);
        }
    } else if (kind == FILE_KIND_DIRECTORY) {
        SDL_UserEvent event;
        event.type = SDL_USEREVENT;
        event.code = EVENT_OPEN_DIRECTORY;
        event.data1 = (void *)path.c_str();
        if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    }
    return 0;
}

// 0x2d574: which files the browser lists: ROMs of a known extension, update zips that contain an
// "updater" entry, and license files.
static bool fileBrowserFilter(const char *name, const char *path)
{
    if (findEmuForFile(name) >= 0) return true;
    const char *suffix = strrchr(name, '.');
    if (strncasecmp(name, "TrimuiUpdate", 12) == 0 && suffix && strncasecmp(suffix, ".zip", 4) == 0) {
        ZipFile *zip = zipOpen(path);
        if (zip) {
            ZipEntry *entry;
            if ((entry = zipOpenEntry(zip, "updater", 0)) == NULL) {
                zipClose(zip);
                printf("Can't found %s in %s\n", "updater", name);
                return false;
            }
            zipCloseEntry(entry);
            zipClose(zip);
            return true;
        }
    }
    if (strncmp(name, "COPYING", 7) == 0) return true;
    if (strncasecmp(name, "LICENSE", 7) == 0) return true;
    return false;
}

// 0x2d700: the file browser (WIN_FILES), starting at path.
FileBrowserMenu::FileBrowserMenu(const char *path) : ListMenu(false), cache(fileBrowserFilter)
{
    fileIcon = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false);
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-game.png", g_backBuffer, false));
    icons.push_back(loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-folder.png", g_backBuffer, false));
    selectionBar = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
    dirItems = cache.open(path);
    highlightSelected = true;
}

// 0x2d908 (D0 0x2da58)
FileBrowserMenu::~FileBrowserMenu()
{
    // Original iterates an empty local vector, leaving the icon entries unfreed.
    std::vector<SDL_Surface *> surfaces;
    for (int i = 0; i < surfaces.size(); i++)
        if (surfaces[i]) SDL_FreeSurface(surfaces[i]);
    if (fileIcon) SDL_FreeSurface(fileIcon);
    if (selectionBar) SDL_FreeSurface(selectionBar);
}

// 0x2da8c: rows in the current folder.
int FileBrowserMenu::count()
{
    if (dirItems) return dirItems->size();
    return 0;
}

// 0x2dad4: A on a row: run its FileAction.
int FileBrowserMenu::onSelect()
{
    TextItem *item = (TextItem *)getItem(cur);
    if (item && item->action) {
        int result;
        if ((result = item->action->execute(item)) >= 0) return result;
    }
    return -1;
}

// 0x2db8c: the row at index.
void *FileBrowserMenu::getItem(int index)
{
    if (index >= 0 && (unsigned)index < dirItems->size()) return (*dirItems)[index];
    return NULL;
}

// 0x2dc14: the icon for a row: file or folder.
SDL_Surface *FileBrowserMenu::itemIcon(TextItem *item)
{
    FileAction *action = (FileAction *)item->action;
    int index = action->kind - FILE_KIND_FILE;
    if (index >= 0 && (unsigned)index < icons.size()) return icons[index];
    return fileIcon;
}

// 0x2dcbc: draws the visible rows: highlight, icon and name.
void FileBrowserMenu::drawList(SDL_Surface *dst)
{
    int height = TTF_FontHeight(g_imeFont);
    SDL_Rect area;
    SDL_Color color;
    SDL_Rect dest;
    if (pageStart == 0 && pageEnd == 0) pageEnd = 5;
    if (cur < pageStart) {
        pageStart = cur;
        pageEnd = pageStart + 5;
    }
    if (cur > pageEnd) {
        pageEnd = cur;
        pageStart = pageEnd - 5;
    }
    if (pageStart < 0) pageStart = 0;
    if ((unsigned)pageEnd >= dirItems->size() - 1) pageEnd = dirItems->size() - 1;
    int i = pageStart;
    int row = 0;
    for (; i <= pageEnd;) {
        TextItem *item = (TextItem *)getItem(i);
        color = g_imeTextColor;
        int x = rect.x;
        int y = rect.y + 2 + row * 60;
        if (i == cur) {
            if (selectionBar) {
                dest.x = x;
                dest.y = (60 - selectionBar->h) / 2 + y;
                area.x = 0;
                area.y = 0;
                area.w = rect.w;
                area.h = rect.h;
                SDL_UpperBlit(selectionBar, &area, dst, &dest);
            } else
                color = g_wifiSelectedColor;
        }
        area.x = x + 20;
        SDL_Surface *icon = itemIcon(item);
        if (icon) {
            area.y = (60 - icon->h) / 2 + y;
            SDL_UpperBlit(icon, NULL, dst, &area);
            area.x = area.x + icon->w + 5;
        }
        area.y = (60 - height) / 2 + y;
        area.w = rect.w - 20;
        area.h = 60;
        drawText(dst, g_imeFont, area, item->label.c_str(), color, 0, highlightSelected ? i == cur : false);
        i++;
        row++;
    }
}

// 0x2e0f0: one button: up/down move, A/Y open, B/X close, MENU passes through; EVENT_OPEN_DIRECTORY
// enters the folder (or goes up for "..").
int FileBrowserMenu::handleKey()
{
    const char *path;
    SDL_Event event;
    int key = handleGlobalInput(&event);
    switch (key) {
        case BUTTON_UP:
            moveUp();
            break;
        case BUTTON_DOWN:
            moveDown();
            break;
        case BUTTON_A:
        case BUTTON_Y:
            puts("a");
            return onSelect();
        case BUTTON_B:
        case BUTTON_X:
            puts("b or c");
            return 1;
        case BUTTON_MENU: return BUTTON_MENU;
        case EVENT_OPEN_DIRECTORY:
            path = (const char *)event.user.data1;
            printf("change dir %s\n", path);
            if (strncmp(path + strlen(path) - 2, "..", 2) == 0 || strncmp(path + strlen(path) - 3, "../", 3) == 0)
                dirItems = cache.parent();
            else
                dirItems = cache.child(path);
            restorePosition(0, 0, 5);
            break;
        default: break;
    }
    return -1;
}

// 0x2e318: free callback for caches of surfaces.
void freeSurfaceCallback(SDL_Surface *surface)
{
    SDL_Surface *value = surface;
    SDL_FreeSurface(value);
}

// 0x2e344: background job: load a ROM thumbnail scaled to fit 250x360 into the preview cache.
void LoadSurfaceWorker::run()
{
    SDL_Surface *surface = loadThumbnail(path.c_str(), 250, 360);
    if (surface) cache->put(path, surface);
}

// 0x2e3ec: the image is in: ask for a redraw.
void LoadSurfaceWorker::finished()
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code = EVENT_REDRAW;
    SDL_PushEvent(&event);
}

// Inline definitions follow the record destructors first used by the code from 0x1a44c to 0x2b788.
inline LoadSurfaceWorker::LoadSurfaceWorker(std::string path, LruCache *cache)
{
    this->cache = cache;
    this->path = path;
}
inline LoadSurfaceWorker::~LoadSurfaceWorker() {}
// =================================================================================================
// 0x2e424-0x3225c: MenuWindow (the window around every list) and FloatWindow.
// =================================================================================================
// 0x2e424: a window that shows a list (built in onShow by createMenu, or the ROM list for an
// emulator in entry) under a title from the language strings.
MenuWindow::MenuWindow(int titleLabelId, int id, void *entry)
    : TrimusWindow(titleLabelId, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = NULL;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = entry;
    showFullPath = false;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2e6d4: the same with a list that is already built (Wi-Fi, hotspot, the root menu).
MenuWindow::MenuWindow(int titleLabelId, int id, ListMenu *menu)
    : TrimusWindow(titleLabelId, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = menu;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = 0;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2e978: the same with a literal title (a folder name).
MenuWindow::MenuWindow(const char *folder, int id, void *entry)
    : TrimusWindow(-1, id), thumbnailLoader(2), thumbnailCache(freeSurfaceCallback)
{
    this->menu = NULL;
    menuChanged = false;
    hasPreview = false;
    previewImage = 0;
    savedCurPos = 0;
    savedPageStart = 0;
    savedPageEnd = 0;
    menuChanged = false;
    hasPreview = false;
    rightOpensDetail = true;
    cachedScreen = 0;
    emuEntry = entry;
    showFullPath = false;
    titleText = folder;
    iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, 0);
    themePath = g_themePath;
    headerIcon = 0;
    ownsHeaderIcon = false;
}
// 0x2ec3c (D0 0x2ed78)
MenuWindow::~MenuWindow()
{
    if (cachedScreen != 0) {
        SDL_FreeSurface(cachedScreen);
    }
    if (iconTF != 0) {
        SDL_FreeSurface(iconTF);
    }
    if (ownsHeaderIcon && headerIcon != 0) {
        SDL_FreeSurface(headerIcon);
    }
}

int findEmuByLabelPrefix(std::vector<EmuInfo> *, const char *);
// 0x121074
extern SDL_Surface *g_previewBg;
// 0x1821fc
extern SDL_Surface *g_emptyImage;
// 0x1821b4
extern SDL_Color g_listColor;
// 0x1822ac
extern SDL_Color g_hintColor;
// 0x1822a8
extern char g_countBuffer[0x100];
// 0x180754
// 0x2edac: what is saved in /tmp/state.json for this window: position, emulator or RetroArch-app
// name, folder and search string.
void MenuWindow::saveState(cJSON *obj)
{
    cJSON *position = cJSON_CreateNumber(menu->getCur());
    cJSON_AddItemToObject(obj, "currpos", position);
    cJSON *start = cJSON_CreateNumber(menu->getPageStart());
    cJSON_AddItemToObject(obj, "pagestart", start);
    cJSON *end = cJSON_CreateNumber(menu->getPageEnd());
    cJSON_AddItemToObject(obj, "pageend", end);
    if (emuEntry) {
        EmuInfo *emu = (EmuInfo *)emuEntry;
        cJSON *name = cJSON_CreateString(emu->label.c_str());
        if (id == WIN_ROM_LIST)
            cJSON_AddItemToObject(obj, "emuname", name);
        else
            cJSON_AddItemToObject(obj, "raname", name);
    }
    if (parentPath.length() != 0) cJSON_AddStringToObject(obj, "ppath", parentPath.c_str());
    std::string search = menu->getSearch();
    if (search.length() != 0) cJSON_AddStringToObject(obj, "searchstr", search.c_str());
}
// 0x2efcc: reads that back (onShow then rebuilds the list at the saved position).
int MenuWindow::restoreState(cJSON *obj)
{
    cJSONItem *position = (cJSONItem *)cJSON_GetObjectItem(obj, "currpos");
    cJSONItem *start = (cJSONItem *)cJSON_GetObjectItem(obj, "pagestart");
    cJSONItem *end = (cJSONItem *)cJSON_GetObjectItem(obj, "pageend");
    if (position && position->type == 8 && start && start->type == 8 && end && end->type == 8) {
        savedCurPos = position->valueint;
        savedPageStart = start->valueint;
        savedPageEnd = end->valueint;
    }
    cJSON *name = cJSON_GetObjectItem(obj, "emuname");
    if (name) {
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_emuFolders, label);
        if (index >= 0) {
            emuEntry = &g_emuFolders[index];
            headerText = g_emuFolders[index].label;
        } else
            emuEntry = NULL;
    } else {
        name = cJSON_GetObjectItem(obj, "raname");
        const char *label = cJSON_GetStringValue(name);
        int index = findEmuByLabelPrefix(&g_rappFolders, label);
        if (index >= 0)
            emuEntry = &g_rappFolders[index];
        else
            emuEntry = NULL;
    }
    cJSONItem *folder = (cJSONItem *)cJSON_GetObjectItem(obj, "ppath");
    if (folder && folder->type == 16) parentPath = folder->valuestring;
    cJSONItem *search = (cJSONItem *)cJSON_GetObjectItem(obj, "searchstr");
    if (search && search->type == 16) path = search->valuestring;
    return 0;
}
// 0x2f290: builds the list when first shown: the global search, a ROM list (at the position last
// used in that folder) or createMenu(id).
void MenuWindow::onShow()
{
    if (menu == NULL) {
        if (id == WIN_SEARCH) {
            menu = new MultiDBCachedTextMenu(path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else if (id != WIN_ROM_LIST && id != WIN_RETROARCH_LIST) {
            menu = createMenu(id, path);
            if (menu) menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
        } else {
            if (emuEntry) {
                EmuInfo *emu = (EmuInfo *)emuEntry;
                RomListPosition position;
                menu = createRomListMenu(emu, id, &parentPath, &path);
                std::string folder = emu->path;
                if (parentPath.length() != 0) folder = emu->path + "/" + parentPath;
                if (getRomListPosition(&g_romListPositions, folder, &position) >= 0)
                    menu->restorePosition(position.current, position.start, position.selected);
                else
                    menu->restorePosition(savedCurPos, savedPageStart, savedPageEnd);
            }
        }
        hasPreview = isListMenuId(id) && id != WIN_FILES && id != WIN_APPS;
        if (menu) isListMenuId(id);
    }
}
// 0x2f6c4: keeps the preview image for the selected row: an app's icon, or the ROM's image from the
// database or the Imgs folder; loads it in the background if it is not cached.
void MenuWindow::updateThumbnail()
{
    if (hasPreview) {
        TextItem *item = (TextItem *)menu->getItem(menu->getCur());
        if (item) {
            std::string unused;
            std::string image;
            if (item->action->result == WIN_APPS) {
                AppAction *action = (AppAction *)item->action;
                AppInfo *app = action->app;
                image = app->iconSelected;
            } else {
                const char *explicitPath = (const char *)menu->getItemData(menu->getCur());
                if (explicitPath) {
                    image = explicitPath;
                    struct stat st;
                    if (stat(image.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) image = "";
                } else {
                    EmuAction *action = (EmuAction *)item->action;
                    EmuInfo *emu = action->emu;
                    std::string game = action->rompath;
                    image = makeThumbPath(&game, emu->imagePath.c_str());
                }
            }
            if (image.size() != 0 && (image.compare(shownImageKey) != 0 || previewImage == NULL)) {
                shownImageKey = image;
                previewImage = (SDL_Surface *)thumbnailCache.get(image);
                if (previewImage == NULL) {
                    LoadSurfaceWorker *worker = new LoadSurfaceWorker(image, &thumbnailCache);
                    thumbnailLoader.addWorker(worker);
                    pendingImageKey = image;
                } else
                    pendingImageKey = "";
            } else {
                if (image.size() == 0) {
                    shownImageKey = "";
                    previewImage = NULL;
                }
            }
        }
    }
}
// 0x2fb14: each frame: drop removed favorites, follow the selection with the preview, and reload
// images after a theme change.
void MenuWindow::update()
{
    if (menu) {
        bool changed = removeDeletedFavorites(menu, id);
        if (menuChanged != true) menuChanged = changed;
        updateThumbnail();
        menu->reloadTheme();
    }
    if (themePath.compare(g_themePath) != 0) {
        if (iconTF) SDL_FreeSurface(iconTF);
        iconTF = loadSkinImage(g_themePath, g_skinSubdir, "skin/icon-TF.png", g_backBuffer, false);
        themePath = g_themePath;
    }
}
// 0x2fc74: on close: save the favorites if any were removed, and remember the position in a ROM
// folder.
void MenuWindow::onHide()
{
    if (menu) {
        if (id == WIN_FAVORITES && menuChanged) {
            TextMenu *text = (TextMenu *)menu;
            saveFavorites("/mnt/SDCARD/Roms/favourite.json", text->getItems());
        }
        if (emuEntry && (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST)) {
            EmuInfo *emu = (EmuInfo *)emuEntry;
            int position = menu->getCur();
            int start = menu->getPageStart();
            int end = menu->getPageEnd();
            std::string search = menu->getSearch();
            if (search.length() == 0) {
                if (parentPath.length() != 0)
                    setRomListPosition(&g_romListPositions, emu->path + "/" + parentPath, position, start, end);
                else
                    setRomListPosition(&g_romListPositions, std::string(emu->path), position, start, end);
            }
        }
        delete menu;
    }
}
// 0x2ff04: one button: the list handles it first. MENU opens the options pop-up for the row (open,
// add to favorites, delete the ROM, search; or refresh the ROMs on the main and games menus;
// remove or clear in favorites; clear Recent). RIGHT opens the game's detail window; START toggles
// the full ROM path in the header.
int MenuWindow::handleInput()
{
    int rows;
    PopUpWindow *popup;
    int key;
    if (menu) {
        key = menu->handleKey();
        if (key == BUTTON_MENU) {
            if (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST || id == WIN_APPS || id == WIN_SEARCH) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    unsigned char canOperate = item->action->finished();
                    if (canOperate) {
                        rows = 2;
                        TextMenu *options = new TextMenu(false, false, 6, NULL);
                        if (id == WIN_APPS)
                            options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                        else
                            options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                        options->addItem(new TextItem(languageString(STR_ADD_FAVORITE), NULL, new AddFavoriteAction(item, id), true));
                        if (id == WIN_ROM_LIST || id == WIN_RETROARCH_LIST) {
                            DeleteRomAction *remove = new DeleteRomAction(menu, id);
                            remove->itemIndex = menu->getCur();
                            Action *confirm = new PopupDialogAction(STR_REMOVE_ROM, STR_REMOVE_ROM_TEXT, true, remove);
                            options->addItem(new TextItem(languageString(STR_DELETE), NULL, confirm, true));
                            rows++;
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(STR_SEARCH), NULL, searchAction, true));
                                rows++;
                            }
                        }
                        options->setUnusedPopupFlag(false);
                        popup = NULL;
                        if (rows == 2)
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-2.png");
                        else if (rows == 3)
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                        else
                            popup = new PopUpWindow(1, options, "skin/bg-pop-menu-4.png");
                        popup->onShow();
                        g_windowStack->push_back((TrimusWindow *)popup);
                    } else {
                        if (id != WIN_APPS) {
                            std::string search = menu->getSearch();
                            printf("popup menu search str is %s\n", search.c_str());
                            if (search.length() == 0) {
                                TextMenu *options = new TextMenu(false, true, 6, NULL);
                                Action *searchAction = new PopupSearchIME(menu, id, emuEntry);
                                options->addItem(new TextItem(languageString(STR_SEARCH), NULL, searchAction, true));
                                options->setUnusedPopupFlag(false);
                                PopUpWindow *window = NULL;
                                window = new PopUpWindow(1, options, "skin/bg-pop-menu-1.png");
                                window->onShow();
                                g_windowStack->push_back((TrimusWindow *)window);
                            }
                        }
                    }
                }
            } else if (id == WIN_MAIN || id == WIN_GAMES || id == WIN_RETROARCH) {
                TextMenu *options = new TextMenu(false, false, 6, NULL);
                Action *reset = new GeneralAction(-1, resetEmulatorSettings, NULL);
                Action *progress = new PopupProgressAction(STR_REFRESH, reset);
                options->addItem(new TextItem(languageString(STR_REFRESH_ROMS_2), NULL, progress, true));
                options->setUnusedPopupFlag(false);
                PopUpWindow *window = new PopUpWindow(53, options, "skin/bg-pop-menu-1.png");
                window->onShow();
                g_windowStack->push_back((TrimusWindow *)window);
            } else if (id == WIN_FAVORITES) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (item->action->result == WIN_APPS)
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    else
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    RemoveFavoriteAction *remove = new RemoveFavoriteAction(item, id);
                    Action *confirm = new PopupDialogAction(STR_REMOVE_FAVORITE, STR_REMOVE_FAVORITE_TEXT, true, remove);
                    printf("item->theaction->mWinType=%d, mitem = %p, menuitem = %p\n", item->action->result, remove, item);
                    options->addItem(new TextItem(languageString(STR_REMOVE), NULL, confirm, true));
                    ClearFavoriteAction *clear = new ClearFavoriteAction(menu, id);
                    Action *clearConfirm = new PopupDialogAction(STR_CLEAR_FAVORITE, STR_CLEAR_FAVORITE_TEXT, true, clear);
                    options->addItem(new TextItem(languageString(STR_CLEAR), NULL, clearConfirm, true));
                    options->setUnusedPopupFlag(false);
                    PopUpWindow *window = new PopUpWindow(1, options, "skin/bg-pop-menu-3.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow *)window);
                }
            } else if (id == WIN_RECENT) {
                TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                if (item) {
                    TextMenu *options = new TextMenu(false, false, 6, NULL);
                    if (id == WIN_APPS)
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    else
                        options->addItem(new TextItem(languageString(STR_OPEN), NULL, item->action, false));
                    ClearRecentAction *clear = new ClearRecentAction(menu, id);
                    Action *confirm = new PopupDialogAction(STR_CLEAR_LIST, STR_CLEAR_RECENT_TEXT, true, clear);
                    options->addItem(new TextItem(languageString(STR_CLEAR), NULL, confirm, true));
                    options->setUnusedPopupFlag(false);
                    PopUpWindow *window = new PopUpWindow(49, options, "skin/bg-pop-menu-2.png");
                    window->onShow();
                    g_windowStack->push_back((TrimusWindow *)window);
                }
            }
        } else if (key == LIST_RIGHT) {
            if (menu && menu->count() > 0 && id != WIN_DEVICE_INFO) {
                if (rightOpensDetail) {
                    TextItem *item = (TextItem *)menu->getItem(menu->getCur());
                    void *data = menu->getItemData(menu->getCur());
                    if (item && item->action) return item->action->progress((int *)item, menu->getCur());
                } else
                    rightOpensDetail = true;
            }
        } else if (key == LIST_LEFT) {
        } else if (key == BUTTON_START)
            showFullPath = !showFullPath;
        return key;
    } else {
        int result = handleGlobalInput(NULL);
        if (result == BUTTON_B)
            return 1;
        else
            return 0;
    }
}
// 0x310ac: draws the window into an off-screen copy (with the status icons).
void MenuWindow::cacheBackground()
{
    if (cachedScreen) {
        SDL_FreeSurface(cachedScreen);
        cachedScreen = NULL;
    }
    cachedScreen = createSurface(g_backBuffer->w, g_backBuffer->h);
    if (cachedScreen) {
        draw(cachedScreen);
        int x = 0;
        int next = 0;
        x = drawBatteryIndicator(cachedScreen, true);
        next = drawWifiIndicator(g_backBuffer, x, true);
        if (next >= 0) x = next;
        if (g_volumeIndicatorVisible) drawKeyStateIndicator(g_backBuffer, x, true);
    }
}
// 0x311f4: the "current/total" count at the bottom right.
int drawPageCount(SDL_Surface *dst, int current, int total)
{
    int y = 420;
    int height = TTF_FontHeight(g_labelFont);
    SDL_Rect area;
    area.x = 320;
    area.y = (60 - height) / 2 + y;
    if (total < 100)
        area.w = 300;
    else
        area.w = 280;
    area.h = 54;
    snprintf(g_countBuffer, 256, "%d", total);
    int width = drawText(dst, g_labelFont, area, g_countBuffer, g_emptyColor, 2, false);
    area.x = area.x - width;
    snprintf(g_countBuffer, 256, "%d/", current);
    drawText(dst, g_labelFont, area, g_countBuffer, g_listColor, 2, false);
    return 0;
}
// 0x31398: title frame, the list, the preview image on the right (with its background) and, for
// lists, the count or the "empty" image and the header (search string, emulator name or full path).
int MenuWindow::draw(SDL_Surface *dst)
{
    char *canonical;
    char *relative;
    int unused = 0;
    if (titleLabelId != 157)
        drawScreenFrame /* 157: the root menu, titled "MIYOO" */ (dst, titleLabelId >= 0 ? languageString(titleLabelId) : titleText.c_str(), id, !isListMenuId(id));
    else
        drawScreenFrame(dst, "MIYOO", id, !isListMenuId(id));
    bool hasImage = previewImage || (pendingImageKey.size() != 0 && pendingImageKey.compare(shownImageKey) == 0);
    if (menu && menu->count() > 0) {
        menu->drawList(dst);
        if (hasPreview && hasImage)
            menu->previewWidth = 250;
        else
            menu->previewWidth = 0;
    }
    if (hasPreview) {
        if (menu && menu->count() > 0 && hasImage) {
            SDL_Rect bounds;
            SDL_Rect dest;
            SDL_Rect source;
            if (g_previewBg) {
                source.x = 0;
                source.y = 0;
                source.w = g_previewBg->w;
                source.h = g_previewBg->h;
                dest.x = 640 - g_previewBg->w;
                dest.y = 60;
                SDL_UpperBlit(g_previewBg, &source, dst, &dest);
                bounds.x = dest.x;
                bounds.y = dest.y;
                bounds.w = g_previewBg->w;
                bounds.h = g_previewBg->h;
            } else {
                bounds.x = 390;
                bounds.y = 60;
                bounds.w = 250;
                bounds.h = 360;
            }
            if (previewImage) {
                dest.x = bounds.x + (250 - previewImage->w) / 2;
                dest.y = bounds.y + (360 - previewImage->h) / 2;
                SDL_UpperBlit(previewImage, NULL, dst, &dest);
            }
        }
    }
    if (isListMenuId(id)) {
        SDL_Rect area;
        if (menu && menu->count() > 0)
            drawPageCount(dst, menu->getCur() + 1, menu->count());
        else if (g_emptyImage) {
            area.x = (640 - g_emptyImage->w) / 2;
            area.y = (480 - g_emptyImage->h) / 2;
            SDL_UpperBlit(g_emptyImage, NULL, dst, &area);
        }
        area.x = area.y = 0;
        area.w = 640;
        area.h = 60;
        if (id != WIN_ROM_LIST && id != WIN_RETROARCH_LIST) {
            if (path.length() != 0) {
                std::string label(languageString(STR_SEARCH));
                label += std::string(":") + path;
                drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
            } else
                drawText(dst, g_titleFont, area, windowTitle(id), g_wifiEmptyColor, 1, true);
        } else {
            EmuInfo *emu = (EmuInfo *)emuEntry;
            if (emu) {
                int now = milliseconds();
                if (showFullPath) {
                    if (iconTF) {
                        SDL_Rect icon;
                        icon.x = 20;
                        icon.y = (area.h - iconTF->h) / 2;
                        SDL_UpperBlit(iconTF, NULL, dst, &icon);
                        area.x = area.x + iconTF->w + 25;
                        area.x += drawText(dst, g_imeFont, area, ":", g_hintColor, 3, true);
                    }
                    canonical = canonicalize_file_name(emu->path.c_str());
                    relative = NULL;
                    if (canonical == NULL) canonical = strdup(emu->path.c_str());
                    if (canonical) {
                        relative = strstr(canonical, "/mnt/SDCARD");
                        relative += 11;
                    }
                    if (relative) drawText(dst, g_imeFont, area, relative, g_hintColor, 3, true);
                    free(canonical);
                } else {
                    if (path.length() != 0) {
                        std::string label = emu->label + std::string(":") + path;
                        drawText(dst, g_titleFont, area, label.c_str(), g_wifiEmptyColor, 1, true);
                    } else
                        drawText(dst, g_titleFont, area, emu->label.c_str(), g_wifiEmptyColor, 1, true);
                }
            }
        }
    }
    return 0;
}
// 0x31ed4: an icon (optionally scaled to 128x48) and text for the header.
void MenuWindow::setHeaderIcon(SDL_Surface *icon, const char *text, bool scale, bool owns)
{
    if (icon) {
        if (scale) {
            headerIcon = SDL_CreateRGBSurface(0, 128, 48, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
            if (headerIcon) {
                scalePixels(0, 0, icon->w, icon->h, 128, 48, icon->w, 0, (int *)icon->pixels, (int *)headerIcon->pixels);
                if (owns) SDL_FreeSurface(icon);
                ownsHeaderIcon = true;
            } else {
                ownsHeaderIcon = owns;
                headerIcon = icon;
            }
        } else {
            headerIcon = icon;
            ownsHeaderIcon = owns;
        }
    }
    headerText = text;
}
// 0x32040: base of the pop-ups and dialogs: drawn over a dimmed copy of the screen behind them.
FloatWindow::FloatWindow(int title) : TrimusWindow(title, -1)
{
    backdrop = NULL;
}
// 0x32094 (D0 0x320f0)
FloatWindow::~FloatWindow()
{
    if (backdrop) SDL_FreeSurface(backdrop);
}
// 0x32124: draw the dimmed backdrop.
int FloatWindow::draw(SDL_Surface *dst)
{
    SDL_UpperBlit(backdrop, NULL, dst, NULL);
    return 0;
}
// 0x32160: when shown: copy the screen and dim it (except the bottom bar).
void FloatWindow::onShow()
{
    if (backdrop == NULL) {
        backdrop = SDL_CreateRGBSurface(0, 640, 480, 32, 0, 0, 0, 0);
        SDL_UpperBlit(g_backBuffer, NULL, backdrop, NULL);
        SDL_Surface *shade = createSurface(640, 480);
        if (shade) {
            SDL_Rect source = {
                0, 0, 640, 420};
            SDL_Rect dest = {
                0, 0, 0, 0};
            SDL_FillRect(shade, NULL, 0xaa000000);
            SDL_UpperBlit(shade, &source, backdrop, &dest);
            SDL_FreeSurface(shade);
        }
    }
}
// =================================================================================================
// 0x3225c-0x36838: Pop-ups, dialogs, the power-off countdown and the game detail window.
// =================================================================================================

// 0x3225c: a pop-up list (the options of MENU) over the dimmed screen, sized to its background
// image (skin/bg-pop-menu-N.png).
PopUpWindow::PopUpWindow(int titleLabelId, TextMenu *list, const char *file) : FloatWindow(titleLabelId)
{
    background = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    menu = list;
    printf("load menu bg %s : %p\n", file, background);
    if (menu) {
        SDL_Rect rect;
        if (background) {
            rect.x = rect.y = 0;
            rect.w = background->w;
            rect.h = background->h - 8;
            if (menu->count() > 1)
                menu->setRowHeight(rect.h / menu->count());
            else
                menu->setRowHeight(rect.h);
        } else {
            rect.x = rect.y = 0;
            rect.w = 120;
            rect.h = 75;
        }
        menu->setRect(&rect);
    }
}
// 0x3247c (D0 0x32534)
PopUpWindow::~PopUpWindow()
{
    if (menu) {
        delete menu;
        menu = NULL;
    }
    if (background) SDL_FreeSurface(background);
    puts("~PopupWindow end");
}
// 0x32568: nothing to do on close.
void PopUpWindow::onHide()
{
}
// 0x32588: the dimmed backdrop, the footer with OK/Cancel, the pop-up background and its rows.
int PopUpWindow::draw(SDL_Surface *dst)
{
    FloatWindow::draw(dst);
    if (g_background) {
        SDL_Rect source;
        source.x = 0;
        source.y = 420;
        source.w = 640;
        source.h = 60;
        SDL_Rect dest;
        dest.x = 0;
        dest.y = 420;
        SDL_UpperBlit(g_background, &source, dst, &dest);
        if (g_unusedIndicator) {
            source.y = g_unusedIndicator->h - 60;
            SDL_UpperBlit(g_unusedIndicator, &source, dst, &dest);
        }
        drawFooter(dst, languageString(STR_OK), languageString(STR_CANCEL));
    }
    if (menu) {
        if (background) {
            SDL_Rect dest;
            dest.x = 0;
            dest.y = 0;
            SDL_UpperBlit(background, NULL, dst, &dest);
        } else {
            SDL_Rect source = {
                0, 0, 0, 0};
            source.w = menu->getRect()->w;
            source.h = menu->getRect()->h;
            SDL_UpperBlit(g_background, &source, dst, menu->getRect());
        }
        menu->drawList(dst);
    }
    return 0;
}
// 0x32784: the list handles the buttons (1 closes).
int PopUpWindow::handleInput()
{
    if (menu) return menu->handleKey();
    return 1;
}
// 0x327e4: a short message with one button image; it closes on any key or after 5 s (and then calls
// fn).
NotifyWindow::NotifyWindow(const char *message, const char *button, const char *file, void (*fn)()) : FloatWindow(-1)
{
    text = message;
    notifyTimer = NULL;
    notifyBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    notifyButton = loadSkinImage(g_themePath, g_skinSubdir, file, g_backBuffer, false);
    buttonLabel = button;
    callback = fn;
}
// 0x32920 (D0 0x329cc)
NotifyWindow::~NotifyWindow()
{
    if (notifyTimer) SDL_RemoveTimer(notifyTimer);
    if (notifyBackground) SDL_FreeSurface(notifyBackground);
    if (notifyButton) SDL_FreeSurface(notifyButton);
}
// 0x32a00: timer: post EVENT_NOTIFY_TIMEOUT and run the callback.
static unsigned notifyTimeout(unsigned interval, void *arg)
{
    void (*fn)() = (void (*)())arg;
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_NOTIFY_TIMEOUT;
    timeval now;
    gettimeofday(&now, NULL);
    printf("timeout sec = %d\n", now.tv_sec);
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    if (fn) fn();
    return interval;
}
// 0x32aa0: start the 5 s timer; without a pop-up skin, cut a bordered box out of the background
// instead.
void NotifyWindow::onShow()
{
    FloatWindow::onShow();
    timeval now;
    gettimeofday(&now, NULL);
    printf("init sec = %d\n", now.tv_sec);
    notifyTimer = SDL_AddTimer(5000, notifyTimeout, (void *)callback);
    if (notifyBackground == NULL && g_background) {
        notifyBackground = SDL_CreateRGBSurface(0, 240, 90, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            200, 195, 240, 90};
        SDL_UpperBlit(g_background, &r, notifyBackground, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 89;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 90;
        SDL_FillRect(notifyBackground, &r, SDL_MapRGB(notifyBackground->format, 255, 255, 255));
    }
}
// 0x32d0c: any key release or the timeout closes it.
int NotifyWindow::handleInput()
{
    SDL_Event event;
    int key = handleGlobalInput(&event);
    if (event.type == SDL_KEYUP)
        return 1;
    else if (event.type == SDL_USEREVENT && event.user.code == EVENT_NOTIFY_TIMEOUT)
        return 1;
    return -1;
}
// 0x32d74: the box, the message (several lines) and the button with its label.
int NotifyWindow::draw(SDL_Surface *dst)
{
    FloatWindow::draw(dst);
    SDL_Rect r;
    if (notifyBackground) {
        r.x = (640 - notifyBackground->w) / 2;
        r.y = (480 - notifyBackground->h) / 2;
        r.w = notifyBackground->w;
        r.h = notifyBackground->h;
        SDL_UpperBlit(notifyBackground, NULL, dst, &r);
    } else {
        r.x = 200;
        r.y = 195;
        r.w = 240;
        r.h = 90;
    }
    if (notifyButton) {
        r.h -= notifyButton->h / 2;
        r.y -= notifyButton->h / 4;
    }
    drawTextLines(dst, g_titleFont, &r, text.c_str(), "\n", &g_selectedTextColor, 1, false);
    if (notifyButton && notifyBackground) {
        r.x = (640 - notifyButton->w) / 2;
        r.y = 468 - (480 - notifyBackground->h) / 2 - notifyButton->h;
        SDL_UpperBlit(notifyButton, NULL, dst, &r);
        r.w = notifyButton->w;
        r.h = notifyButton->h;
        drawText(dst, g_titleFont, r, buttonLabel, g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33060: the power-off countdown: "Power off in N seconds" from 10; any button cancels (calling
// cancel), the end calls timeout.
CountDownWindow::CountDownWindow(int label, void (*timeout)(), void (*cancel)()) : NotifyWindow("", languageString(STR_CANCEL), "skin/bg-button-f.png", timeout)
{
    title = label;
    seconds = 10;
    tickTimer = NULL;
    onCancel = cancel;
}
// 0x330fc (D0 0x33158)
CountDownWindow::~CountDownWindow()
{
    if (tickTimer) SDL_RemoveTimer(tickTimer);
}
// 0x3318c: timer: post EVENT_COUNTDOWN_TICK every second.
static unsigned countdownTick(unsigned interval, void *arg)
{
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_COUNTDOWN_TICK;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
// 0x331ec: start the full-length timeout and the one-second tick.
void CountDownWindow::onShow()
{
    NotifyWindow::onShow();
    snprintf(countdownText, 128, languageString(title), seconds);
    text = countdownText;
    if (notifyTimer) SDL_RemoveTimer(notifyTimer);
    notifyTimer = SDL_AddTimer(seconds * 1000, notifyTimeout, (void *)callback);
    tickTimer = SDL_AddTimer(1000, countdownTick, NULL);
}
// 0x332d8: a tick counts down; any other input except status events cancels.
int CountDownWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    if (key != EVENT_BATTERY && key != EVENT_HOTPLUG && key != EVENT_COUNTDOWN_TICK) {
        if (onCancel) onCancel();
        return 1;
    } else {
        if (key == EVENT_COUNTDOWN_TICK) {
            seconds--;
            snprintf(countdownText, 128, languageString(title), seconds);
            text = countdownText;
            SDL_RemoveTimer(tickTimer);
            tickTimer = SDL_AddTimer(1000, countdownTick, NULL);
        }
        return -1;
    }
}
// 0x333ec: a yes/no question: heading and message are language strings; OK runs action (owned when
// owns).
DialogWindow::DialogWindow(int heading, int message, bool owns, Action *act) : FloatWindow(-1)
{
    title = heading;
    text = message;
    selected = 1;
    popupBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    if (popupBackground) {
        noPopupSkin = false;
        buttonHighlight = NULL;
    } else {
        buttonHighlight = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-list-s.png", g_backBuffer, false);
        noPopupSkin = true;
    }
    iconA = g_iconA;
    iconB = g_iconB;
    action = act;
    ownsAction = owns;
}
// 0x33570 (D0 0x33630)
DialogWindow::~DialogWindow()
{
    if (popupBackground) SDL_FreeSurface(popupBackground);
    if (buttonHighlight) SDL_FreeSurface(buttonHighlight);
    if (action) delete action;
}
// 0x33664: without a pop-up skin, draw a bordered box with two button cells instead.
void DialogWindow::onShow()
{
    FloatWindow::onShow();
    if (g_background && noPopupSkin) {
        popupBackground = SDL_CreateRGBSurface(0, 240, 120, 32, 0, 0, 0, 0);
        SDL_Rect r = {
            0, 0, 240, 120};
        SDL_UpperBlit(g_background, &r, popupBackground, NULL);
        r.x = 0;
        r.y = 0;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 119;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 239;
        r.y = 0;
        r.w = 1;
        r.h = 120;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 0;
        r.y = 90;
        r.w = 240;
        r.h = 1;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
        r.x = 120;
        r.y = 90;
        r.w = 1;
        r.h = 30;
        SDL_FillRect(popupBackground, &r, SDL_MapRGB(popupBackground->format, 255, 255, 255));
    }
}
// 0x33934: the plain look: message and two buttons, Cancel and OK, the selected one highlighted.
int DialogWindow::drawFallback(SDL_Surface *dst)
{
    SDL_Rect r;
    r.x = 200;
    r.y = 180;
    r.w = 240;
    r.h = 120;
    if (popupBackground) SDL_UpperBlit(popupBackground, NULL, dst, &r);
    r.h = 90;
    drawText(dst, g_titleFont, r, languageString(text), g_selectedTextColor, 1, false);
    for (int i = 0; i < 2; i++) {
        r.x = 200 + i * 120;
        r.y = 270;
        r.w = 120;
        r.h = 30;
        if (i == selected) {
            if (buttonHighlight) {
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = r.w;
                source.h = r.h;
                SDL_UpperBlit(buttonHighlight, &source, dst, &r);
            }
            drawText(dst, g_titleFont, r, (i == 0 ? languageString(STR_CANCEL) : languageString(STR_OK)), g_imeTextColor, 1, false);
        } else
            drawText(dst, g_titleFont, r, (i == 0 ? languageString(STR_CANCEL) : languageString(STR_OK)), g_imeTextColor, 1, false);
    }
    return 0;
}
// 0x33bac: the skinned look: heading, message, and A = OK / B = Cancel hints.
int DialogWindow::draw(SDL_Surface *dst)
{
    FloatWindow::draw(dst);
    if (noPopupSkin != true && popupBackground) {
        SDL_Rect r;
        r.x = (640 - popupBackground->w) / 2;
        r.y = (480 - popupBackground->h) / 2;
        r.w = popupBackground->w;
        r.h = popupBackground->h;
        SDL_UpperBlit(popupBackground, NULL, dst, &r);
        r.h = 50;
        drawText(dst, g_titleFont, r, languageString(title), g_emptyColor, 1, false);
        r.y = (480 - popupBackground->h) / 2 + 50;
        r.h = 100;
        drawTextLines(dst, g_titleFont, &r, languageString(text), "\n", &g_selectedTextColor, 1, false);
        r.x = (640 - popupBackground->w) / 2 + 230;
        r.y = (480 - popupBackground->h) / 2 + 160;
        r.w = popupBackground->w;
        r.h = iconA->h;
        SDL_UpperBlit(iconA, NULL, dst, &r);
        r.x = r.x + iconA->w + 10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(STR_OK), g_clearColor, 3, false);
        r.x = (640 - popupBackground->w) / 2 + 230;
        r.y = (480 - popupBackground->h) / 2 + iconA->h + 170;
        SDL_UpperBlit(iconB, NULL, dst, &r);
        r.x = r.x + iconB->w + 10;
        r.y += 5;
        drawText(dst, g_titleFont, r, languageString(STR_CANCEL), g_clearColor, 3, false);
    } else
        return drawFallback(dst);
    return 0;
}
// 0x3400c: A or Y on OK runs the action; B, X or A on Cancel closes; left/right choose the button.
int DialogWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_A:
        case BUTTON_Y:
            if (selected == 1 && action) return action->execute(NULL);
            return 1;
        case BUTTON_B:
        case BUTTON_X: return 1;
        case BUTTON_LEFT:
            if (selected > 0) selected--;
            break;
        case BUTTON_RIGHT:
            if (selected < 1) selected++;
            break;
        default: break;
    }
    return -1;
}
// 0x34134: thread: run the dialog's action while g_busy is set.
static int progressThread(void *arg)
{
    ProgressDialog *dialog = (ProgressDialog *)arg;
    g_busy = 1;
    dialog->runAction();
    g_busy = 0;
    return 0;
}
// 0x34184: a "please wait" pop-up that runs an action on a thread (e.g. Wi-Fi connect, refresh
// ROMs).
ProgressDialog::ProgressDialog(int heading, Action *act) : FloatWindow(-1)
{
    title = heading;
    action = act;
    progressBackground = loadSkinImage(g_themePath, g_skinSubdir, "skin/pop-bg.png", g_backBuffer, false);
    progressButton = loadSkinImage(g_themePath, g_skinSubdir, "skin/bg-button-f.png", g_backBuffer, false);
    actionThread = NULL;
}
// 0x342ac (D0 0x3437c)
ProgressDialog::~ProgressDialog()
{
    if (progressBackground) SDL_FreeSurface(progressBackground);
    if (progressButton) SDL_FreeSurface(progressButton);
    if (action) delete action;
}
// 0x343b0: start the action thread.
void ProgressDialog::onShow()
{
    FloatWindow::onShow();
    actionThread = SDL_CreateThread(progressThread, this);
}
// 0x343f4
void ProgressDialog::runAction()
{
    action->execute(NULL);
}
// 0x34438: wait for the action to finish.
void ProgressDialog::onHide()
{
    SDL_WaitThread(actionThread, NULL);
}
// 0x34468: draws the pop-up and animates the dots until the action finishes (this blocks the window
// loop).
int ProgressDialog::draw(SDL_Surface *dst)
{
    FloatWindow::draw(g_backBuffer);
    int frame = 0;
    SDL_Rect r;
    if (progressBackground) {
        r.x = (640 - progressBackground->w) / 2;
        r.y = (480 - progressBackground->h) / 2;
        r.w = progressBackground->w;
        r.h = progressBackground->h;
        SDL_UpperBlit(progressBackground, NULL, g_backBuffer, &r);
    } else {
        r.x = 200;
        r.y = 195;
        r.w = 240;
    }
    r.h = 50;
    drawText(g_backBuffer, g_imeFont, r, languageString(title), g_emptyColor, 1, false);
    SDL_Surface *dot = loadSkinImage(g_themePath, g_skinSubdir, "skin/progress-dot.png", g_backBuffer, false);
    SDL_Rect target;
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    while (g_busy) {
        if (dot) {
            r.x = 265;
            r.y = (480 - dot->h) / 2;
            r.w = 140;
            r.h = dot->h;
            if (frame / 5 % 4 == 0) {
                SDL_UpperBlit(backdrop, &r, g_backBuffer, &r);
                if (progressBackground) {
                    target = r;
                    target.x = r.x - (640 - progressBackground->w) / 2;
                    target.y = r.y - (480 - progressBackground->h) / 2;
                    SDL_UpperBlit(progressBackground, &target, g_backBuffer, &r);
                }
            }
            if (frame / 5 % 4 == 1) SDL_UpperBlit(dot, NULL, g_backBuffer, &r);
            if (frame / 5 % 4 == 2) {
                target = r;
                target.x = r.x + (140 - dot->w * 3) / 4 + dot->w;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &target);
            }
            if (frame / 5 % 4 == 3) {
                target = r;
                target.x = r.x + (140 - dot->w * 3) / 2 + dot->w * 2;
                SDL_UpperBlit(dot, NULL, g_backBuffer, &target);
            }
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_Flip(g_screen);
        }
        SDL_Delay(28);
        frame++;
    }
    if (dot) SDL_FreeSurface(dot);
    return 0;
}
// 0x349f0: closes as soon as the loop gets input back (the action is done).
int ProgressDialog::handleInput()
{
    return 1;
}
// 0x34a14: an empty game detail window (state restore fills it).
GameDetailWindow::GameDetailWindow(int title, int id) : TrimusWindow(title, id)
{
    sourceWindowId = 0;
    listIndex = 0;
    favorite = false;
    thumbnailImage = 0;
    favoriteImage = 0;
    animatePending = true;
    animationCapture = 0;
    parentMenu = 0;
    openedIndex = 0;
}
// 0x34b1c: a game detail window: title, item (emulator label), game path, thumbnail folder, list
// index, window id of the list (kind) and favorite mark.
GameDetailWindow::GameDetailWindow(int title, int id, const char *name, const char *item, const char *game, const char *icon, int index, int kind, bool fav) : TrimusWindow(title, id)
{
    this->item = item;
    this->name = name;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    sourceWindowId = kind;
    listIndex = index;
    favorite = fav;
    thumbnailImage = NULL;
    favoriteImage = NULL;
    animationCapture = NULL;
    animatePending = true;
    openedIndex = index;
    parentMenu = NULL;
    loadInfo();
}
// 0x34d0c (D0 0x34d98)
GameDetailWindow::~GameDetailWindow()
{
}
// 0x34dcc: free the images.
void GameDetailWindow::onHide()
{
    if (thumbnailImage) {
        SDL_FreeSurface(thumbnailImage);
        thumbnailImage = NULL;
    }
    if (favoriteImage) {
        SDL_FreeSurface(favoriteImage);
        favoriteImage = NULL;
    }
    if (animationCapture) {
        SDL_FreeSurface(animationCapture);
        animationCapture = NULL;
    }
}
// 0x34e6c: load the thumbnail (or the default one) and the favorite mark.
void GameDetailWindow::onShow()
{
    printf("Load thumbnail %s\n", thumbPath.c_str());
    thumbnailImage = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (thumbnailImage == NULL) thumbnailImage = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    if (favorite) favoriteImage = loadSkinImage(g_themePath, g_skinSubdir, "skin/ic-favorite-mark.png", g_backBuffer, false);
}
// 0x34f84: find the list this window was opened from (the window below it), to move through it with
// up/down.
void GameDetailWindow::update()
{
    if (g_windowStack && !parentMenu) {
        if (g_windowStack->size() > 2) {
            MenuWindow *parent = (MenuWindow *)(*g_windowStack)[g_windowStack->size() - 2];
            parentMenu = (TextMenuDrawer *)parent->getMenu();
            printf("parent win is %s, %d: %p\n", parent->getTitle(), parent->getId(), parentMenu);
        }
    }
}
// 0x35080: wraps the game name onto up to four lines of about 15 bytes (not splitting UTF-8
// characters), adding "-" or "...".
void GameDetailWindow::loadInfo()
{
    int length = name.length();
    nameLines.clear();
    std::string line;
    int count = 0;
    int start = 0;
    int end = 0;
    while (start + 15 < length && count <= 3 && end < length) {
        end = start + 15;
        while (end < length && (name.at(end) & 0xc0) == 0x80) end++;
        line = name.substr(start, end - start);
        start = end;
        count++;
        if (end < length && line.at(line.length() - 1) != ' ' && count <= 3) line = line + "-";
        nameLines.push_back(line);
    }
    if (start < length - 1) {
        if (count < 4) {
            line = name.substr(start);
            nameLines.push_back(line);
            start += line.length();
        } else {
            line = nameLines.back();
            nameLines.pop_back();
            line += "...";
            nameLines.push_back(line);
        }
    }
}
// 0x353a0: show another game.
void GameDetailWindow::resetDetails(const char *heading, const char *label, const char *game, const char *icon, int index, int kind, bool fav)
{
    name = heading;
    item = label;
    launch = game;
    thumbPath = makeThumbPath(&launch, icon);
    printf("GameDetailWindow::resetDetailsInternal(name %s ,label %s) thumbnail path %s\n", heading, label, thumbPath.c_str());
    sourceWindowId = kind;
    listIndex = index;
    favorite = fav;
    if (thumbnailImage) SDL_FreeSurface(thumbnailImage);
    thumbnailImage = loadThumbnail(thumbPath.c_str(), 250, 360);
    if (thumbnailImage == NULL) thumbnailImage = loadSkinImage(g_themePath, g_skinSubdir, "./skin/thumb-default.png", g_backBuffer, false);
    loadInfo();
}
// 0x35538: show the game of a list row (app or ROM).
void GameDetailWindow::resetFromItem(TextItem *row, int index)
{
    if (row->action->result == WIN_APPS) {
        AppAction *action = (AppAction *)row->action;
        AppInfo *app = action->app;
        int kind = action->result;
        std::string image = app->iconSelected;
        resetDetails(row->label.c_str(), app->label.c_str(), app->launch.c_str(), image.c_str(), index, kind, row->favorite);
    } else {
        EmuAction *action = (EmuAction *)row->action;
        EmuInfo *emu = action->emu;
        int kind = action->result;
        resetDetails(row->label.c_str(), emu->label.c_str(), action->rompath.c_str(), action->imgpath.c_str(), index, kind, row->favorite);
    }
}
// 0x356e4: the thumbnail on the left, the list number, the wrapped name and the favorite mark.
int GameDetailWindow::draw(SDL_Surface *dst)
{
    if (titleLabelId != 157)
        drawScreenFrame(dst, languageString(titleLabelId), id, true);   /* 157: "MIYOO" */
    else
        drawScreenFrame(dst, "MIYOO", id, true);
    SDL_Rect r;
    if (thumbnailImage) {
        int height = 360;
        SDL_Rect source;
        SDL_Rect dest;
        if (thumbnailImage->w > 250) {
            source.w = 250;
            source.x = (thumbnailImage->w - 250) / 2;
        } else {
            source.w = thumbnailImage->w;
            source.x = 0;
        }
        if (thumbnailImage->h > height) {
            source.h = height;
            source.y = (thumbnailImage->h - height) / 2;
        } else {
            source.h = thumbnailImage->h;
            source.y = 0;
        }
        dest.x = (250 - source.w) / 2;
        dest.y = (360 - thumbnailImage->h) / 2 + 60;
        SDL_UpperBlit(thumbnailImage, &source, dst, &dest);
    }
    if (g_numberBg && g_listNumber) {
        r.x = 280;
        r.y = 80;
        r.w = 124;
        r.h = 50;
        drawListNumber(listIndex + 1, &r, dst, g_imeFont);
    }
    r.x = 284;
    r.w = 350;
    if (nameLines.size() == 0) {
        r.y = 145;
        r.h = 35;
        drawText(dst, g_titleFont, r, name.c_str(), g_imeTextColor, 0, false);
    } else {
        for (int i = 0; i < nameLines.size(); i++) {
            r.y = i * 35 + 145;
            r.h = 35;
            drawText(dst, g_titleFont, r, nameLines[i].c_str(), g_imeTextColor, 0, false);
        }
    }
    if (favoriteImage && favorite) {
        r.x = 284;
        r.y = r.y + r.h + 10;
        SDL_UpperBlit(favoriteImage, NULL, dst, &r);
        r.x = r.x + favoriteImage->w + 10;
        r.w = 630 - r.x;
        r.h = 35;
        drawText(dst, g_titleFont, r, languageString(STR_FAVORITE_LOWER), g_imeTextColor, 0, false);
    }
    return 0;
}
// 0x35bbc: A/Y launch the game with its emulator; B, X or LEFT close; up/down move to the
// previous/next game of the list.
int GameDetailWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_A:
        case BUTTON_Y:
            if (sourceWindowId == WIN_ROM_LIST) {
                int index = findEmuByLabelPrefix(&g_emuFolders, item.c_str());
                printf("find emu %s return %d\n", item.c_str(), index);
                if (index >= 0 && (unsigned)index < g_emuFolders.size()) return launchRom(name.c_str(), &g_emuFolders[index], launch.c_str(), thumbPath.c_str(), sourceWindowId);
            } else if (sourceWindowId == WIN_RETROARCH_LIST) {
                int index = findEmuByLabelPrefix(&g_rappFolders, item.c_str());
                printf("find emu %s return %d\n", item.c_str(), index);
                if (index >= 0 && (unsigned)index < g_rappFolders.size()) return launchRom(name.c_str(), &g_rappFolders[index], launch.c_str(), thumbPath.c_str(), sourceWindowId);
            } else
                printf("unsupport game type %d\n", sourceWindowId);
            return 1;
        case BUTTON_B:
        case BUTTON_X:
        case BUTTON_LEFT: return 1;
        case BUTTON_UP:
            if (parentMenu) {
                parentMenu->moveUp();
                TextItem *row = (TextItem *)parentMenu->getItem(parentMenu->getCur());
                if (row) resetFromItem(row, parentMenu->getCur());
            }
            break;
        case BUTTON_DOWN:
            if (parentMenu) {
                parentMenu->moveDown();
                TextItem *row = (TextItem *)parentMenu->getItem(parentMenu->getCur());
                if (row) resetFromItem(row, parentMenu->getCur());
            }
        // The final case falls through to this shared exit in the original.
        default: break;
    }
    return -1;
}
// 0x35fe4: what /tmp/state.json keeps of this window.

void GameDetailWindow::saveState(cJSON *obj)
{
    cJSON *gameName = cJSON_CreateString(name.c_str());
    cJSON_AddItemToObject(obj, "gamename", gameName);
    cJSON *gamePath = cJSON_CreateString(launch.c_str());
    cJSON_AddItemToObject(obj, "gamepath", gamePath);
    cJSON *thumb = cJSON_CreateString(thumbPath.c_str());
    cJSON_AddItemToObject(obj, "thumbpath", thumb);
    cJSON *gameIndex = cJSON_CreateNumber(listIndex);
    cJSON_AddItemToObject(obj, "gameidx", gameIndex);
    cJSON *gameType = cJSON_CreateNumber(sourceWindowId);
    cJSON_AddItemToObject(obj, "gametype", gameType);
    cJSON *isFavorite = cJSON_CreateBool(favorite);
    cJSON_AddItemToObject(obj, "favourite", isFavorite);
}
// 0x36140: reads it back.
int GameDetailWindow::restoreState(cJSON *obj)
{
    cJSONItem *entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gamename");
    name = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gamepath");
    launch = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "thumbpath");
    thumbPath = entry->valuestring;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gameidx");
    listIndex = entry->valueint;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "gametype");
    sourceWindowId = entry->valueint;
    entry = (cJSONItem *)cJSON_GetObjectItem(obj, "favourite");
    favorite = entry->valueint != 0;
    return 0;
}
// 0x36288: slides the window in from the right over the captured list (20 eased steps), once.
void GameDetailWindow::animateIn(SDL_Surface *dst, FlipFn flip)
{
    if (animatePending) {
        animatePending = false;
        SDL_Flip(g_screen);
        if (animationCapture && listIndex == openedIndex) {
            int width = animationCapture->w;
            int height = animationCapture->h;
            SDL_Surface *surface = createSurface(width, height);
            if (surface) {
                int now = 0;
                draw(surface);
                float t = 0.0;
                for (int i = 0; i < 20; i++) {
                    t = t + 0.05;
                    float eased = 1.0 - (1.0 - t) * (1.0 - t);
                    int x = eased * 640.0f;
                    timeval tv;
                    gettimeofday(&tv, NULL);
                    now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
                    printf("anim %d at %d\n", x, now);
                    SDL_Rect source;
                    source.x = x;
                    source.y = 0;
                    source.w = width - x;
                    source.h = height;
                    SDL_Rect dest;
                    dest.x = 0;
                    dest.y = 0;
                    SDL_UpperBlit(animationCapture, &source, dst, &dest);
                    source.x = 0;
                    source.y = 0;
                    source.w = x;
                    source.h = height;
                    dest.x = width - x;
                    dest.y = 0;
                    SDL_UpperBlit(surface, &source, dst, &dest);
                    flip();
                }
                SDL_FreeSurface(surface);
            }
        }
    }
}
// 0x36534: slides it back out.
void GameDetailWindow::animateOut(SDL_Surface *dst, FlipFn flip)
{
    animatePending = true;
    if (animationCapture && listIndex == openedIndex) {
        int width = animationCapture->w;
        int height = animationCapture->h;
        SDL_Surface *surface = createSurface(width, height);
        if (surface) {
            draw(surface);
            float t = 0.0;
            for (int i = 0; i < 20; i++) {
                t = t + 0.05;
                float eased = 1.0 - (1.0 - t) * (1.0 - t);
                int x = eased * 640.0f;
                SDL_Rect source;
                source.x = 0;
                source.y = 0;
                source.w = width - x;
                source.h = height;
                SDL_Rect dest;
                dest.x = x;
                dest.y = 0;
                SDL_UpperBlit(surface, &source, dst, &dest);
                source.x = width - x;
                source.y = 0;
                source.w = x;
                source.h = height;
                dest.x = 0;
                dest.y = 0;
                SDL_UpperBlit(animationCapture, &source, dst, &dest);
                flip();
            }
            SDL_FreeSurface(surface);
        }
    }
}
// 0x3675c: keep a copy of the screen to slide over.
void GameDetailWindow::captureBackground(SDL_Surface *source)
{
    if (source) {
        animationCapture = createSurface(source->w, source->h);
        if (animationCapture) SDL_UpperBlit(source, NULL, animationCapture, NULL);
    }
}
// 0x367d8: timer posting EVENT_NOTIFY_TIMEOUT; the ADC info window uses it as a 500 ms
// refresh tick.
static unsigned detailTimeout(unsigned interval, void *arg)
{
    SDL_UserEvent event;
    event.type = SDL_USEREVENT;
    event.code = EVENT_NOTIFY_TIMEOUT;
    if (SDL_PushEvent(&event) < 0) puts("Failed to push SDL event");
    return interval;
}
// =================================================================================================
// 0x36838-0x39240: ADC info, the window loop, the menu factory and the charging screen.
// =================================================================================================
#include <fcntl.h>
#include "HardwareTestMenu.h"

extern std::vector<TrimusWindow *> g_pendingWindows;
extern SDL_Surface *g_gameFocused, *g_gameNormal, *g_raRow;
// 0x1821c0, 0x1821c4, 0x1821d0
extern SDL_Thread *g_loadingThread;
// 0x180854
extern TTF_Font *g_iconFont;
// 0x182180
bool sdCardMounted();
int enableSwap();
bool dualPartition();
// 0x36838: "ADC INFO": shows the battery reading, refreshed every 500 ms.
ADCInfoWindow::ADCInfoWindow(int title, int id) : TrimusWindow(title, id)
{
    timer = SDL_AddTimer(500, detailTimeout, NULL);
}
// 0x368b8 (D0 0x36914)
ADCInfoWindow::~ADCInfoWindow()
{
    if (timer) SDL_RemoveTimer(timer);
}
// 0x36948: B closes.
int ADCInfoWindow::handleInput()
{
    int key = handleGlobalInput(NULL);
    switch (key) {
        case BUTTON_B: return 1;
        default: break;
    }
    return -1;
}
// 0x36990: the battery value and a status line. Original quirk, kept as-is: the status is fixed at
// "Disconnected".
int ADCInfoWindow::draw(SDL_Surface *dst)
{
    if (titleLabelId != 157)
        drawScreenFrame(dst, languageString(titleLabelId), id, true);
    else
        drawScreenFrame(dst, "MIYOO", id, true);
    int status = 2;   // never read from the device
    const char *label = NULL;
    switch (status) {
        case 0:
            label = languageString(STR_CHARGING);
            break;
        case 1:
            label = languageString(STR_OTG);
            break;
        case 2:
            label = languageString(STR_DISCONNECTED);
            break;
        case 3:
            label = languageString(STR_ERROR);
            break;
    }
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 60;
    rect.w = 640;
    rect.h = 360;
    int battery = 100;
    int voltage = 0;
    int charging = 0;
    readAxpStatus(&battery, &voltage, &charging);
    snprintf(g_launchCmd, 0x800, "%s     %d\n%s  %s\n", languageString(STR_ADC), battery, languageString(STR_STATUS), label);
    drawTextLines(dst, g_imeFont, &rect, g_launchCmd, "\n", &g_imeTextColor, 1, false);
    return 0;
}
// 0x36b68: closes and deletes every window, top first.
void clearWindows(std::vector<TrimusWindow *> *windows)
{
    printf("%ld window to free\n", windows->size());
    while (windows->size() != 0) {
        TrimusWindow *window = windows->back();
        windows->pop_back();
        window->onHide();
        delete window;
    }
}
// 0x36c20: show the back buffer: copy it to the screen and pan the framebuffer.
void flipScreen()
{
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    flipDisplay();
}
// 0x36c58: show the back buffer with SDL_UpdateRect (the charging screen).
void updateScreen()
{
    SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
    SDL_UpdateRect(g_screen, 0, 0, 0, 0);
}
// 0x36cb8: deletes windows queued by pushWindow that were never shown.
void flushPendingWindows()
{
    for (int i = 0; i < g_pendingWindows.size(); i++) {
        g_pendingWindows[i]->onHide();
        delete g_pendingWindows[i];
    }
    g_pendingWindows.clear();
}
// 0x36d88: queue a window to open on top; the window loop shows it after the current frame (only
// the last one queued is kept).
void pushWindow(TrimusWindow *window)
{
    flushPendingWindows();
    g_pendingWindows.push_back(window);
}
// 0x36dbc: opens /dev/fb0 and pans back to the first page.
void initFramebuffer()
{
    g_fbFd = open("/dev/fb0", 2);
    if (g_fbFd >= 0) {
        if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &g_fbVar) == -1) {
            close(g_fbFd);
            g_fbFd = -1;
            puts("cat't ioctl /dev/fb0 ");
        } else {
            g_fbVar.yoffset = 0;
            ioctl(g_fbFd, FBIOPAN_DISPLAY, &g_fbVar);
        }
    } else
        puts("cat't open /dev/fb0 ");
}
// 0x36eb0
void closeFramebuffer()
{
    if (g_fbFd >= 0) close(g_fbFd);
}
// 0x36ee8: the window loop. Each frame: update, slide in, draw the top window and the status icons
// (battery, sound card, Wi-Fi, volume), show it; open a queued window if there is one; otherwise
// hand every pending input event to the top window and act on its result (WindowResult). Ends when
// the stack is empty, a window quits or launches, or SIGTERM; then saves the window state for the
// next start and the ROM list positions.
void runWindowLoop(std::vector<TrimusWindow *> *windows, std::atomic<bool> *running)
{
    int result;
    int slot;
    int count;
    TrimusWindow *window;
    int windowId;
    int next;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    }
    g_windowStack = windows;
    count = 0;
    while (windows->size() != 0 && *running) {
        window = windows->back();
        window->update();
        window->animateIn(g_backBuffer, flipScreen);
        window->draw(g_backBuffer);
        windowId = window->getId();
        if (windowId != WIN_TEST && windowId != -1) {
            slot = drawBatteryIndicator(g_backBuffer, true);
            next = drawSoundCardIndicator(g_backBuffer, true);
            if (next >= 0) slot = next;
            next = drawWifiIndicator(g_backBuffer, slot, true);
            if (next >= 0) slot = next;
            if (g_volumeIndicatorVisible) drawKeyStateIndicator(g_backBuffer, slot, true);
        }
        flipScreen();
        if (g_pendingWindows.size() != 0) {
            TrimusWindow *pending = g_pendingWindows.back();
            pending->onShow();
            windows->push_back(pending);
            g_pendingWindows.pop_back();
            flushPendingWindows();
            continue;
        } else {
            count = 0;
            do {
                if ((result = window->handleInput()) >= 0) {
                    if ((result == WINDOW_CLOSE && windows->size() > 1) || result == WINDOW_FORCE_CLOSE) {
                        window = windows->back();
                        window->animateOut(g_backBuffer, flipScreen);
                        window->onHide();
                        printf("hide window:");
                        window->dump();
                        delete window;
                        puts("top windows is delete");
                        windows->pop_back();
                        break;
                    }
                    if (result == WINDOW_LAUNCH) {
                        window = windows->back();
                        if (window->getId() < 0) {
                            window->onHide();
                            delete window;
                            windows->pop_back();
                        }
                        break;
                    }
                }
                SDL_PumpEvents();
                count++;
            } while (SDL_PeepEvents(&event, 1, 1, ~0u) > 0);
            if (result == WINDOW_LAUNCH || result == WINDOW_QUIT) break;
        }
    }
    if (result == WINDOW_CLOSE) unlink("/tmp/cmd_to_run.sh");
    saveWindowState(windows, "/tmp/state.json");
    saveRomListPositions(&g_romListPositions);
    clearWindows(windows);
}
// 0x373c4: the root window: the main menu, titled "MIYOO".
void createRootWindow(std::vector<TrimusWindow *> *windows)
{
    ListMenu *menu = createMenu(WIN_MAIN, "");
    MenuWindow *window = new MenuWindow(157, WIN_MAIN, menu) /* 157: MenuWindow::draw titles it "MIYOO" */;
    window->onShow();
    windows->push_back((TrimusWindow *)window);
}
// 0x374bc: the title of a window id, or NULL.
const char *windowTitle(int id)
{
    switch (id) {
        case WIN_GAMES: return languageString(STR_GAME);
        case WIN_FAVORITES: return languageString(STR_FAVORITE);
        case WIN_APPS: return languageString(STR_APP);
        case WIN_FILES: return languageString(STR_FILE);
        case WIN_OTHER: return languageString(STR_OTHER);
        case WIN_SETTING: return languageString(STR_SETTING);
        case WIN_GAME_INFO: return languageString(STR_GAME_INFO_TITLE);
        case WIN_DEVICE_INFO: return languageString(STR_DEVICE_INFO_TITLE);
        case WIN_ADC_INFO: return languageString(STR_ADC_INFO_TITLE);
        case WIN_ENCDEC_TEST: return languageString(STR_ENCDEC_TEST);
        case WIN_RECENT: return languageString(STR_RECENT);
        case WIN_SEARCH: return languageString(STR_SEARCH);
        default: return NULL;
    }
}
// 0x37610: builds a ROM list behind the busy screen (the cache may need building first).
ListMenu *createRomListMenu(EmuInfo *emu, int id, std::string *dir, std::string *search)
{
    g_progressCur = 0;
    if (emu->path.size() != 0) {
        printf("load files in %s %s\n", emu->path.c_str(), dir->c_str());
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = emu->label.c_str();
        g_progressTotal = 0;
        g_progressCur = 0;
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        ListMenu *menu = buildRomList(emu, id, dir, search);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else
        return NULL;
}
// 0x37760: returns the theme's small font.
static TTF_Font *smallMenuFont()
{
    return g_smallFont;
}
// Onion patched twelve calls in createMenu to NOPs (mov r0, r0) after compiling, which disables
// six menu items (docs/onion-changes.md). This macro reproduces that: while createMenu is
// assembled, a `bl` at one of the listed offsets from the function start becomes the NOP, and
// every other call stays a normal call. The arguments are still set up, as in the original.
asm(R"(
.set .Lmainui_patched_calls, 0
.macro bl target
    .set .Lmainui_skip_call, 0
    .irp patch, 0x210, 0x21c, 0xa4c, 0xa58, 0xae4, 0xaf0, 0xb60, 0xb6c, 0xb98, 0xba4, 0xc08, 0xc14
        .if (. - _Z10createMenuiNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE) == \patch
            .set .Lmainui_skip_call, 1
        .endif
    .endr
    .if .Lmainui_skip_call
        mov r0, r0
        .set .Lmainui_patched_calls, .Lmainui_patched_calls + 1
    .else
        .inst 0xebfffffe
        .reloc . - 4, R_ARM_CALL, \target
    .endif
.endm
)");
// 0x37784: builds the list for a window id: the main menu, the games and RetroArch icons,
// favorites, apps, file browser, settings (and the fixes submenu), device info, hardware test,
// Recent, language, color and theme lists, or the ROM list of an emulator.
ListMenu *createMenu(int id, std::string path)
{
    printf("createMenu %d\n", id);
    if (id == WIN_MAIN) {
        IconMenu *menu = new IconMenu(1, 4, false, true, 10, 0, true);
        if (g_recentGames.size() != 0) menu->addItem(new IconMenuItem(STR_RECENT, "skin/ic-recent-f.png", "skin/ic-recent-n.png", 0, 0, WIN_RECENT, 0, false));
        menu->addItem(new IconMenuItem(STR_FAVORITE, "skin/ic-favorite-f.png", "skin/ic-favorite-n.png", 0, 0, WIN_FAVORITES, 0, false));
        menu->addItem(new IconMenuItem(STR_GAME, "skin/ic-game-f.png", "skin/ic-game-n.png", 0, 0, WIN_GAMES, 0, false));
        // MainUI-354-clean patches the following constructor and append calls to NOPs.
        menu->addItem(new IconMenuItem(STR_RETROARCH, "skin/ic-retroarch-f.png", "skin/ic-retroarch-n.png", 0, 0, WIN_RETROARCH, 0, false));
        menu->addItem(new IconMenuItem(STR_APP, "skin/ic-app-f.png", "skin/ic-app-n.png", 0, 0, WIN_APPS, 0, false));
        menu->addItem(new IconMenuItem(STR_SETTING, "skin/ic-setting-f.png", "skin/ic-setting-n.png", 0, 0, WIN_SETTING, 0, false));
        return menu;
    } else if (id == WIN_GAMES) {
        IconMenu *menu = new IconMenu(2, 4, false, false, 10, 10, false);
        menu->setLabelOffsetY(45);
        for (int i = 0; i < g_emuFolders.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_emuFolders[i].label.c_str(), g_emuFolders[i].iconSelected.c_str(), g_emuFolders[i].icon.c_str(), g_gameFocused, g_gameNormal, WIN_ROM_LIST, &g_emuFolders[i], false);
            item->verboseLabel = g_emuFolders[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == WIN_RETROARCH) {
        IconMenu *menu = new IconMenu(3, 3, true, false, 0, 0, false);
        menu->setIconCropSize(192, 72);
        menu->setFontFn(smallMenuFont);
        menu->setLabelOffsetY(28);
        for (int i = 0; i < g_rappFolders.size(); i++) {
            IconMenuItem *item = new IconMenuItem(g_rappFolders[i].label.c_str(), g_rappFolders[i].iconSelected.c_str(), g_rappFolders[i].icon.c_str(), g_raRow, 0, WIN_RETROARCH_LIST, &g_rappFolders[i], false);
            item->verboseLabel = g_rappFolders[i].labelVerbose;
            menu->addItem(item);
        }
        return menu;
    } else if (id == WIN_FAVORITES) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildFavoriteList("/mnt/SDCARD/Roms/favourite.json");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_APPS) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildAppList(&g_appFolder);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_FILES) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        FileBrowserMenu *menu = NULL;
        if (dualPartition() != true)
            menu = new FileBrowserMenu("/mnt/SDCARD");
        else
            menu = new FileBrowserMenu("/mnt/P2EXT");
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else if (id == WIN_SETTING) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_SETTING menu %p\n", menu);
        menu->addItem((TextItem *)new SettingMenuItem(STR_POWER_OFF, "skin/icon-Shutdown.png", SETTING_POWER_OFF));
        menu->addItem((TextItem *)new SettingMenuItem(STR_BRIGHTNESS, "skin/icon-brightness-48.png", SETTING_BRIGHTNESS));
        menu->addItem((TextItem *)new SettingMenuItem(STR_WIFI, "skin/icon-setting-wifi.png", SETTING_WIFI));
        menu->addItem((TextItem *)new SettingMenuItem(STR_COLOR, "skin/color.png", SETTING_COLOR));
        std::vector<LanguageChoice> *languages = languageChoices();
        if (languages->size() > 1) menu->addItem((TextItem *)new SettingMenuItem(STR_LANGUAGE, "skin/icon-language-48.png", SETTING_LANGUAGE));
        // Five pairs of calls below are disabled in the original; patch sites recorded separately.
        menu->addItem((TextItem *)new SettingMenuItem(STR_KEYS_MAP, "skin/icon-key-setting-48.png", SETTING_KEYMAP));
        menu->addItem((TextItem *)new SettingMenuItem(STR_BGM_VOLUME, "skin/sound-icon.png", SETTING_BGM_VOLUME));
        if (g_themeList.size() != 0) menu->addItem((TextItem *)new SettingMenuItem(STR_THEMES, "skin/icon-theme.png", SETTING_THEME));
        puts("insert hibernate item");
        menu->addItem((TextItem *)new SettingMenuItem(STR_HIBERNATE, "skin/icon-device-info-48.png", SETTING_HIBERNATE));
        menu->addItem((TextItem *)new SettingMenuItem(STR_FIXES, "skin/fixit.png", SETTING_FIXES));
        menu->addItem((TextItem *)new SettingMenuItem(STR_FACTORY_RESET, "skin/icon-factory-reset-48.png", SETTING_FACTORY_RESET));
        menu->addItem((TextItem *)new SettingMenuItem(STR_DEVICE_INFO, "skin/icon-device-info-48.png", SETTING_DEVICE_INFO));
        menu->addItem((TextItem *)new SettingMenuItem(STR_HARDWARE_TEST, "skin/icon-device-info-48.png", SETTING_IO_TEST));
        return menu;
    } else if (id == WIN_FIXES) {
        SettingMenu *menu = new SettingMenu();
        printf("create WIN_FIXES menu %p\n", menu);
        menu->addItem((TextItem *)new SettingMenuItem(STR_AUDIO_FIX, NULL, SETTING_AUDIOFIX));
        return menu;
    } else if (id == WIN_DEVICE_INFO) {
        DeviceInfoMenu *menu = new DeviceInfoMenu();
        return menu;
    } else if (id == WIN_HARDWARE_TEST) {
        HardwareTestMenu *menu = new HardwareTestMenu();
        return menu;
    } else if (id == WIN_RECENT) {
        g_busy = 1;
        BusyData data;
        data.type = id;
        data.title = windowTitle(id);
        SDL_Thread *thread = SDL_CreateThread(busyScreenThread, &data);
        TextMenu *menu = buildRecentList(&g_recentGames);
        g_busy = 0;
        SDL_WaitThread(thread, NULL);
        return menu;
    } else {
        if (id == WIN_LANGUAGE) {
            std::vector<LanguageChoice> *languages = languageChoices();
            if (languages->size() > 1) {
                LanguageMenu *menu = new LanguageMenu(*languages);
                return menu;
            }
        } else {
            if (id == WIN_COLOR) {
                ColorMenu *menu = new ColorMenu();
                return menu;
            }
            if (id == WIN_THEME) {
                ThemeMenu *menu = new ThemeMenu();
                return menu;
            }
        }
        return NULL;
    }
}

asm(R"(
.purgem bl
.if .Lmainui_patched_calls != 12
    .error "createMenu must retain all twelve MainUI-354-clean patched calls"
.endif
)");
// 0x38834: make sure there is swap space (needs the SD card).
int enableSwapAction(void *arg)
{
    if (sdCardMounted()) enableSwap();
    return 1;
}
// 0x38868: show the "LOADING" animation on a thread during start-up.
void startBackgroundThread()
{
    g_busy = 1;
    g_loadingThread = SDL_CreateThread(loadingScreenThread, NULL);
}
// 0x388a8: stop it.
void stopBackgroundThread()
{
    g_busy = 0;
    SDL_WaitThread(g_loadingThread, NULL);
    g_loadingThread = NULL;
}
// 0x388f0: at start-up: enable the swap file if no swap is active.
void ensureSwap(void *arg)
{
    int elapsed = 0;
    timeval now;
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec * 1000 + now.tv_usec / 1000;
    sync();
    gettimeofday(&now, NULL);
    elapsed = now.tv_sec * 1000 + now.tv_usec / 1000 - elapsed;
    int size = swapSize();
    printf("swap size is %d take %dms\n", size, elapsed);
    if (size < 1) enableSwapAction(NULL);
}
// 0x389d4: charging screen thread: the battery picture (five levels, animated upwards from the
// current one) and "Charging..."; powers off if the charger is unplugged.
static int chargingThread(void *arg)
{
    SDL_FillRect(g_backBuffer, NULL, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
    int frame = 0;
    int current = 0;
    int previous = 0;
    int count = 5;
    int battery;
    int voltage;
    int state;
    SDL_Rect imageRect;
    SDL_Rect textRect;
    SDL_Surface *images[5];
    images[0] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-01.png", g_backBuffer, false);
    images[1] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-02.png", g_backBuffer, false);
    images[2] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-03.png", g_backBuffer, false);
    images[3] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-04.png", g_backBuffer, false);
    images[4] = loadSkinImage(g_themePath, g_skinSubdir, "skin/charging-05.png", g_backBuffer, false);
    imageRect.x = 224;
    imageRect.y = 192;
    imageRect.w = 192;
    imageRect.h = 96;
    textRect.w = 640;
    textRect.h = TTF_FontHeight(g_iconFont);
    textRect.x = 0;
    textRect.y = imageRect.y + 20;
    while (g_charging) {
        readAxpStatus(&battery, &voltage, &state);
        if (state != 2 && state != 3) {
            puts("usb unplug, poweroff");
            system("sync");
            system(g_powerOffCommand);
        }
        if (battery >= 99)
            current = 4;
        else if (battery >= 75)
            current = 3;
        else if (battery >= 50)
            current = 2;
        else if (battery >= 25)
            current = 1;
        else
            current = 0;
        if (previous != current) {
            previous = current;
            frame = current;
        }
        if (images[frame % 5]) {
            SDL_FillRect(g_backBuffer, &imageRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            imageRect.w = images[frame % 5]->w;
            imageRect.h = images[frame % 5]->h;
            imageRect.x = (640 - imageRect.w) / 2;
            imageRect.y = (480 - imageRect.h) / 2;
            SDL_UpperBlit(images[frame % 5], NULL, g_backBuffer, &imageRect);
            textRect.y = imageRect.y + imageRect.h + 20;
            SDL_FillRect(g_backBuffer, &textRect, SDL_MapRGB(g_backBuffer->format, 0, 0, 0));
            drawText(g_backBuffer, g_iconFont, textRect, "Charging...", g_wifiEmptyColor, 1, false);
            SDL_UpperBlit(g_backBuffer, NULL, g_screen, NULL);
            SDL_UpdateRect(g_screen, 0, 0, 0, 0);
        }
        usleep(150000);
        frame++;
        if (frame > 4) frame = current;
    }
    SDL_FreeSurface(images[0]);
    SDL_FreeSurface(images[1]);
    SDL_FreeSurface(images[2]);
    SDL_FreeSurface(images[3]);
    SDL_FreeSurface(images[4]);
    return 0;
}
// 0x38ff8: the charging screen, shown instead of the menus when the device was started by plugging
// in a charger. Holding power for over half a second reboots into the normal menus.
void chargingScreenLoop()
{
    g_charging = true;
    SDL_Thread *thread = SDL_CreateThread(chargingThread, NULL);
    g_powerSent = false;
    system("touch /tmp/battery_charging");
    SDL_Event event;
    while (true) {
        if (SDL_WaitEvent(&event) != 0) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case MIYOO_KEY_POWER:
                            g_powerDownCount++;
                            printf("SDLK_POWER down %d, sent %d\n", g_powerDownCount, g_powerSent);
                            if ((g_powerDownCount - 1) * 100 > 500 && g_powerSent != true) {
                                g_powerSent = true;
                                system("sync");
                                system("reboot");
                            }
                            break;
                        default:
                            printf("event.key.keysym.sym = %d\n", event.key.keysym.sym);
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case MIYOO_KEY_POWER:
                            g_powerDownCount = 0;
                            g_powerSent = false;
                            break;
                        default: break;
                    }
                default: break;
            }
        }
    }
}
// The compiler emits the original static-init pair here (0x3919c and 0x39228).
