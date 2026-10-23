// tu_015b20 part p04 (0x1d9d8-0x20500): ListMenu navigation and TextMenuDrawer drawing.
#include <sys/time.h>   // gettimeofday (no inline functions, so it does not move the tail)

extern SDL_Color g_highlightColor;     // 0x18228c (only drawListNumber uses it)
extern SDL_Color g_accentColor;         // 0x1822b0 (search match in a list row)

SDL_Surface *loadImage(const char *path, SDL_Surface *fmt, bool flag); // 0x123a38 (tu_11cdc8)
int pinyinFind(const char *text, const char *initials, const char *key, const char **start,
               int *bytes);                                     // 0x143cf4 (tu_143720)

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
