// tu_015b20 part p03 (0x1a44c-0x1d9d8): favorites, Recent and app lists, ROM deletion, the window base.

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
#include "../PixelScale.h"

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
