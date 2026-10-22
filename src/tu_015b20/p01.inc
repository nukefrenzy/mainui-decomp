// p01 0x15b20-0x18c54: runtime helpers, handleGlobalInput, launch, writeCmdToRun; EmuAction
// Functions in address order; see docs/plan-tu_015b20.md.

// 0x15b20: name of a key or event code (Ghidra missed this start; no caller in the binary).
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

// 0x15d00
void flipDisplay()
{
    SDL_Flip(g_screen);
}

// 0x15d24: draws `text` in `rect` (align 0 left, 1 centred, 3 vertically centred, 4 right and
// vertically centred, else right), optionally over a shadow; returns the text width. The shadow blit
// is a NOP in this build.
// Preserve the target binary's post-build NOP patch(es), including argument setup.
// Only these function-relative BL sites lose their relocation; other calls are unchanged.
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

// 0x1622c: blits `bg` at `rect` and `icon` centred on it (or centred in 48x48 without `bg`).
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

// 0x16450: blits `bg` at `rect` and centres `text` on it.
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
    int v = GetKeyShm(g_keyShm, 0);
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
// are NOPs in this build.
// Preserve the target binary's post-build NOP patch(es), including argument setup.
// Only these function-relative BL sites lose their relocation; other calls are unchanged.
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
int playEnterSound(int a, int b)
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
    drawFooter(dst, languageString(88), languageString(89));
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

// 0x176a8
void powerOffNow()
{
    powerOff(0);
}

// 0x176c0: asks the window loop to power off (user event 21).
int sendPowerOffEvent(int a, int b)
{
    MainEvent event;
    event.type = 24;
    event.user.code = 21;
    puts("send poweroff event");
    if (SDL_PushEvent(&event) < 0)
        puts("Failed to push poweroff SDL event");
    return 0;
}

// 0x1772c: called for every event except status notifications; empty in this build.
void onUserActivity()
{
}

// 0x17744: waits for one SDL event, copies it to `out` and maps it to a key code (1 A, 2 B,
// 3 X, 4 Y, 5-8 up/down/left/right, 9 select, 10 start, 11 menu, 12/13 L/R, 14/15 L2/R2, ...),
// handling power-key repeats, audio device changes and power-off requests; returns user event codes as is.
int handleGlobalInput(SDL_Event *out)
{
    SDL_Event event;
    if (SDL_WaitEvent(&event)) {
        if (event.type != 24 || (event.user.code != 18 && event.user.code != 31 &&
                                 event.user.code != 40 && event.user.code != 41 &&
                                 event.user.code != 0))
            onUserActivity();
        if (out != NULL)
            *out = event;
        switch (event.type) {
        case 2:
            if (event.key.keysym.sym != 320 && event.key.keysym.sym != 116 &&
                event.key.keysym.sym != 101 && g_changeSound != NULL) {
                if (Mix_PlayChannel(1, g_changeSound, 0) == -1)
                    printf("Mix_PlayChunk: %s\n", SDL_GetError());
            }
            printf("%lu keydown event %d\n", milliseconds(), event.key.keysym.sym);
            switch (event.key.keysym.sym) {
            case 273:
                return 5;
            case 274:
                return 6;
            case 276:
                return 7;
            case 275:
                return 8;
            case 305:
                g_selectHeld = true;
                break;
            case 13:
                g_startHeld = true;
                break;
            case 9:
                if (!g_startHeld && !g_selectHeld)
                    return 12;
                break;
            case 8:
                if (!g_startHeld && !g_selectHeld)
                    return 13;
                break;
            case 101:
                return 14;
            case 116:
                return 15;
            case 320:
                g_powerDownCount++;
                printf("SDLK_POWER down %d, sent %d\n", g_powerDownCount, g_powerSent);
                if ((g_powerDownCount - 1) * 100 > 1000 && !g_powerSent) {
                    g_powerSent = true;
                    TrimusWindow *w = new DialogWindow(86, 108, false,
                                                       new GeneralAction(-1, powerOff, NULL));
                    pushWindow(w);
                    return 27;
                }
                break;
            default:
                printf("event.key.keysym.sym = %d\n", event.key.keysym.sym);
                break;
            }
            break;
        case 3:
            switch (event.key.keysym.sym) {
            case 27:
                return 11;
            case 308:
                return 4;
            case 32:
                return 1;
            case 306:
                return 2;
            case 304:
                return 3;
            case 13:
                g_startHeld = false;
                puts("return KEY_START");
                return 10;
            case 305:
                g_selectHeld = false;
                puts("return KEY_SELECT");
                return 9;
            case 320:
                g_powerDownCount = 0;
                g_powerSent = false;
                break;
            case 311:
                break;
            case 312:
                break;
            default:
                break;
            }
            break;
        case 24:
            if (event.user.code == 19) {
                setVolumeTinymix(GetKeyShm(g_keyShm, 0));
                Mix_CloseAudio();
                if (hasDsp1(2) > 0)
                    putenv((char *)"AUDIODEV=/dev/dsp1");
                else
                    putenv((char *)"AUDIODEV=/dev/dsp");
                if (Mix_OpenAudio(48000, 0x8010, 2, 128) < 0) {
                    printf("open auido fail: %s\n", SDL_GetError());
                    Mix_PauseMusic();
                } else {
                    Mix_PlayMusic(g_themeMusic, -1);
                }
            } else if (event.user.code == 21) {
                TrimusWindow *w = new CountDownWindow(87, powerOffNow, onUserActivity);
                pushWindow(w);
            } else if (event.user.code == 40) {
                g_statusIndicatorFlag = true;
            } else if (event.user.code == 41) {
                g_statusIndicatorFlag = false;
            }
            return event.user.code;
        case 6:
            if (event.button.x < 100 && event.button.y > 100 && event.button.y < 380)
                return 7;
            if (event.button.x > 540 && event.button.y > 100 && event.button.y < 380)
                return 8;
            if (event.button.y < 100 && event.button.x > 100 && event.button.x < 540)
                return 5;
            if (event.button.y > 380 && event.button.x > 100 && event.button.x < 540)
                return 6;
        default:
            break;
        }
    }
    return 0;
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

// 0x17fc0
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
    drawText(g_backBuffer, g_imeFont, r, languageString(75), g_wifiEmptyColor, 1, false);
    SDL_BlitSurface(g_backBuffer, NULL, g_screen, NULL);
    SDL_Flip(g_screen);
    SDL_BlitSurface(g_backBuffer, NULL, g_screen, NULL);
    g_fbVar.yoffset = 0;
    if (g_fbFd >= 0)
        ioctl(g_fbFd, FBIOPAN_DISPLAY, &g_fbVar);
    writeCmdToRun(cmd);
}

// 0x18170
int donoting_action(void *a, void *b)
{
    puts(__func__);
    return 1;
}

// 0x181a0: builds the ROM launch command (through libpadsp.so unless the audio fix is off),
// shows the launch screen and records the game in the recent list; returns 3 (exit MainUI), or -1
// after opening the swap dialog when the emulator needs swap and none is enabled.
int launchRom(const char *label, EmuInfo *emu, const char *rompath, const char *imgpath, int type)
{
    bool audioFix = GetKeyShm(g_keyShm, 13) != 0;
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
            ProgressDialog *d = new ProgressDialog(85, new GeneralAction(-1, enableSwapAction, NULL));
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
    bool audioFix = GetKeyShm(g_keyShm, 13) != 0;
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
        w->setStrB4(folderArg);
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
