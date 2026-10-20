"""Hand-verified names (from reading decompiled code) merged over auto names; writes names.tsv.

Usage: python3 -I manual_names.py <out_dir>
confidence: confirmed = read and verified in code; inferred = strong evidence, not fully traced.
"""
import json
import sys

out = sys.argv[1]
names = json.load(open(f"{out}/names.json"))
classes = json.load(open(f"{out}/classes.json"))

M = {
    "00015b20": ("keyEventName", "input", "confirmed", "Byte-exact key/event code to name switch (key_none..key_hotkey4, event_battery/hotplug/notify, key_max_key); no caller; missed by Ghidra export."),
    "00018c94": ("AppAction::progress", "actions", "confirmed", "Byte-exact Action slot 3: opens GameDetailWindow (0x9c) for the selected TextItem when the detail flag is set; 0 or -1."),
    "00018dd4": ("digitCount", "render", "confirmed", "Byte-exact decimal digit count; field width for the busy animation's cur/total text."),
    "00018e3c": ("animateBusy", "render", "confirmed", "Byte-exact four-dot busy animation (28 ms frames, step every 5) with optional cur/total counter until g_busy (volatile int 0x181908) clears."),
    "000193a8": ("busyScreenThread", "render", "confirmed", "Byte-exact SDL thread body: window frame, status bar, dimmed screen, then pop-bg popup with animateBusy; arg {title, type}."),
    "000198a8": ("loadingScreenThread", "render", "confirmed", "Byte-exact SDL thread body: background, text 0x4b, animateBusy below it."),
    "0004c418": ("ringAt", "container", "confirmed", "Byte-exact indexed access to a circular singly linked list; missed by Ghidra export."),
    "0004c7a8": ("ringCount", "container", "confirmed", "Byte-exact circular-list count; missed by Ghidra export."),
    # program entry and lifecycle
    "0004a2fc": ("main", "core", "confirmed", "Startup: governor, wifi driver, SDL/TTF/audio, 640x480 video, key shm, theme, charging UI, threads, folders, state restore, then runWindowLoop."),
    "00036ee8": ("runWindowLoop", "core", "confirmed", "Main loop over the window stack: draw top window + status bar, flip, dispatch input; result codes 1/4 close window, 3 close if id<0, 2 quit. Saves /tmp/state.json on exit."),
    "000373c4": ("createRootWindow", "core", "confirmed", "Builds the main menu (createMenu(0)) inside MenuWindow id 0x9d and pushes it."),
    "00037784": ("createMenu", "core", "confirmed", "Menu factory: createMenu(menuId, path). 0 main IconMenu, 1 games, 2 favourites, 3 apps, 4 file browser, 7 settings, 9 device info, 10 recent, 0xb hardware test, 0xe language, 0x12 color, 0x13 theme, 0x14 fixes, 0x10 icon menu."),
    "00036d88": ("pushWindow", "core", "confirmed", "Queue a window; runWindowLoop moves it onto the stack next pass."),
    "00036cb8": ("flushPendingWindows", "core", "inferred", "Operates on the pending-window queue at 0x17ff3c."),
    "0003919c": ("staticInit_globals", "core", "confirmed", "iostream init + pending-window queue vector."),
    "00049c3c": ("onSIGTERM", "core", "confirmed", "SIGTERM handler: clears the running flag (0x181b0c)."),
    "0004aec0": ("atomicBool_store", "core", "inferred", ""),
    "0003937c": ("atomicBool_load", "core", "confirmed", "Returns the running flag."),
    "00049c68": ("appExit", "core", "confirmed", "Close audio/TTF/SDL, trimui_show=no, exit(code)."),
    # window stack (std::vector<TrimusWindow*>)
    "0003b790": ("windowVec_size", "core", "inferred", ""),
    "0003cea8": ("windowVec_back", "core", "inferred", ""),
    "0003cf00": ("windowVec_popBack", "core", "inferred", ""),
    "0003cf78": ("windowVec_pushBack", "core", "confirmed", ""),
    "0003b2d8": ("windowVec_emplaceBack", "core", "confirmed", ""),
    "000394c8": ("window_getId", "core", "confirmed", "Returns window->id (+8)."),
    # rendering / status bar
    "00036c20": ("flipScreen", "render", "confirmed", "Blit back buffer (0x181928) to screen (0x181924)."),
    "00016b54": ("statusBar_drawSoundCard", "render", "inferred", ""),
    "00016c98": ("statusBar_drawIndicatorA", "render", "inferred", "Status-bar icon drawn right-to-left; returns next x."),
    "00016788": ("statusBar_drawIndicatorB", "render", "inferred", ""),
    "0001656c": ("statusBar_drawIndicatorC", "render", "inferred", "Only drawn when flag 0x180907 set; reads key shm."),
    "00036dbc": ("initFramebuffer", "display", "confirmed", "/dev/fb0 ioctl setup."),
    # state persistence
    "0001c1fc": ("restoreWindowState", "config", "confirmed", "Reads /tmp/state.json, recreates saved windows."),
    "0001c544": ("saveWindowState", "config", "confirmed", "Writes open windows to /tmp/state.json."),
    "0001c994": ("loadRomListPositions", "config", "confirmed", "Reads /appconfigs/romwinidx.json (per-list selection)."),
    "0001cc90": ("saveRomListPositions", "config", "inferred", ""),
    # threads and background polling
    "0001d7dc": ("TrimusWindow::TrimusWindow", "ui_framework", "confirmed", "(this, type@+4, id@+8): sets vtable 0x15402c."),
    "0001d830": ("TrimusWindow::~TrimusWindow", "ui_framework", "confirmed", "Base destructor body: restores vtable 0x15402c."),
    "00039478": ("TrimusWindow::animateIn", "ui_framework", "confirmed", "Default: does nothing (surface, flipFn)."),
    "000394a0": ("TrimusWindow::animateOut", "ui_framework", "confirmed", "Default: does nothing (surface, flipFn)."),
    "001323e0": ("MenuHelper60::MenuHelper60", "ui_framework", "confirmed", "32-byte MenuWindow member at +0x60, constructed with 2."),
    "0013249c": ("MenuHelper60::~MenuHelper60", "ui_framework", "confirmed", ""),
    "00131e34": ("MenuHelper80::MenuHelper80", "ui_framework", "confirmed", "24-byte MenuWindow member at +0x80, takes a surface-free callback."),
    "00131e94": ("MenuHelper80::~MenuHelper80", "ui_framework", "confirmed", ""),
    "00123af0": ("loadSkinImage", "render", "inferred", "(themePath, g_skinSubdir, file, fmtSurface, flags) -> SDL_Surface*"),
    "0002e318": ("freeSurfaceCallback", "render", "inferred", "Wraps SDL_FreeSurface; passed to MenuHelper80."),
    "0004a050": ("statusPollThread", "threads", "confirmed", "Every 1s: poll battery/wifi/volume and push SDL user events."),
    "00049cc0": ("pollStatusA_pushEvent", "threads", "inferred", "Reads key shm, pushes SDL event."),
    "00049e24": ("pollStatusB_pushEvent", "threads", "inferred", ""),
    "00049f24": ("pollVolume_pushEvent", "threads", "confirmed", "Volume change -> SDL event + key shm."),
    "0004a170": ("hotplugThread", "threads", "confirmed", ""),
    "0004a108": ("pushHotplugEvent", "threads", "confirmed", ""),
    "0014f634": ("startWifiMonitor", "wifi", "confirmed", ""),
    "0004a19c": ("wifiStatusCallback", "wifi", "confirmed", "udhcpc / wpa_supplicant status handling."),
    # input, audio, misc startup
    "00017744": ("handleGlobalInput", "input", "inferred", "Key events, power key, START/SELECT, key sounds."),
    "0001bf90": ("initAudioSettings", "audio", "inferred", "Restores music volume; button-name table L2,L,R2,R,X,A,B,Y."),
    "00038ff8": ("chargingScreenLoop", "power", "confirmed", "Shown when powered on by charger; power key -> reboot."),
    "00038868": ("startBackgroundThread", "threads", "inferred", ""),
    "00143920": ("loadPinyinTable", "games", "confirmed", "./py.dat for Chinese pinyin search."),
    # launching
    "00017fc0": ("writeCmdToRun", "launch", "confirmed", "Writes /tmp/cmd_to_run.sh; MainUI then exits so the shell runs it."),
    "00018560": ("launchApp_buildCommand", "launch", "confirmed", "cd dir; chmod a+x; LD_PRELOAD=libpadsp.so <launch> <args>."),
    # menus and windows
    "0002e6d4": ("MenuWindow::MenuWindow", "ui_framework", "confirmed", "(titleLabelId, id, ListMenu*). Byte-exact rewrite in src/tu_015b20/p09.inc."),
    "0002e424": ("MenuWindow::MenuWindow_byMenuId", "ui_framework", "confirmed", "(labelId, menuId, param)"),
    "0002e978": ("MenuWindow::MenuWindow_byPath", "ui_framework", "confirmed", "(path, menuId, param)"),
    "0002ec3c": ("MenuWindow::~MenuWindow", "ui_framework", "confirmed", "Byte-exact rewrite in src/tu_015b20/p09.inc."),
    "0002f290": ("MenuWindow::onShow", "ui_framework", "confirmed", "Lazily builds its ListMenu: 0x18 search, 5/0x11 DB rom list, else createMenu."),
    "00027724": ("IconMenu::onSelect", "ui_framework", "confirmed", "Opens a MenuWindow for item->menuId (+0x4c); item 0x18 = global search."),
    "000256f0": ("IconMenuItem::IconMenuItem", "ui_framework", "confirmed", "(labelId@+0x40, iconFocused, iconNormal, x, y, menuId@+0x4c, param@+0x44, flag)"),
    "000282d0": ("SettingMenuItem::SettingMenuItem", "settings", "inferred", "(labelId, icon, handler/target)"),
    "00027a30": ("itemVec_pushBack", "ui_framework", "inferred", ""),
    "001208a0": ("loadRecentList", "games", "confirmed", "Reads /mnt/SDCARD/Roms/recentlist.json (label, rompath, launch, imgpath)."),
    "00120af4": ("freeRecentList", "games", "inferred", ""),
    "00134800": ("loadLanguages", "language", "confirmed", "Scans ./lang/ for *.lang, default English."),
    "00122470": ("loadEmuFolder", "launch", "confirmed", "Scans a folder of <name>/config.json entries (Emu, RApp)."),
    "00121958": ("loadAppFolder", "launch", "confirmed", "Scans App/<name>/config.json."),
    "00037610": ("createRomListMenu", "games", "inferred", "DB-cached ROM list for menu ids 5/0x11."),
    # Function starts Ghidra missed (no callers); byte-exact in src/tu_14acfc.c and src/tu_14e0cc.c
    "0014acfc": ("stub_4acfc", "device", "confirmed", "Returns 0. Byte-exact (src/tu_14acfc.c)."),
    "0014ad34": ("stub_4ad34", "device", "confirmed", "Returns 0. Byte-exact (src/tu_14acfc.c)."),
    "0014ade8": ("stub_4ade8", "device", "confirmed", "Returns 2. Byte-exact (src/tu_14acfc.c)."),
    "0014e584": ("hexDigit", "wifi", "confirmed", "0-15 to '0'-'9'/'A'-'F', else 0xff. Byte-exact (src/tu_14e0cc.c)."),
}

# Virtual slot meanings, from runWindowLoop / callers. Byte offset = slot*4.
WINDOW_SLOTS = {0: "dtor", 1: "deletingDtor", 2: "handleInput", 3: "draw", 4: "onHide",
                5: "onShow", 6: "saveState", 7: "restoreState", 8: "update", 9: "animateIn", 10: "animateOut"}
ACTION_SLOTS = {0: "dtor", 1: "deletingDtor", 2: "execute"}
LISTMENU_SLOTS = {0: "dtor", 1: "deletingDtor", 2: "restorePosition", 4: "onSelect", 5: "getItem"}


def family(c):
    seen = c
    while classes.get(seen, {}).get("bases"):
        seen = classes[seen]["bases"][0]
    return seen


def depth(c):
    d = 0
    while classes.get(c, {}).get("bases"):
        c = classes[c]["bases"][0]
        d += 1
    return d


# Methods that exist only in vtables (no Ghidra function yet): name them after the most-base owner
owner = {}
for cname in sorted(classes, key=depth):
    for vt in classes[cname]["vtables"]:
        for i, e in enumerate(vt["entries"]):
            if e and e != "pure":
                owner.setdefault("%08x" % int(e, 16), (cname, i))
for a, (cname, i) in owner.items():
    if a not in names:
        names[a] = {"name": f"{cname}::vfunc{i}", "subsystem": "ui_framework", "confidence": "confirmed-class"}

WINDOW_SLOTS.update({6: "saveState", 7: "restoreState"})

for cname, c in classes.items():
    root = family(cname)
    slots = {"TrimusWindow": WINDOW_SLOTS, "Action": ACTION_SLOTS, "ListMenu": LISTMENU_SLOTS}.get(root, {0: "dtor", 1: "deletingDtor"})
    for vt in c["vtables"]:
        for i, e in enumerate(vt["entries"]):
            if not e or e == "pure":
                continue
            a = "%08x" % int(e, 16)
            cur = names.get(a, {})
            if not cur.get("name", "").startswith(cname + "::vfunc"):
                continue  # implementation inherited from (and named under) a base class
            if i in slots:
                conf = "confirmed" if (root, i) in {("TrimusWindow", 2), ("TrimusWindow", 3), ("TrimusWindow", 5), ("TrimusWindow", 6), ("TrimusWindow", 7), ("Action", 2), ("ListMenu", 4)} or i < 2 else "inferred"
                names[a] = {"name": f"{cname}::{slots[i]}", "subsystem": cur.get("subsystem", ""), "confidence": conf}

# Exact library names: these objects were rebuilt and matched byte-for-byte at fixed addresses
import os
if os.path.exists(f"{out}/libnames.tsv"):
    for line in open(f"{out}/libnames.tsv").read().splitlines()[1:]:
        a, nm, lib = line.split("\t")
        names[a] = {"name": nm, "subsystem": "lib_" + lib.split("-")[0].lower(), "confidence": "library-exact",
                    "comment": f"{lib}, verified byte-identical with -O0 -fsigned-char"}

# Hardware-test names verified by whole-unit proof.
M.update({'0004cb68': ('readTestInput', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004cce4': ('SDLKeyTestWindow::SDLKeyTestWindow',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004cd88': ('SDLKeyTestWindow::handleInput',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004cebc': ('SDLKeyTestWindow::keyName', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004cfb4': ('drawHighlightedText', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d53c': ('SDLKeyTestWindow::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d6f8': ('testKeyTimer', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d7cc': ('SDLKeyTestWindow::restartTimer',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004d828': ('SDLKeyTestWindow::onHide', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d870': ('SDLKeyTestWindow::onShow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d894': ('TestKeyImage::TestKeyImage', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d93c': ('TestKeyImage::~TestKeyImage', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004d99c': ('KeyTestWindow::KeyTestWindow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004db34': ('KeyTestWindow::keyName', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004dc44': ('KeyTestWindow::expecting', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004dc98': ('KeyTestWindow::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004e1c4': ('KeyTestWindow2::KeyTestWindow2',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004e230': ('KeyTestWindow2::~KeyTestWindow2',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004e3a0': ('KeyTestWindow2::loadMasks', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004e5dc': ('KeyTestWindow2::mapKey', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004e774': ('KeyTestWindow2::handleInput', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004e8e8': ('keyMask', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004e9bc': ('keyMaskRect', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004edf8': ('drawKeyTestFooter', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004ef30': ('KeyTestWindow2::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f10c': ('LCDTestWindow::LCDTestWindow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f1cc': ('LCDTestWindow::handleInput', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f2fc': ('LCDTestWindow::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f444': ('LCDTestWindow::onHide', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f464': ('LCDTestWindow::onShow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f484': ('SoundTestWindow::SoundTestWindow',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004f4e0': ('SoundTestWindow::handleInput', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f588': ('SoundTestWindow::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f7f0': ('soundTestTimer', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f860': ('SoundTestWindow::playNext', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004f978': ('SoundTestWindow::onHide', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004fa44': ('SoundTestWindow::onShow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004fb64': ('readUsbTestInput', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004fbd4': ('SDCardTestWindow::SDCardTestWindow',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004fc78': ('SDCardTestWindow::handleInput',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0004fd2c': ('SDCardTestWindow::draw', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0004ffd8': ('SDCardTestWindow::testWrite', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0005020c': ('SDCardTestWindow::testRead', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '000504a8': ('SDCardTestWindow::onHide', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '000504e8': ('sdCardTestThread', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '0005052c': ('SDCardTestWindow::onShow', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050564': ('TestMenuItem::TestMenuItem', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050624': ('TestMenuItem::~TestMenuItem', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050674': ('HardwareTestMenu::HardwareTestMenu',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '000507d4': ('HardwareTestMenu::~HardwareTestMenu',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.'),
 '0005084c': ('HardwareTestMenu::vfunc4', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050ce8': ('testResultLabel', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050d40': ('testResultColor', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00050dd4': ('HardwareTestMenu::vfunc7', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00051390': ('HardwareTestMenu::vfunc18', 'hardware', 'confirmed', 'Byte-exact tu_04cb68 function.'),
 '00051454': ('HardwareTestMenu::initializeTests',
              'hardware',
              'confirmed',
              'Byte-exact tu_04cb68 function.')})

for a, (nm, sub, conf, cmt) in M.items():
    names[a] = {"name": nm, "subsystem": sub, "confidence": conf, "comment": cmt}

json.dump(names, open(f"{out}/names.json", "w"), indent=0)
with open(f"{out}/names.tsv", "w") as fh:
    fh.write("addr\tname\tsubsystem\tconfidence\tcomment\n")
    for a in sorted(names):
        v = names[a]
        fh.write(f"{a}\t{v['name']}\t{v['subsystem']}\t{v['confidence']}\t{v.get('comment', '')}\n")
import collections
print(collections.Counter(v["confidence"] for v in names.values()))
