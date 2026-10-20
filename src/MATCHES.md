# Byte-exact functions

Each compiles with Arm GNU Toolchain 8.2-2018.08, `-O0 -fsigned-char`, to the same bytes as
`MainUI-354-clean`. Only link-time words are ignored. CI re-checks all of them
(`scripts/run_matches.sh`). The list itself is `src/matches.tsv`; regenerate this table with
`python3 tools/progress.py --matches-md src/MATCHES.md`.

| Function | Address | Size | File |
| --- | --- | --- | --- |
| `keyEventName(int)` | 0x15b20 | 480 | tu_015b20.cpp |
| `flipScreen()` | 0x15d00 | 36 | tu_015b20.cpp |
| `drawText(SDL_Surface*, void*, SDL_Rect, char const*, SDL_Color, int, bool)` | 0x15d24 | 756 | tu_015b20.cpp |
| `drawTextLines(SDL_Surface*, void*, SDL_Rect*, char const*, char const*, SDL_Color*, int, bool)` | 0x16018 | 532 | tu_015b20.cpp |
| `drawIconOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, SDL_Surface*)` | 0x1622c | 420 | tu_015b20.cpp |
| `drawSkinIconOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, char const*)` | 0x163d0 | 128 | tu_015b20.cpp |
| `drawTextOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, char const*)` | 0x16450 | 284 | tu_015b20.cpp |
| `drawKeyStateIndicator(SDL_Surface*, int, bool)` | 0x1656c | 540 | tu_015b20.cpp |
| `drawWifiIndicator(SDL_Surface*, int, bool)` | 0x16788 | 972 | tu_015b20.cpp |
| `drawSoundCardIndicator(SDL_Surface*, bool)` | 0x16b54 | 324 | tu_015b20.cpp |
| `drawBatteryIndicator(SDL_Surface*, bool)` | 0x16c98 | 584 | tu_015b20.cpp |
| `playEnterSound(int, int)` | 0x16ee0 | 136 | tu_015b20.cpp |
| `drawTitleBar(SDL_Surface*, char const*, bool)` | 0x16f68 | 400 | tu_015b20.cpp |
| `drawIconLabel(SDL_Surface*, SDL_Surface*, int, int, char const*, char const*)` | 0x170f8 | 424 | tu_015b20.cpp |
| `drawFooter(SDL_Surface*, char const*, char const*)` | 0x172a0 | 432 | tu_015b20.cpp |
| `drawBackground(SDL_Surface*)` | 0x17450 | 280 | tu_015b20.cpp |
| `drawScreenFrame(SDL_Surface*, char const*, int, bool)` | 0x17568 | 180 | tu_015b20.cpp |
| `powerOff(void*)` | 0x1761c | 140 | tu_015b20.cpp |
| `powerOffNow()` | 0x176a8 | 24 | tu_015b20.cpp |
| `sendPowerOffEvent(int, int)` | 0x176c0 | 108 | tu_015b20.cpp |
| `onUserActivity()` | 0x1772c | 24 | tu_015b20.cpp |
| `handleGlobalInput(SDL_Event*)` | 0x17744 | 1996 | tu_015b20.cpp |
| `compareDirentNames(dirent const**, dirent const**)` | 0x17f10 | 80 | tu_015b20.cpp |
| `compareLabels(void const*, void const*)` | 0x17f60 | 96 | tu_015b20.cpp |
| `writeCmdToRun(const char*)` | 0x17fc0 | 88 | tu_015b20.cpp |
| `showLaunchScreen(char const*)` | 0x18018 | 344 | tu_015b20.cpp |
| `doNothingAction(void*, void*)` | 0x18170 | 48 | tu_015b20.cpp |
| `launchRom(char const*, EmuInfo*, char const*, char const*, int)` | 0x181a0 | 960 | tu_015b20.cpp |
| `launchApp_buildCommand(AppInfo*, int)` | 0x18560 | 1052 | tu_015b20.cpp |
| `EmuAction::execute(void*)` | 0x1897c | 388 | tu_015b20.cpp |
| `EmuAction::progress(int*, int*)` | 0x18b00 | 340 | tu_015b20.cpp |
| `TrimusWindow::TrimusWindow(int titleLabelId, int id)` | 0x1d7dc | 84 | tu_015b20.cpp |
| `TrimusWindow::~TrimusWindow()` | 0x1d830 | 52 | tu_015b20.cpp |
| `MenuWindow::MenuWindow(int, int, ListMenu*)` | 0x2e6d4 | 676 | tu_015b20.cpp |
| `MenuWindow::~MenuWindow() (complete)` | 0x2ec3c | 316 | tu_015b20.cpp |
| `MenuWindow::~MenuWindow() (deleting)` | 0x2ed78 | 52 | tu_015b20.cpp |
| `TrimusWindow::animateIn(SDL_Surface*, FlipFn)` | 0x39478 | 40 | tu_015b20.cpp |
| `TrimusWindow::animateOut(SDL_Surface*, FlipFn)` | 0x394a0 | 40 | tu_015b20.cpp |
| `TrimusWindow::getId()` | 0x394c8 | 40 | TrimusWindow.cpp |
| `onSIGTERM(int)` | 0x49c3c | 44 | tu_049c3c.cpp |
| `appExit(int)` | 0x49c68 | 88 | tu_049c3c.cpp |
| `pollStatusA_pushEvent()` | 0x49cc0 | 356 | tu_049c3c.cpp |
| `pollStatusB_pushEvent()` | 0x49e24 | 256 | tu_049c3c.cpp |
| `pollVolume_pushEvent()` | 0x49f24 | 300 | tu_049c3c.cpp |
| `statusPollThread(void*)` | 0x4a050 | 184 | tu_049c3c.cpp |
| `pushHotplugEvent(int, int)` | 0x4a108 | 104 | tu_049c3c.cpp |
| `hotplugThread(void*)` | 0x4a170 | 44 | tu_049c3c.cpp |
| `wifiStatusCallback(WifiStatus*)` | 0x4a19c | 352 | tu_049c3c.cpp |
| `main` | 0x4a2fc | 2648 | tu_049c3c.cpp |
| `ringAt(RingNode*, int)` | 0x4c418 | 180 | tu_04c418.cpp |
| `freeRing(RingNode*, void (*)(void*))` | 0x4c4cc | 140 | tu_04c418.cpp |
| `insertRingSorted(RingNode*, void*, int (*)(void*, void*), int)` | 0x4c558 | 472 | tu_04c418.cpp |
| `insertRing(RingNode*, void*, int (*)(void*, void*))` | 0x4c730 | 60 | tu_04c418.cpp |
| `insertRingUnique(RingNode*, void*, int (*)(void*, void*))` | 0x4c76c | 60 | tu_04c418.cpp |
| `ringCount(RingNode*)` | 0x4c7a8 | 120 | tu_04c418.cpp |
| `appendRing(RingNode*, void*)` | 0x4c820 | 200 | tu_04c418.cpp |
| `prependRing(RingNode*, void*)` | 0x4c8e8 | 192 | tu_04c418.cpp |
| `removeRingNode(RingNode*, RingNode*)` | 0x4c9a8 | 312 | tu_04c418.cpp |
| `forEachRing(RingNode*, void (*)(void*))` | 0x4cae0 | 136 | tu_04c418.cpp |
| `readTestInput()` | 0x4cb68 | 380 | tu_04cb68.cpp |
| `SDLKeyTestWindow::SDLKeyTestWindow(int const*, int, int (*)(), char const*, int)` | 0x4cce4 | 164 | tu_04cb68.cpp |
| `SDLKeyTestWindow::handleInput()` | 0x4cd88 | 308 | tu_04cb68.cpp |
| `SDLKeyTestWindow::keyName(int)` | 0x4cebc | 248 | tu_04cb68.cpp |
| `drawHighlightedText(SDL_Surface*, void*, SDL_Rect, char const*, SDL_Color, int, int, int)` | 0x4cfb4 | 1416 | tu_04cb68.cpp |
| `SDLKeyTestWindow::draw(SDL_Surface*)` | 0x4d53c | 444 | tu_04cb68.cpp |
| `testKeyTimer(unsigned int, void*)` | 0x4d6f8 | 212 | tu_04cb68.cpp |
| `SDLKeyTestWindow::restartTimer()` | 0x4d7cc | 92 | tu_04cb68.cpp |
| `SDLKeyTestWindow::onHide()` | 0x4d828 | 72 | tu_04cb68.cpp |
| `SDLKeyTestWindow::onShow()` | 0x4d870 | 36 | tu_04cb68.cpp |
| `TestKeyImage::TestKeyImage(char const*, char const*)` | 0x4d894 | 168 | tu_04cb68.cpp |
| `TestKeyImage::~TestKeyImage()` | 0x4d93c | 96 | tu_04cb68.cpp |
| `KeyTestWindow::KeyTestWindow(int const*, int, int (*)(), char const*, int)` | 0x4d99c | 408 | tu_04cb68.cpp |
| `KeyTestWindow::keyName(int)` | 0x4db34 | 272 | tu_04cb68.cpp |
| `KeyTestWindow::expecting(int)` | 0x4dc44 | 84 | tu_04cb68.cpp |
| `KeyTestWindow::draw(SDL_Surface*)` | 0x4dc98 | 1324 | tu_04cb68.cpp |
| `KeyTestWindow2::KeyTestWindow2()` | 0x4e1c4 | 108 | tu_04cb68.cpp |
| `KeyTestWindow2::~KeyTestWindow2()` | 0x4e230 | 316 | tu_04cb68.cpp |
| `KeyTestWindow2::loadMasks()` | 0x4e3a0 | 572 | tu_04cb68.cpp |
| `KeyTestWindow2::mapKey(SDLKey)` | 0x4e5dc | 408 | tu_04cb68.cpp |
| `KeyTestWindow2::handleInput()` | 0x4e774 | 372 | tu_04cb68.cpp |
| `keyMask(KeyTestWindow2*, int)` | 0x4e8e8 | 212 | tu_04cb68.cpp |
| `keyMaskRect(KeyTestWindow2*, int, SDL_Rect*)` | 0x4e9bc | 1084 | tu_04cb68.cpp |
| `drawKeyTestFooter(SDL_Surface*)` | 0x4edf8 | 312 | tu_04cb68.cpp |
| `KeyTestWindow2::draw(SDL_Surface*)` | 0x4ef30 | 476 | tu_04cb68.cpp |
| `LCDTestWindow::LCDTestWindow()` | 0x4f10c | 192 | tu_04cb68.cpp |
| `LCDTestWindow::handleInput()` | 0x4f1cc | 304 | tu_04cb68.cpp |
| `LCDTestWindow::draw(SDL_Surface*)` | 0x4f2fc | 328 | tu_04cb68.cpp |
| `LCDTestWindow::onHide()` | 0x4f444 | 32 | tu_04cb68.cpp |
| `LCDTestWindow::onShow()` | 0x4f464 | 32 | tu_04cb68.cpp |
| `SoundTestWindow::SoundTestWindow()` | 0x4f484 | 92 | tu_04cb68.cpp |
| `SoundTestWindow::handleInput()` | 0x4f4e0 | 168 | tu_04cb68.cpp |
| `SoundTestWindow::draw(SDL_Surface*)` | 0x4f588 | 616 | tu_04cb68.cpp |
| `soundTestTimer(unsigned int, void*)` | 0x4f7f0 | 112 | tu_04cb68.cpp |
| `SoundTestWindow::playNext()` | 0x4f860 | 280 | tu_04cb68.cpp |
| `SoundTestWindow::onHide()` | 0x4f978 | 204 | tu_04cb68.cpp |
| `SoundTestWindow::onShow()` | 0x4fa44 | 288 | tu_04cb68.cpp |
| `readUsbTestInput()` | 0x4fb64 | 112 | tu_04cb68.cpp |
| `SDCardTestWindow::SDCardTestWindow()` | 0x4fbd4 | 164 | tu_04cb68.cpp |
| `SDCardTestWindow::handleInput()` | 0x4fc78 | 180 | tu_04cb68.cpp |
| `SDCardTestWindow::draw(SDL_Surface*)` | 0x4fd2c | 684 | tu_04cb68.cpp |
| `SDCardTestWindow::testWrite()` | 0x4ffd8 | 564 | tu_04cb68.cpp |
| `SDCardTestWindow::testRead()` | 0x5020c | 668 | tu_04cb68.cpp |
| `SDCardTestWindow::onHide()` | 0x504a8 | 64 | tu_04cb68.cpp |
| `sdCardTestThread(void*)` | 0x504e8 | 68 | tu_04cb68.cpp |
| `SDCardTestWindow::onShow()` | 0x5052c | 56 | tu_04cb68.cpp |
| `TestMenuItem::TestMenuItem(char const*, char const*)` | 0x50564 | 192 | tu_04cb68.cpp |
| `TestMenuItem::~TestMenuItem()` | 0x50624 | 80 | tu_04cb68.cpp |
| `HardwareTestMenu::HardwareTestMenu()` | 0x50674 | 352 | tu_04cb68.cpp |
| `HardwareTestMenu::~HardwareTestMenu()` | 0x507d4 | 68 | tu_04cb68.cpp |
| `HardwareTestMenu::vfunc4()` | 0x5084c | 1180 | tu_04cb68.cpp |
| `testResultLabel(int)` | 0x50ce8 | 88 | tu_04cb68.cpp |
| `testResultColor(int)` | 0x50d40 | 148 | tu_04cb68.cpp |
| `HardwareTestMenu::vfunc7(SDL_Surface*)` | 0x50dd4 | 1468 | tu_04cb68.cpp |
| `HardwareTestMenu::vfunc18()` | 0x51390 | 196 | tu_04cb68.cpp |
| `HardwareTestMenu::initializeTests()` | 0x51454 | 540 | tu_04cb68.cpp |
| `scalePixels(int, int, int, int, int, int, int, int, int*, int*)` | 0x11cdc8 | 376 | tu_11cdc8.cpp |
| `sdCardSpace[abi:cxx11]()` | 0x11cf40 | 600 | tu_11cdc8.cpp |
| `sdCardFreeMB()` | 0x11d198 | 332 | tu_11cdc8.cpp |
| `isMounted(char const*)` | 0x11d2e4 | 220 | tu_11cdc8.cpp |
| `sdCardMounted()` | 0x11d3c0 | 32 | tu_11cdc8.cpp |
| `moduleLoaded(char const*)` | 0x11d3e0 | 240 | tu_11cdc8.cpp |
| `isSpinand()` | 0x11d4d0 | 52 | tu_11cdc8.cpp |
| `fileExists(char const*, char const*, int)` | 0x11d504 | 400 | tu_11cdc8.cpp |
| `loadPlatformPaths()` | 0x11d694 | 2316 | tu_11cdc8.cpp |
| `dualPartition()` | 0x11dfa0 | 140 | tu_11cdc8.cpp |
| `copyNeoGeo(char const*)` | 0x11e02c | 196 | tu_11cdc8.cpp |
| `platformPath(int)` | 0x11e0f0 | 304 | tu_11cdc8.cpp |
| `verifyGameList(char const*, char const*)` | 0x11e220 | 140 | tu_11cdc8.cpp |
| `mountP2Ext()` | 0x11e2ac | 228 | tu_11cdc8.cpp |
| `removeAllDBFiles()` | 0x11e390 | 5028 | tu_11cdc8.cpp |
| `removeEmulatorConfigs()` | 0x11f734 | 92 | tu_11cdc8.cpp |
| `initSdCardFolders()` | 0x11f790 | 2072 | tu_11cdc8.cpp |
| `swapSize()` | 0x11ffa8 | 180 | tu_11cdc8.cpp |
| `enableSwap()` | 0x12005c | 248 | tu_11cdc8.cpp |
| `maxCpuMHz()` | 0x120154 | 140 | tu_11cdc8.cpp |
| `memoryMB()` | 0x1201e0 | 176 | tu_11cdc8.cpp |
| `removeMax(std::__cxx11::list<int, std::allocator<int> >*)` | 0x120290 | 276 | tu_11cdc8.cpp |
| `removeMin(std::__cxx11::list<int, std::allocator<int> >*)` | 0x1203a4 | 288 | tu_11cdc8.cpp |
| `addRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*, char const*, char const*, char const*, char const*, int, bool)` | 0x1204c4 | 724 | tu_11cdc8.cpp |
| `removeRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*, char const*)` | 0x120798 | 264 | tu_11cdc8.cpp |
| `loadRecentList(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x1208a0 | 596 | tu_11cdc8.cpp |
| `saveRecentList(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x120af4 | 812 | tu_11cdc8.cpp |
| `clearRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x120e20 | 56 | tu_11cdc8.cpp |
| `removeApp(std::vector<AppInfo, std::allocator<AppInfo> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x120e58 | 296 | tu_11cdc8.cpp |
| `findEmuByLaunch(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x120f80 | 244 | tu_11cdc8.cpp |
| `findEmuByLabelPrefix(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x121074 | 216 | tu_11cdc8.cpp |
| `findApp(std::vector<AppInfo, std::allocator<AppInfo> >*, char const*)` | 0x12114c | 224 | tu_11cdc8.cpp |
| `parseApp(void*, char const*, char const*, cJSON*)` | 0x12122c | 1208 | tu_11cdc8.cpp |
| `loadConfigFolder(void*, char const*, char const*, void (*)(void*, char const*, char const*, cJSON*))` | 0x1216e4 | 628 | tu_11cdc8.cpp |
| `loadAppFolder(std::vector<AppInfo, std::allocator<AppInfo> >*, char const*)` | 0x121958 | 64 | tu_11cdc8.cpp |
| `removeEmu(std::vector<EmuInfo, std::allocator<EmuInfo> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x121998 | 296 | tu_11cdc8.cpp |
| `parseEmu(void*, char const*, char const*, cJSON*)` | 0x121ac0 | 2480 | tu_11cdc8.cpp |
| `loadEmuFolder(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x122470 | 64 | tu_11cdc8.cpp |
| `parseThemeColor(char const*, char const*, SDL_Color*)` | 0x1224b0 | 260 | tu_11cdc8.cpp |
| `parseFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, SDL_Color*)` | 0x1225b4 | 440 | tu_11cdc8.cpp |
| `parseListFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, SDL_Color*, SDL_Color*, SDL_Color*)` | 0x12276c | 640 | tu_11cdc8.cpp |
| `parseGridFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, int*, int*, int*, int*, SDL_Color*, SDL_Color*)` | 0x1229ec | 908 | tu_11cdc8.cpp |
| `parseTheme(void*, char const*, char const*, cJSON*)` | 0x122d78 | 1316 | tu_11cdc8.cpp |
| `loadThemeFolder(std::vector<Theme, std::allocator<Theme> >*, char const*)` | 0x12329c | 64 | tu_11cdc8.cpp |
| `findTheme(std::vector<Theme, std::allocator<Theme> >*, char const*)` | 0x1232dc | 212 | tu_11cdc8.cpp |
| `replaceAll(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x1233b0 | 148 | tu_11cdc8.cpp |
| `loadTestResults(int*, char*)` | 0x123444 | 532 | tu_11cdc8.cpp |
| `saveTestResults(int*)` | 0x123658 | 336 | tu_11cdc8.cpp |
| `getSerial(char*)` | 0x1237a8 | 176 | tu_11cdc8.cpp |
| `languageFontExists()` | 0x123858 | 84 | tu_11cdc8.cpp |
| `milliseconds()` | 0x1238ac | 92 | tu_11cdc8.cpp |
| `printSurfaceFormat(char const*, SDL_Surface*)` | 0x123908 | 304 | tu_11cdc8.cpp |
| `loadImage(char const*, SDL_Surface*, bool)` | 0x123a38 | 184 | tu_11cdc8.cpp |
| `loadSkinImage(char const*, char const*, char const*, SDL_Surface*, bool)` | 0x123af0 | 412 | tu_11cdc8.cpp |
| `readTextFile(char const*, char*, int)` | 0x123c8c | 156 | tu_11cdc8.cpp |
| `findProcess(char const*, int)` | 0x123d28 | 620 | tu_11cdc8.cpp |
| `openThemeFont(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*, char const*, int)` | 0x129f9c | 420 | tu_129f9c.cpp |
| `loadImeImage(char const*, char const*, char const*)` | 0x12a140 | 316 | tu_129f9c.cpp |
| `changeLanguageFonts(Theme*, int*)` | 0x12a27c | 1640 | tu_129f9c.cpp |
| `compositeThemeImage(SDL_Surface*, SDL_Surface*)` | 0x12a8e4 | 384 | tu_129f9c.cpp |
| `setDefaultTheme(Theme*)` | 0x12aa64 | 456 | tu_129f9c.cpp |
| `loadTheme(Theme*, int*)` | 0x12ac2c | 6864 | tu_129f9c.cpp |
| `freeTheme()` | 0x12c6fc | 1836 | tu_129f9c.cpp |
| `FileBrowserCache::FileBrowserCache(bool (*)(char const*, char const*))` | 0x12ce28 | 76 | tu_12ce28.cpp |
| `FileBrowserCache::~FileBrowserCache()` | 0x12ce74 | 68 | tu_12ce28.cpp |
| `FileBrowserCache::clear()` | 0x12ceb8 | 256 | tu_12ce28.cpp |
| `compareFileItems(TextItem*, TextItem*)` | 0x12cfb8 | 148 | tu_12ce28.cpp |
| `insertFileItem(std::vector<TextItem*, std::allocator<TextItem*> >*, TextItem*)` | 0x12d04c | 320 | tu_12ce28.cpp |
| `countDirectory(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool (*)(char const*, char const*), bool, char*)` | 0x12d18c | 628 | tu_12ce28.cpp |
| `countDirectoryItems(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool (*)(char const*, char const*))` | 0x12d400 | 180 | tu_12ce28.cpp |
| `loadDirectory(std::vector<TextItem*, std::allocator<TextItem*> >*, char const*, bool (*)(char const*, char const*), int, int)` | 0x12d4b4 | 792 | tu_12ce28.cpp |
| `FileBrowserCache::freeItems(std::vector<TextItem*, std::allocator<TextItem*> >*)` | 0x12d7cc | 164 | tu_12ce28.cpp |
| `FileBrowserCache::totalItems()` | 0x12d870 | 168 | tu_12ce28.cpp |
| `FileBrowserCache::trim()` | 0x12d918 | 272 | tu_12ce28.cpp |
| `FileBrowserCache::find(char const*)` | 0x12da28 | 464 | tu_12ce28.cpp |
| `FileBrowserCache::open(char const*)` | 0x12dbf8 | 320 | tu_12ce28.cpp |
| `FileBrowserCache::parent()` | 0x12dd38 | 504 | tu_12ce28.cpp |
| `FileBrowserCache::child(char const*)` | 0x12df30 | 420 | tu_12ce28.cpp |
| `FileAction::FileAction(char const*, int)` | 0x12e0d4 | 156 | tu_12ce28.cpp |
| `zerror(char const*)` | 0x1302cc | 40 | tu_1302cc.cpp |
| `bit_reverse16(int)` | 0x1302f4 | 188 | tu_1302cc.cpp |
| `bit_reverse(int, int)` | 0x1303b0 | 100 | tu_1302cc.cpp |
| `zbuild_huffman(zhuffman*, unsigned char*, int)` | 0x130414 | 1016 | tu_1302cc.cpp |
| `zget8(zbuf*)` | 0x13080c | 172 | tu_1302cc.cpp |
| `fill_bits(zbuf*)` | 0x1308b8 | 172 | tu_1302cc.cpp |
| `zreceive(zbuf*, int)` | 0x130964 | 144 | tu_1302cc.cpp |
| `zhuffman_decode(zbuf*, zhuffman*)` | 0x1309f4 | 540 | tu_1302cc.cpp |
| `compute_huffman_codes(zbuf*)` | 0x130c10 | 992 | tu_1302cc.cpp |
| `init_default_huffman()` | 0x130ff0 | 304 | tu_1302cc.cpp |
| `historyWrite(zbuf*, unsigned char)` | 0x131120 | 108 | tu_1302cc.cpp |
| `historyRead(zbuf*, int)` | 0x13118c | 100 | tu_1302cc.cpp |
| `parse_zlib_header(zbuf*)` | 0x1311f0 | 232 | tu_1302cc.cpp |
| `parse_huffman_block(zbuf*, int)` | 0x1312d8 | 876 | tu_1302cc.cpp |
| `parse_uncompressed_block(zbuf*, int)` | 0x131644 | 352 | tu_1302cc.cpp |
| `parse_header(zbuf*)` | 0x1317a4 | 708 | tu_1302cc.cpp |
| `openZlib(unsigned char*, _IO_FILE*, int)` | 0x131a68 | 252 | tu_1302cc.cpp |
| `readZlib(zbuf*, void*, int)` | 0x131b64 | 388 | tu_1302cc.cpp |
| `stbi_zlib_decode_malloc(unsigned char*, int, int*)` | 0x131ce8 | 332 | tu_1302cc.cpp |
| `MenuHelper80::MenuHelper80(void (*)(SDL_Surface*))` | 0x131e34 | 96 | tu_131e34.cpp |
| `MenuHelper80::~MenuHelper80()` | 0x131e94 | 68 | tu_131e34.cpp |
| `MenuHelper80::clear()` | 0x131ed8 | 216 | tu_131e34.cpp |
| `MenuHelper80::put(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, void*)` | 0x131fb0 | 300 | tu_131e34.cpp |
| `MenuHelper80::get(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)` | 0x1320dc | 384 | tu_131e34.cpp |
| `MenuHelper80::count()` | 0x13225c | 44 | tu_131e34.cpp |
| `workerThread(void*)` | 0x132288 | 344 | tu_131e34.cpp |
| `MenuHelper60::MenuHelper60(int)` | 0x1323e0 | 188 | tu_131e34.cpp |
| `MenuHelper60::~MenuHelper60()` | 0x13249c | 308 | tu_131e34.cpp |
| `MenuHelper60::addWorker(BGWorker*)` | 0x1325d0 | 240 | tu_131e34.cpp |
| `MenuHelper60::takeWorker()` | 0x1326c0 | 160 | tu_131e34.cpp |
| `genShortLangFile(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x134080 | 836 | tu_134080.cpp |
| `createEnglishLangFile()` | 0x1343c4 | 420 | tu_134080.cpp |
| `readLanguageFile(char const*, Language&)` | 0x134568 | 664 | tu_134080.cpp |
| `loadLanguages(char const*)` | 0x134800 | 1844 | tu_134080.cpp |
| `languageChoices()` | 0x134f34 | 540 | tu_134080.cpp |
| `languageString(int)` | 0x135150 | 300 | tu_134080.cpp |
| `openInputDevice(char const*)` | 0x1376fc | 744 | tu_1376fc.cpp |
| `touchInputThread(void*)` | 0x1379e4 | 988 | tu_1376fc.cpp |
| `startTouchInput()` | 0x137dc0 | 64 | tu_1376fc.cpp |
| `stopTouchInput()` | 0x137e00 | 56 | tu_1376fc.cpp |
| `waitMappingKey()` | 0x137e38 | 280 | tu_1376fc.cpp |
| `KeySettingWindow::KeySettingWindow()` | 0x137f50 | 248 | tu_1376fc.cpp |
| `KeySettingWindow::~KeySettingWindow()` | 0x138048 | 216 | tu_1376fc.cpp |
| `KeySettingWindow::~KeySettingWindow()` | 0x138120 | 52 | tu_1376fc.cpp |
| `KeySettingWindow::handleInput()` | 0x138154 | 948 | tu_1376fc.cpp |
| `drawKeySettingFooter(SDL_Surface*, SDL_Surface*)` | 0x138508 | 236 | tu_1376fc.cpp |
| `KeySettingWindow::draw(SDL_Surface*)` | 0x1385f4 | 380 | tu_1376fc.cpp |
| `KeySettingWindow::loadResources()` | 0x138770 | 952 | tu_1376fc.cpp |
| `KeySettingWindow::onHide()` | 0x138ba4 | 32 | tu_1376fc.cpp |
| `KeySettingWindow::onShow()` | 0x138bc4 | 32 | tu_1376fc.cpp |
| `KeySettingWindow::saveState(cJSON*)` | 0x138be4 | 36 | tu_1376fc.cpp |
| `KeySettingWindow::restoreState(cJSON*)` | 0x138c08 | 40 | tu_1376fc.cpp |
| `KeySettingWindow::update()` | 0x138c30 | 32 | tu_1376fc.cpp |
| `wifiMode()` | 0x138c50 | 116 | tu_138c50.cpp |
| `wlanAddress(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)` | 0x138cc4 | 304 | tu_138c50.cpp |
| `switch2WpaMode(int)` | 0x138df4 | 288 | tu_138c50.cpp |
| `swicth2ApMode()` | 0x138f14 | 320 | tu_138c50.cpp |
| `WifiOnOffLabel::WifiOnOffLabel(Action*, bool)` | 0x139054 | 204 | tu_138c50.cpp |
| `WifiOnOffLabel::~WifiOnOffLabel()` | 0x139120 | 124 | tu_138c50.cpp |
| `WifiOnOffLabel::~WifiOnOffLabel()` | 0x13919c | 52 | tu_138c50.cpp |
| `WifiOnOffLabel::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x1391d0 | 936 | tu_138c50.cpp |
| `WifiLabel::WifiLabel(Action*)` | 0x139578 | 68 | tu_138c50.cpp |
| `WifiLabel::~WifiLabel()` | 0x1395bc | 60 | tu_138c50.cpp |
| `WifiLabel::~WifiLabel()` | 0x1395f8 | 52 | tu_138c50.cpp |
| `WifiLabel::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x13962c | 224 | tu_138c50.cpp |
| `WifiItem::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x13970c | 904 | tu_138c50.cpp |
| `WifiConnectAction::execute(void*)` | 0x139a94 | 544 | tu_138c50.cpp |
| `WifiConnectDialogAction::execute(void*)` | 0x139cb4 | 160 | tu_138c50.cpp |
| `WifiItemAction::execute(void*)` | 0x139d54 | 428 | tu_138c50.cpp |
| `toggleWifi()` | 0x139f00 | 428 | tu_138c50.cpp |
| `WifiOnOffAction::execute(void*)` | 0x13a0ac | 448 | tu_138c50.cpp |
| `WifiOnOffDialogAction::execute(void*)` | 0x13a26c | 140 | tu_138c50.cpp |
| `WifiMenuDrawer::WifiMenuDrawer()` | 0x13a2f8 | 224 | tu_138c50.cpp |
| `WifiMenuDrawer::~WifiMenuDrawer()` | 0x13a3d8 | 116 | tu_138c50.cpp |
| `WifiMenuDrawer::~WifiMenuDrawer()` | 0x13a44c | 52 | tu_138c50.cpp |
| `WifiMenuDrawer::vfunc3()` | 0x13a480 | 300 | tu_138c50.cpp |
| `WifiMenuDrawer::vfunc4()` | 0x13a5ac | 184 | tu_138c50.cpp |
| `WifiMenuDrawer::getItem(int)` | 0x13a664 | 136 | tu_138c50.cpp |
| `WifiMenuDrawer::drawItems(SDL_Surface*)` | 0x13a6ec | 1256 | tu_138c50.cpp |
| `WifiMenuDrawer::count()` | 0x13abd4 | 48 | tu_138c50.cpp |
| `WifiMenuDrawer::vfunc13()` | 0x13ac04 | 308 | tu_138c50.cpp |
| `WifiMenu::WifiMenu()` | 0x13ad38 | 832 | tu_138c50.cpp |
| `WifiMenu::~WifiMenu()` | 0x13b078 | 260 | tu_138c50.cpp |
| `WifiMenu::~WifiMenu()` | 0x13b17c | 52 | tu_138c50.cpp |
| `WifiMenu::vfunc7(SDL_Surface*)` | 0x13b1b0 | 160 | tu_138c50.cpp |
| `WifiMenu::vfunc4()` | 0x13b250 | 112 | tu_138c50.cpp |
| `WifiMenu::vfunc15(int)` | 0x13b2c0 | 2212 | tu_138c50.cpp |
| `HotspotSwitchAction::execute(void*)` | 0x13bb64 | 348 | tu_138c50.cpp |
| `writeConfigValue(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x13bcc0 | 488 | tu_138c50.cpp |
| `readConfigValue(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x13bea8 | 524 | tu_138c50.cpp |
| `HotspotWriteAction::execute(void*)` | 0x13c0b4 | 232 | tu_138c50.cpp |
| `HotspotImePopupAction::execute(void*)` | 0x13c19c | 200 | tu_138c50.cpp |
| `HotspotMenu::HotspotMenu()` | 0x13c264 | 284 | tu_138c50.cpp |
| `HotspotMenu::~HotspotMenu()` | 0x13c380 | 100 | tu_138c50.cpp |
| `HotspotMenu::~HotspotMenu()` | 0x13c3e4 | 52 | tu_138c50.cpp |
| `HotspotMenu::vfunc7(SDL_Surface*)` | 0x13c418 | 48 | tu_138c50.cpp |
| `HotspotMenu::vfunc15(int)` | 0x13c448 | 40 | tu_138c50.cpp |
| `ImeWindow::ImeWindow(int title, Action *callback, int label)` | 0x13edfc | 1060 | tu_13edfc.cpp |
| `ImeWindow::~ImeWindow()` | 0x13f220 | 688 | tu_13edfc.cpp |
| `ImeWindow::~ImeWindow() [deleting]` | 0x13f4d0 | 52 | tu_13edfc.cpp |
| `void ImeWindow::rowKeys(std::vector<int>&, int key)` | 0x13f504 | 460 | tu_13edfc.cpp |
| `void ImeWindow::columnKeys(std::vector<int>&, int key, bool up)` | 0x13f6d0 | 404 | tu_13edfc.cpp |
| `void ImeWindow::moveHorizontal(bool left)` | 0x13f864 | 652 | tu_13edfc.cpp |
| `void ImeWindow::moveVertical(bool up)` | 0x13faf0 | 652 | tu_13edfc.cpp |
| `int ImeWindow::handleInput()` | 0x13fd7c | 1716 | tu_13edfc.cpp |
| `void ImeWindow::drawKeys(SDL_Surface*, int x, int y)` | 0x140430 | 1756 | tu_13edfc.cpp |
| `int ImeWindow::draw(SDL_Surface*)` | 0x140b0c | 328 | tu_13edfc.cpp |
| `void ImeWindow::setKeyIcons(int key, SDL_Surface *icon, SDL_Surface *hint)` | 0x140c54 | 244 | tu_13edfc.cpp |
| `void ImeWindow::initKey(ImeKey&, int id, SDL_Surface *focused, SDL_Surface *normal, SDL_Surface *icon, SDL_Surface *hint)` | 0x140d48 | 3716 | tu_13edfc.cpp |
| `void ImeWindow::buildKeys()` | 0x141bcc | 1036 | tu_13edfc.cpp |
| `void ImeWindow::onHide()` | 0x141fd8 | 32 | tu_13edfc.cpp |
| `void ImeWindow::onShow()` | 0x141ff8 | 32 | tu_13edfc.cpp |
| `void ImeWindow::saveState(cJSON*)` | 0x142018 | 36 | tu_13edfc.cpp |
| `int ImeWindow::restoreState(cJSON*)` | 0x14203c | 40 | tu_13edfc.cpp |
| `void ImeWindow::update()` | 0x142064 | 32 | tu_13edfc.cpp |
| `int parsePinyinLine(tiny_utf8::string, char32_t*, std::string&)` | 0x143720 | 512 | tu_143720.cpp |
| `void loadPinyinTable()` | 0x143920 | 452 | tu_143720.cpp |
| `char* pinyinInitials(const char*)` | 0x143ae4 | 528 | tu_143720.cpp |
| `int pinyinFind(const char*, const char*, const char*, const char**, int*)` | 0x143cf4 | 308 | tu_143720.cpp |
| `deleteTextItem(SDL_Surface*)` | 0x1484d4 | 68 | tu_1484d4.cpp |
| `freeData(SDL_Surface*)` | 0x148518 | 44 | tu_1484d4.cpp |
| `lastPathComponent(char*)` | 0x148544 | 184 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::MultiDBCachedTextMenu(std::string const&)` | 0x1485fc | 416 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::~MultiDBCachedTextMenu()` | 0x14879c | 256 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::~MultiDBCachedTextMenu() [deleting]` | 0x14889c | 52 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::openCache(EmuInfo&, char*, size_t)` | 0x1488d0 | 756 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::prepare()` | 0x148bc4 | 744 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::fetchForward()` | 0x148eac | 132 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::fetchBackward()` | 0x148f30 | 124 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::vfunc16()` | 0x148fac | 60 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::vfunc17()` | 0x148fe8 | 60 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::vfunc9()` | 0x149024 | 60 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::vfunc10()` | 0x149060 | 60 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::restorePosition(int, int, int)` | 0x14909c | 156 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::prepareBGWorkers(int, int)` | 0x149138 | 988 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::getItem(int)` | 0x149514 | 784 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::vfunc6(int)` | 0x149824 | 816 | tu_1484d4.cpp |
| `MultiDBCachedTextMenu::count()` | 0x149b54 | 40 | tu_1484d4.cpp |
| `int stub_4acfc()` | 0x14acfc | 28 | tu_14acfc.c |
| `int stub_4ad18()` | 0x14ad18 | 28 | tu_14acfc.c |
| `int stub_4ad34()` | 0x14ad34 | 28 | tu_14acfc.c |
| `int setVolumeTinymix(int vol)` | 0x14ad50 | 152 | tu_14acfc.c |
| `int stub_4ade8()` | 0x14ade8 | 28 | tu_14acfc.c |
| `int writeFile(const char *path, const char *text)` | 0x14ae04 | 112 | tu_14acfc.c |
| `void initBacklightPwm()` | 0x14ae74 | 96 | tu_14acfc.c |
| `int setBacklight(int level)` | 0x14aed4 | 336 | tu_14acfc.c |
| `int backlightOff()` | 0x14b024 | 40 | tu_14acfc.c |
| `bool isBacklightOff()` | 0x14b04c | 176 | tu_14acfc.c |
| `bool isPowerKeyReleased()` | 0x14b0fc | 156 | tu_14acfc.c |
| `int readAxpStatus(int *battery, int *voltage, int *charging)` | 0x14b198 | 476 | tu_14acfc.c |
| `int fdList_find(int fd)` | 0x14b374 | 120 | tu_14acfc.c |
| `int fdList_add(int fd)` | 0x14b3ec | 120 | tu_14acfc.c |
| `int fdList_remove(int fd)` | 0x14b464 | 216 | tu_14acfc.c |
| `int hasDsp1(int card)` | 0x14b53c | 80 | tu_14acfc.c |
| `int hotplugLoop(void (*cb)(int, int))` | 0x14b58c | 648 | tu_14acfc.c |
| `int stopHotplug()` | 0x14b814 | 76 | tu_14acfc.c |
| `long fileSize(FILE *f)` | 0x14b860 | 68 | tu_14b860.c |
| `int readWholeFile(const char *name, FILE *f, char **out)` | 0x14b8a4 | 372 | tu_14b860.c |
| `const char *getLanguageFile()` | 0x14ba18 | 64 | tu_14b860.c |
| `void setLanguageFile(const char *lang)` | 0x14ba58 | 96 | tu_14b860.c |
| `const char *getKeymap()` | 0x14bab8 | 64 | tu_14b860.c |
| `void setKeymap(const char *keymap)` | 0x14baf8 | 108 | tu_14b860.c |
| `void setupKeymap(const char *keymap)` | 0x14bb64 | 120 | tu_14b860.c |
| `int getWifiOn()` | 0x14bbdc | 36 | tu_14b860.c |
| `void setWifiOn(int on)` | 0x14bc00 | 84 | tu_14b860.c |
| `int getWifiConnected()` | 0x14bc54 | 36 | tu_14b860.c |
| `void setWifiConnected(int connected)` | 0x14bc78 | 48 | tu_14b860.c |
| `void loadSystemJson(void *shm, char **theme, int *fontSize)` | 0x14bca8 | 1976 | tu_14b860.c |
| `void loadThemeAndFontSize(char **theme, int *fontSize)` | 0x14c460 | 412 | tu_14b860.c |
| `void saveSystemJson(void *shm, const char *theme, int fontSize)` | 0x14c5fc | 1320 | tu_14b860.c |
| `int readByte(FILE *f)` | 0x14cb24 | 84 | tu_14cb24.c |
| `unsigned char readU8(FILE *f)` | 0x14cb78 | 44 | tu_14cb24.c |
| `void skipBytes(FILE *f, int n)` | 0x14cba4 | 112 | tu_14cb24.c |
| `unsigned short readU16BE(FILE *f)` | 0x14cc14 | 80 | tu_14cb24.c |
| `unsigned int readU32BE(FILE *f)` | 0x14cc64 | 68 | tu_14cb24.c |
| `unsigned short readU16LE(FILE *f)` | 0x14cca8 | 80 | tu_14cb24.c |
| `unsigned int readU32LE(FILE *f)` | 0x14ccf8 | 72 | tu_14cb24.c |
| `void readFields(FILE *f, const char *fmt, ...)` | 0x14cd40 | 412 | tu_14cb24.c |
| `int findSignature(ZipFile *zip, const unsigned char *sig)` | 0x14cedc | 276 | tu_14cb24.c |
| `int zipReadCentralDir(ZipFile *zip)` | 0x14cff0 | 492 | tu_14cb24.c |
| `int zipFindEntry(ZipFile *zip, const char *name, unsigned int crc)` | 0x14d1dc | 572 | tu_14cb24.c |
| `ZipEntry *zipOpenEntry(ZipFile *zip, const char *name, unsigned int crc)` | 0x14d418 | 544 | tu_14cb24.c |
| `int zipRead(ZipEntry *e, void *buf, unsigned int len)` | 0x14d638 | 248 | tu_14cb24.c |
| `void *zipReadFile(ZipFile *zip, const char *name, unsigned int crc, unsigned int *size)` | 0x14d730 | 296 | tu_14cb24.c |
| `void zipCloseEntry(ZipEntry *e)` | 0x14d858 | 88 | tu_14cb24.c |
| `ZipFile *zipOpen(const char *path)` | 0x14d8b0 | 364 | tu_14cb24.c |
| `void zipClose(ZipFile *zip)` | 0x14da1c | 52 | tu_14cb24.c |
| `cJSON *loadLanguageStrings(const char *lang)` | 0x14da50 | 260 | tu_14da50.c |
| `void freeLanguageStrings(cJSON *strings)` | 0x14db54 | 48 | tu_14da50.c |
| `const char *getLanguageString(cJSON *strings, int id)` | 0x14db84 | 148 | tu_14da50.c |
| `unsigned char i2cRead(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len)` | 0x14dc18 | 540 | tu_14dc18.c |
| `unsigned char i2cWrite(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len)` | 0x14de34 | 356 | tu_14dc18.c |
| `void msleep(int ms)` | 0x14df98 | 48 | tu_14dc18.c |
| `unsigned char randomByte()` | 0x14dfc8 | 140 | tu_14dc18.c |
| `unsigned char waitForProbe()` | 0x14e054 | 120 | tu_14dc18.c |
| `void printThreadInfo(const char *name)` | 0x14e0cc | 76 | tu_14e0cc.c |
| `int wifiSignalLevel(int rssi)` | 0x14e118 | 108 | tu_14e0cc.c |
| `int readFileTo(const char *path, void *buf)` | 0x14e184 | 156 | tu_14e0cc.c |
| `int setWifiScanEnable(int on)` | 0x14e220 | 64 | tu_14e0cc.c |
| `int wifiConnectOpen(const char *ssid)` | 0x14e260 | 316 | tu_14e0cc.c |
| `int wifiConnectPsk(const char *ssid, const char *psk)` | 0x14e39c | 324 | tu_14e0cc.c |
| `void runUdhcpc()` | 0x14e4e0 | 72 | tu_14e0cc.c |
| `int wifiStartConnect()` | 0x14e528 | 60 | tu_14e0cc.c |
| `WifiStatus *getWifiStatus()` | 0x14e564 | 32 | tu_14e0cc.c |
| `unsigned char hexDigit(unsigned char v)` | 0x14e584 | 116 | tu_14e0cc.c |
| `unsigned char hexValue(unsigned char c)` | 0x14e5f8 | 168 | tu_14e0cc.c |
| `void unescapeSsid(char *s, int len)` | 0x14e6a0 | 388 | tu_14e0cc.c |
| `int parseScanLine(const char *text, WifiAP *ap)` | 0x14e824 | 680 | tu_14e0cc.c |
| `int isScannedSsid(const char *ssid)` | 0x14eacc | 208 | tu_14e0cc.c |
| `int copyScanResults(WifiAP *out, int max)` | 0x14eb9c | 140 | tu_14e0cc.c |
| `void parseScanResults()` | 0x14ec28 | 644 | tu_14e0cc.c |
| `void rx_wifi_do_scan()` | 0x14eeac | 236 | tu_14e0cc.c |
| `WifiStatus queryWifiStatus()` | 0x14ef98 | 1432 | tu_14e0cc.c |
| `void *wifiMonitorThread(void *arg)` | 0x14f530 | 260 | tu_14e0cc.c |
| `void startWifiMonitor(void (*callback)(WifiStatus *))` | 0x14f634 | 192 | tu_14e0cc.c |
| `void stopWifiMonitor()` | 0x14f6f4 | 60 | tu_14e0cc.c |
| `byte readDeviceKeys()` | 0x14f730 | 552 | tu_14f730.c |
| `void invertSwapHalves(byte *data)` | 0x14f958 | 220 | tu_14f730.c |
| `void xorSwapHalves(byte *data, byte *key)` | 0x14fa34 | 268 | tu_14f730.c |
| `void rotateBytesRight(byte *data)` | 0x14fb40 | 188 | tu_14f730.c |
| `void transformDeviceBlock(byte *input, byte *output, byte *seed, byte rounds, int bits, byte mode)` | 0x14fbfc | 1208 | tu_14f730.c |
| `byte beginDeviceAuth()` | 0x1500b4 | 620 | tu_14f730.c |
| `byte sendDeviceKey()` | 0x150320 | 484 | tu_14f730.c |
| `byte authenticateDevice(byte *challenge, byte *response)` | 0x150504 | 1492 | tu_14f730.c |

Embedded libraries, verified as whole objects by `scripts/verify-libs.sh`:

| Library | Range | Bytes |
| --- | --- | --- |
| SQLite 3.30.1 (C) | 0x57920–0x11cdc8 | 808,104 |
| cJSON 1.7.10 (compiled as C++) | 0x51b60–0x5791c | 23,996 |
