# Byte-exact functions

Each compiles with Arm GNU Toolchain 8.2-2018.08, `-O0 -fsigned-char`, to the same bytes as
`MainUI-354-clean`. Only link-time words are ignored. CI re-checks all of them
(`scripts/run_matches.sh`). The list itself is `src/matches.tsv`; regenerate this table with
`python3 tools/progress.py --matches-md src/MATCHES.md`.

| Function | Address | Size | File |
| --- | --- | --- | --- |
| `keyEventName(int)` | 0x15b20 | 480 | MainUI.cpp |
| `flipDisplay()` | 0x15d00 | 36 | MainUI.cpp |
| `drawText(SDL_Surface*, void*, SDL_Rect, char const*, SDL_Color, int, bool)` | 0x15d24 | 756 | MainUI.cpp |
| `drawTextLines(SDL_Surface*, void*, SDL_Rect*, char const*, char const*, SDL_Color*, int, bool)` | 0x16018 | 532 | MainUI.cpp |
| `drawIconOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, SDL_Surface*)` | 0x1622c | 420 | MainUI.cpp |
| `drawSkinIconOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, char const*)` | 0x163d0 | 128 | MainUI.cpp |
| `drawTextOnBg(SDL_Surface*, SDL_Surface*, SDL_Rect*, char const*)` | 0x16450 | 284 | MainUI.cpp |
| `drawKeyStateIndicator(SDL_Surface*, int, bool)` | 0x1656c | 540 | MainUI.cpp |
| `drawWifiIndicator(SDL_Surface*, int, bool)` | 0x16788 | 972 | MainUI.cpp |
| `drawSoundCardIndicator(SDL_Surface*, bool)` | 0x16b54 | 324 | MainUI.cpp |
| `drawBatteryIndicator(SDL_Surface*, bool)` | 0x16c98 | 584 | MainUI.cpp |
| `playEnterSound(int, int)` | 0x16ee0 | 136 | MainUI.cpp |
| `drawTitleBar(SDL_Surface*, char const*, bool)` | 0x16f68 | 400 | MainUI.cpp |
| `drawIconLabel(SDL_Surface*, SDL_Surface*, int, int, char const*, char const*)` | 0x170f8 | 424 | MainUI.cpp |
| `drawFooter(SDL_Surface*, char const*, char const*)` | 0x172a0 | 432 | MainUI.cpp |
| `drawBackground(SDL_Surface*)` | 0x17450 | 280 | MainUI.cpp |
| `drawScreenFrame(SDL_Surface*, char const*, int, bool)` | 0x17568 | 180 | MainUI.cpp |
| `powerOff(void*)` | 0x1761c | 140 | MainUI.cpp |
| `powerOffNow()` | 0x176a8 | 24 | MainUI.cpp |
| `sendPowerOffEvent(int, int)` | 0x176c0 | 108 | MainUI.cpp |
| `onUserActivity()` | 0x1772c | 24 | MainUI.cpp |
| `handleGlobalInput(SDL_Event*)` | 0x17744 | 1996 | MainUI.cpp |
| `compareDirentNames(dirent const**, dirent const**)` | 0x17f10 | 80 | MainUI.cpp |
| `compareLabels(void const*, void const*)` | 0x17f60 | 96 | MainUI.cpp |
| `writeCmdToRun(const char*)` | 0x17fc0 | 88 | MainUI.cpp |
| `showLaunchScreen(char const*)` | 0x18018 | 344 | MainUI.cpp |
| `donoting_action(void*, void*)` | 0x18170 | 48 | MainUI.cpp |
| `launchRom(char const*, EmuInfo*, char const*, char const*, int)` | 0x181a0 | 960 | MainUI.cpp |
| `launchApp_buildCommand(AppInfo*, int)` | 0x18560 | 1052 | MainUI.cpp |
| `EmuAction::execute(void*)` | 0x1897c | 388 | MainUI.cpp |
| `EmuAction::progress(int*, int)` | 0x18b00 | 340 | MainUI.cpp |
| `AppAction::execute(void*)` | 0x18c54 | 64 | MainUI.cpp |
| `AppAction::progress(int*, int)` | 0x18c94 | 320 | MainUI.cpp |
| `digitCount(int)` | 0x18dd4 | 104 | MainUI.cpp |
| `animateBusy(SDL_Surface*, SDL_Surface*, SDL_Rect, SDL_Rect, bool)` | 0x18e3c | 1388 | MainUI.cpp |
| `busyScreenThread(void*)` | 0x193a8 | 1280 | MainUI.cpp |
| `loadingScreenThread(void*)` | 0x198a8 | 548 | MainUI.cpp |
| `RemoveFavoriteAction::execute(void*)` | 0x19acc | 56 | MainUI.cpp |
| `SearchAction::execute(void*)` | 0x19b04 | 276 | MainUI.cpp |
| `GlobalSearchAction::execute(void*)` | 0x19c18 | 268 | MainUI.cpp |
| `PopupSearchIME::execute(void*)` | 0x19d24 | 156 | MainUI.cpp |
| `PopupDialogAction::execute(void*)` | 0x19dc0 | 132 | MainUI.cpp |
| `PopupProgressAction::execute(void*)` | 0x19e44 | 136 | MainUI.cpp |
| `GeneralAction::execute(void*)` | 0x19ecc | 152 | MainUI.cpp |
| `ClearFavoriteAction::execute(void*)` | 0x19f64 | 116 | MainUI.cpp |
| `ClearRecentAction::execute(void*)` | 0x19fd8 | 84 | MainUI.cpp |
| `AddFavoriteAction::execute(void*)` | 0x1a02c | 1056 | MainUI.cpp |
| `DeleteRomAction::execute(void*)` | 0x1a44c | 112 | MainUI.cpp |
| `saveFavorites(char const*, std::vector<void*>*)` | 0x1a4bc | 672 | MainUI.cpp |
| `removeFavorite(char const*, char const*)` | 0x1a75c | 456 | MainUI.cpp |
| `buildFavoriteList(char const*)` | 0x1a924 | 2284 | MainUI.cpp |
| `isFavorite(char*, FILE*, char const*)` | 0x1b210 | 632 | MainUI.cpp |
| `buildRecentList(std::list<RecentItem>*)` | 0x1b488 | 1908 | MainUI.cpp |
| `buildAppList(std::vector<AppInfo>*)` | 0x1bbfc | 680 | MainUI.cpp |
| `setLcdColor(int, int, int, int)` | 0x1bea4 | 236 | MainUI.cpp |
| `initAudioSettings(void*)` | 0x1bf90 | 284 | MainUI.cpp |
| `removeDeletedFavorites(void*, int)` | 0x1c0ac | 336 | MainUI.cpp |
| `restoreWindowState(std::vector<TrimusWindow*>*)` | 0x1c1fc | 840 | MainUI.cpp |
| `saveWindowState(std::vector<TrimusWindow*>*, char const*)` | 0x1c544 | 540 | MainUI.cpp |
| `setRomListPosition(std::vector<RomListPosition>*, std::string const&, int, int, int)` | 0x1c760 | 368 | MainUI.cpp |
| `getRomListPosition(std::vector<RomListPosition>*, std::string, RomListPosition*)` | 0x1c8d0 | 196 | MainUI.cpp |
| `loadRomListPositions(std::vector<RomListPosition>*)` | 0x1c994 | 764 | MainUI.cpp |
| `saveRomListPositions(std::vector<RomListPosition>*)` | 0x1cc90 | 592 | MainUI.cpp |
| `loadThumbnail(char const*, int, int)` | 0x1cee0 | 1080 | MainUI.cpp |
| `makeThumbPath(std::string*, char const*)` | 0x1d318 | 1032 | MainUI.cpp |
| `createSurface(int, int)` | 0x1d720 | 188 | MainUI.cpp |
| `TrimusWindow::TrimusWindow(int titleLabelId, int id)` | 0x1d7dc | 84 | MainUI.cpp |
| `TrimusWindow::~TrimusWindow()` | 0x1d830 | 52 | MainUI.cpp |
| `TrimusWindow::~TrimusWindow() (deleting)` | 0x1d864 | 52 | MainUI.cpp |
| `TrimusWindow::dump()` | 0x1d898 | 100 | MainUI.cpp |
| `ListMenu::ListMenu(bool)` | 0x1d8fc | 160 | MainUI.cpp |
| `ListMenu::setRect(SDL_Rect*)` | 0x1d99c | 60 | MainUI.cpp |
| `ListMenu::restorePosition(int, int, int)` | 0x1d9d8 | 272 | MainUI.cpp |
| `ListMenu::moveDown()` | 0x1dae8 | 132 | MainUI.cpp |
| `ListMenu::moveUp()` | 0x1db6c | 112 | MainUI.cpp |
| `ListMenu::pageUp()` | 0x1dbdc | 84 | MainUI.cpp |
| `ListMenu::pageDown()` | 0x1dc30 | 152 | MainUI.cpp |
| `ListMenu::recordKey(int)` | 0x1dcc8 | 300 | MainUI.cpp |
| `ListMenu::keyRepeating()` | 0x1ddf4 | 64 | MainUI.cpp |
| `ListMenu::handleKey()` | 0x1de34 | 756 | MainUI.cpp |
| `TextMenuDrawer::TextMenuDrawer(bool, bool, int, char const*)` | 0x1e128 | 720 | MainUI.cpp |
| `TextMenuDrawer::~TextMenuDrawer()` | 0x1e3f8 | 236 | MainUI.cpp |
| `TextMenuDrawer::~TextMenuDrawer() (deleting)` | 0x1e4e4 | 52 | MainUI.cpp |
| `TextMenuDrawer::onSelect()` | 0x1e518 | 184 | MainUI.cpp |
| `TextMenuDrawer::reloadTheme()` | 0x1e5d0 | 412 | MainUI.cpp |
| `TextMenuDrawer::makeSelectedBackground(SDL_Surface*)` | 0x1e76c | 252 | MainUI.cpp |
| `drawListNumber(int, SDL_Rect*, SDL_Surface*, void*)` | 0x1e868 | 920 | MainUI.cpp |
| `truncateUtf8(char*, int, int)` | 0x1ec00 | 484 | MainUI.cpp |
| `TextMenuDrawer::drawHighlighted(SDL_Surface*, void*, SDL_Rect, char const*, SDL_Color, int, bool, char const*, char const*)` | 0x1ede4 | 2044 | MainUI.cpp |
| `TextMenuDrawer::setBackground(char const*)` | 0x1f5e0 | 180 | MainUI.cpp |
| `TextMenuDrawer::freeSelectedBackground(bool)` | 0x1f694 | 92 | MainUI.cpp |
| `TextMenuDrawer::drawList(SDL_Surface*)` | 0x1f6f0 | 3556 | MainUI.cpp |
| `TextMenuDrawer::handleKey()` | 0x204d4 | 44 | MainUI.cpp |
| `TextMenu::TextMenu(bool, bool, int, char const*)` | 0x20500 | 112 | MainUI.cpp |
| `TextMenu::~TextMenu()` | 0x20570 | 84 | MainUI.cpp |
| `TextMenu::~TextMenu() (deleting)` | 0x205c4 | 52 | MainUI.cpp |
| `TextMenu::freeListItem()` | 0x205f8 | 232 | MainUI.cpp |
| `TextMenu::clearItems()` | 0x206e0 | 52 | MainUI.cpp |
| `TextMenu::getItem(int)` | 0x20714 | 136 | MainUI.cpp |
| `TextMenu::removeItem(int)` | 0x2079c | 252 | MainUI.cpp |
| `TextMenu::addItem(void*)` | 0x20898 | 56 | MainUI.cpp |
| `TextMenu::insertSorted(void*, int (*)(TextItem*, TextItem*))` | 0x208d0 | 364 | MainUI.cpp |
| `TextMenu::addUnique(TextItem*)` | 0x20a3c | 224 | MainUI.cpp |
| `romFolderName(char*)` | 0x20b1c | 184 | MainUI.cpp |
| `importGameList(char const*, int, char const*, sqlite3*)` | 0x20bd4 | 3140 | MainUI.cpp |
| `isHiddenRomFile(EmuInfo*, char const*, char const*, int)` | 0x21818 | 1336 | MainUI.cpp |
| `scanRomDir(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, EmuInfo*, char const*, bool, sqlite3*, char*, char*, int, char*, int, unsigned long*, int*)` | 0x21d50 | 3020 | MainUI.cpp |
| `rebuildRomCache(EmuInfo*, int, char const*, sqlite3*)` | 0x2291c | 876 | MainUI.cpp |
| `isRomFile(char const*, char const*)` | 0x22c88 | 264 | MainUI.cpp |
| `prepareRomCache(EmuInfo*, int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*, int)` | 0x22d90 | 1416 | MainUI.cpp |
| `buildRomList(EmuInfo*, int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)` | 0x23318 | 560 | MainUI.cpp |
| `DBCacheWorker::run()` | 0x23548 | 2868 | MainUI.cpp |
| `DBCacheWorker::finished()` | 0x2407c | 56 | MainUI.cpp |
| `DBCachedTextMenu::pageUp()` | 0x240b4 | 56 | MainUI.cpp |
| `DBCachedTextMenu::pageDown()` | 0x240ec | 60 | MainUI.cpp |
| `DBCachedTextMenu::moveDown()` | 0x24128 | 60 | MainUI.cpp |
| `DBCachedTextMenu::moveUp()` | 0x24164 | 60 | MainUI.cpp |
| `DBCachedTextMenu::prefetchNext()` | 0x241a0 | 316 | MainUI.cpp |
| `DBCachedTextMenu::prefetchPrevious()` | 0x242dc | 308 | MainUI.cpp |
| `DBCachedTextMenu::getItem(int)` | 0x24410 | 372 | MainUI.cpp |
| `DBCachedTextMenu::getItemData(int)` | 0x24584 | 372 | MainUI.cpp |
| `DBCachedTextMenu::restorePosition(int, int, int)` | 0x246f8 | 316 | MainUI.cpp |
| `DBCachedTextMenu::open(char const*, char const*, char const*)` | 0x24834 | 480 | MainUI.cpp |
| `DBCachedTextMenu::removeItem(int)` | 0x24a14 | 788 | MainUI.cpp |
| `deleteCachedTextItem(SDL_Surface*)` | 0x24d28 | 68 | MainUI.cpp |
| `freeCachedData(SDL_Surface*)` | 0x24d6c | 44 | MainUI.cpp |
| `DBCachedTextMenu::setSearch(char const*)` | 0x24d98 | 1192 | MainUI.cpp |
| `DBCachedTextMenu::DBCachedTextMenu(char const*, EmuInfo*, int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)` | 0x25240 | 460 | MainUI.cpp |
| `DBCachedTextMenu::~DBCachedTextMenu()` | 0x2540c | 208 | MainUI.cpp |
| `DBCachedTextMenu::~DBCachedTextMenu()` | 0x254dc | 52 | MainUI.cpp |
| `TextItem::TextItem(char const*, SDL_Surface*, Action*, bool)` | 0x25510 | 272 | MainUI.cpp |
| `TextItem::~TextItem()` | 0x25620 | 208 | MainUI.cpp |
| `IconMenuItem::IconMenuItem(int, char const*, char const*, SDL_Surface*, SDL_Surface*, int, void*, bool)` | 0x256f0 | 484 | MainUI.cpp |
| `IconMenuItem::IconMenuItem(char const*, char const*, char const*, SDL_Surface*, SDL_Surface*, int, void*, bool)` | 0x258d4 | 440 | MainUI.cpp |
| `IconMenuItem::~IconMenuItem()` | 0x25a8c | 208 | MainUI.cpp |
| `IconMenuItem::reload()` | 0x25b5c | 268 | MainUI.cpp |
| `defaultIconFont()` | 0x25c68 | 36 | MainUI.cpp |
| `IconMenu::IconMenu(int, int, bool, bool, int, int, bool)` | 0x25c8c | 444 | MainUI.cpp |
| `IconMenu::~IconMenu()` | 0x25e48 | 128 | MainUI.cpp |
| `IconMenu::~IconMenu()` | 0x25ec8 | 52 | MainUI.cpp |
| `IconMenu::drawList(SDL_Surface*)` | 0x25efc | 4260 | MainUI.cpp |
| `IconMenu::getItem(int)` | 0x26fa0 | 116 | MainUI.cpp |
| `prepareGlobalRomCache()` | 0x27014 | 1808 | MainUI.cpp |
| `IconMenu::onSelect()` | 0x27724 | 600 | MainUI.cpp |
| `IconMenu::freeItems()` | 0x2797c | 180 | MainUI.cpp |
| `IconMenu::addItem(void*)` | 0x27a30 | 56 | MainUI.cpp |
| `IconMenu::reloadTheme()` | 0x27a68 | 348 | MainUI.cpp |
| `IconMenu::handleKey()` | 0x27bc4 | 1804 | MainUI.cpp |
| `SettingMenuItem::SettingMenuItem(int, char const*, int)` | 0x282d0 | 232 | MainUI.cpp |
| `SettingMenuItem::~SettingMenuItem()` | 0x283b8 | 80 | MainUI.cpp |
| `SettingMenuItem::reload()` | 0x28408 | 148 | MainUI.cpp |
| `SettingMenu::SettingMenu()` | 0x2849c | 376 | MainUI.cpp |
| `SettingMenu::~SettingMenu()` | 0x28614 | 308 | MainUI.cpp |
| `SettingMenu::~SettingMenu()` | 0x28748 | 52 | MainUI.cpp |
| `SettingMenu::reloadTheme()` | 0x2877c | 536 | MainUI.cpp |
| `SettingMenu::drawList(SDL_Surface*)` | 0x28994 | 2916 | MainUI.cpp |
| `previewVolume(unsigned int, void*)` | 0x294f8 | 176 | MainUI.cpp |
| `SettingMenu::decrease()` | 0x295a8 | 836 | MainUI.cpp |
| `SettingMenu::increase()` | 0x298ec | 836 | MainUI.cpp |
| `rebootToEfex()` | 0x29c30 | 28 | MainUI.cpp |
| `resetEmulatorSettings(void*)` | 0x29c4c | 40 | MainUI.cpp |
| `resetAllSettings(void*)` | 0x29c74 | 340 | MainUI.cpp |
| `SettingMenu::select()` | 0x29dc8 | 2060 | MainUI.cpp |
| `SettingMenu::handleKey()` | 0x2a5d4 | 280 | MainUI.cpp |
| `SettingMenu::freeListItem()` | 0x2a6ec | 196 | MainUI.cpp |
| `DeviceInfoItem::DeviceInfoItem(char const*, char const*)` | 0x2a7b0 | 140 | MainUI.cpp |
| `DeviceInfoMenu::DeviceInfoMenu()` | 0x2a83c | 248 | MainUI.cpp |
| `DeviceInfoMenu::~DeviceInfoMenu()` | 0x2a934 | 160 | MainUI.cpp |
| `DeviceInfoMenu::~DeviceInfoMenu()` | 0x2a9d4 | 52 | MainUI.cpp |
| `DeviceInfoMenu::reloadTheme()` | 0x2aa08 | 220 | MainUI.cpp |
| `DeviceInfoMenu::drawList(SDL_Surface*)` | 0x2aae4 | 1348 | MainUI.cpp |
| `formatDeviceUuid(char*, unsigned int)` | 0x2b028 | 208 | MainUI.cpp |
| `DeviceInfoMenu::loadInfo()` | 0x2b0f8 | 1036 | MainUI.cpp |
| `DeviceInfoMenu::freeListItem()` | 0x2b504 | 184 | MainUI.cpp |
| `applyPanelTiming()` | 0x2b5bc | 460 | MainUI.cpp |
| `ColorMenu::ColorMenu()` | 0x2b788 | 308 | MainUI.cpp |
| `ColorMenu::~ColorMenu()` | 0x2b8bc | 176 | MainUI.cpp |
| `ColorMenu::~ColorMenu()` | 0x2b96c | 52 | MainUI.cpp |
| `ColorMenu::drawList(SDL_Surface*)` | 0x2b9a0 | 2260 | MainUI.cpp |
| `ColorMenu::handleKey()` | 0x2c274 | 1224 | MainUI.cpp |
| `ColorMenu::onSelect()` | 0x2c73c | 36 | MainUI.cpp |
| `ColorMenu::freeListItem()` | 0x2c760 | 196 | MainUI.cpp |
| `ColorMenu::loadItems()` | 0x2c824 | 300 | MainUI.cpp |
| `LanguageMenu::LanguageMenu(std::vector<LanguageChoice, std::allocator<LanguageChoice> >)` | 0x2c950 | 344 | MainUI.cpp |
| `LanguageMenu::~LanguageMenu()` | 0x2caa8 | 76 | MainUI.cpp |
| `LanguageMenu::~LanguageMenu()` | 0x2caf4 | 52 | MainUI.cpp |
| `LanguageMenu::onSelect()` | 0x2cb28 | 236 | MainUI.cpp |
| `ThemeMenu::ThemeMenu()` | 0x2cc14 | 364 | MainUI.cpp |
| `ThemeMenu::~ThemeMenu()` | 0x2cd80 | 60 | MainUI.cpp |
| `ThemeMenu::~ThemeMenu()` | 0x2cdbc | 52 | MainUI.cpp |
| `ThemeMenu::onSelect()` | 0x2cdf0 | 464 | MainUI.cpp |
| `findEmuForFile(char const*)` | 0x2cfc0 | 408 | MainUI.cpp |
| `requestUpgrade(void*)` | 0x2d158 | 100 | MainUI.cpp |
| `FileAction::execute(void*)` | 0x2d1bc | 952 | MainUI.cpp |
| `fileBrowserFilter(char const*, char const*)` | 0x2d574 | 396 | MainUI.cpp |
| `FileBrowserMenu::FileBrowserMenu(char const*)` | 0x2d700 | 520 | MainUI.cpp |
| `FileBrowserMenu::~FileBrowserMenu()` | 0x2d908 | 336 | MainUI.cpp |
| `FileBrowserMenu::~FileBrowserMenu()` | 0x2da58 | 52 | MainUI.cpp |
| `FileBrowserMenu::count()` | 0x2da8c | 72 | MainUI.cpp |
| `FileBrowserMenu::onSelect()` | 0x2dad4 | 184 | MainUI.cpp |
| `FileBrowserMenu::getItem(int)` | 0x2db8c | 136 | MainUI.cpp |
| `FileBrowserMenu::itemIcon(TextItem*)` | 0x2dc14 | 168 | MainUI.cpp |
| `FileBrowserMenu::drawList(SDL_Surface*)` | 0x2dcbc | 1076 | MainUI.cpp |
| `FileBrowserMenu::handleKey()` | 0x2e0f0 | 552 | MainUI.cpp |
| `freeSurfaceCallback(SDL_Surface*)` | 0x2e318 | 44 | MainUI.cpp |
| `LoadSurfaceWorker::run()` | 0x2e344 | 168 | MainUI.cpp |
| `LoadSurfaceWorker::finished()` | 0x2e3ec | 56 | MainUI.cpp |
| `MenuWindow::MenuWindow(int, int, void*)` | 0x2e424 | 688 | MainUI.cpp |
| `MenuWindow::MenuWindow(int, int, ListMenu*)` | 0x2e6d4 | 676 | MainUI.cpp |
| `MenuWindow::MenuWindow(char const*, int, void*)` | 0x2e978 | 708 | MainUI.cpp |
| `MenuWindow::~MenuWindow() (complete)` | 0x2ec3c | 316 | MainUI.cpp |
| `MenuWindow::~MenuWindow() (deleting)` | 0x2ed78 | 52 | MainUI.cpp |
| `MenuWindow::saveState(cJSON*)` | 0x2edac | 544 | MainUI.cpp |
| `MenuWindow::restoreState(cJSON*)` | 0x2efcc | 708 | MainUI.cpp |
| `MenuWindow::onShow()` | 0x2f290 | 1076 | MainUI.cpp |
| `MenuWindow::updateThumbnail()` | 0x2f6c4 | 1104 | MainUI.cpp |
| `MenuWindow::update()` | 0x2fb14 | 352 | MainUI.cpp |
| `MenuWindow::onHide()` | 0x2fc74 | 656 | MainUI.cpp |
| `MenuWindow::handleInput()` | 0x2ff04 | 4520 | MainUI.cpp |
| `MenuWindow::cacheBackground()` | 0x310ac | 328 | MainUI.cpp |
| `drawPageCount(SDL_Surface*, int, int)` | 0x311f4 | 420 | MainUI.cpp |
| `MenuWindow::draw(SDL_Surface*)` | 0x31398 | 2876 | MainUI.cpp |
| `MenuWindow::setHeaderIcon(SDL_Surface*, char const*, bool, bool)` | 0x31ed4 | 364 | MainUI.cpp |
| `FloatWindow::FloatWindow(int)` | 0x32040 | 84 | MainUI.cpp |
| `FloatWindow::~FloatWindow()` | 0x32094 | 92 | MainUI.cpp |
| `FloatWindow::~FloatWindow()` | 0x320f0 | 52 | MainUI.cpp |
| `FloatWindow::draw(SDL_Surface*)` | 0x32124 | 60 | MainUI.cpp |
| `FloatWindow::onShow()` | 0x32160 | 252 | MainUI.cpp |
| `PopUpWindow::PopUpWindow(int, TextMenu*, char const*)` | 0x3225c | 544 | MainUI.cpp |
| `PopUpWindow::~PopUpWindow()` | 0x3247c | 184 | MainUI.cpp |
| `PopUpWindow::~PopUpWindow() (deleting)` | 0x32534 | 52 | MainUI.cpp |
| `PopUpWindow::onHide()` | 0x32568 | 32 | MainUI.cpp |
| `PopUpWindow::draw(SDL_Surface*)` | 0x32588 | 508 | MainUI.cpp |
| `PopUpWindow::handleInput()` | 0x32784 | 96 | MainUI.cpp |
| `NotifyWindow::NotifyWindow(char const*, char const*, char const*, void (*)())` | 0x327e4 | 316 | MainUI.cpp |
| `NotifyWindow::~NotifyWindow()` | 0x32920 | 172 | MainUI.cpp |
| `NotifyWindow::~NotifyWindow() (deleting)` | 0x329cc | 52 | MainUI.cpp |
| `notifyTimeout(unsigned int, void*)` | 0x32a00 | 160 | MainUI.cpp |
| `NotifyWindow::onShow()` | 0x32aa0 | 620 | MainUI.cpp |
| `NotifyWindow::handleInput()` | 0x32d0c | 104 | MainUI.cpp |
| `NotifyWindow::draw(SDL_Surface*)` | 0x32d74 | 748 | MainUI.cpp |
| `CountDownWindow::CountDownWindow(int, void (*)(), void (*)())` | 0x33060 | 156 | MainUI.cpp |
| `CountDownWindow::~CountDownWindow()` | 0x330fc | 92 | MainUI.cpp |
| `CountDownWindow::~CountDownWindow() (deleting)` | 0x33158 | 52 | MainUI.cpp |
| `countdownTick(unsigned int, void*)` | 0x3318c | 96 | MainUI.cpp |
| `CountDownWindow::onShow()` | 0x331ec | 236 | MainUI.cpp |
| `CountDownWindow::handleInput()` | 0x332d8 | 276 | MainUI.cpp |
| `DialogWindow::DialogWindow(int, int, bool, Action*)` | 0x333ec | 388 | MainUI.cpp |
| `DialogWindow::~DialogWindow()` | 0x33570 | 192 | MainUI.cpp |
| `DialogWindow::~DialogWindow() (deleting)` | 0x33630 | 52 | MainUI.cpp |
| `DialogWindow::onShow()` | 0x33664 | 720 | MainUI.cpp |
| `DialogWindow::drawFallback(SDL_Surface*)` | 0x33934 | 632 | MainUI.cpp |
| `DialogWindow::draw(SDL_Surface*)` | 0x33bac | 1120 | MainUI.cpp |
| `DialogWindow::handleInput()` | 0x3400c | 296 | MainUI.cpp |
| `progressThread(void*)` | 0x34134 | 80 | MainUI.cpp |
| `ProgressDialog::ProgressDialog(int, Action*)` | 0x34184 | 296 | MainUI.cpp |
| `ProgressDialog::~ProgressDialog()` | 0x342ac | 208 | MainUI.cpp |
| `ProgressDialog::~ProgressDialog() (deleting)` | 0x3437c | 52 | MainUI.cpp |
| `ProgressDialog::onShow()` | 0x343b0 | 68 | MainUI.cpp |
| `ProgressDialog::runAction()` | 0x343f4 | 68 | MainUI.cpp |
| `ProgressDialog::onHide()` | 0x34438 | 48 | MainUI.cpp |
| `ProgressDialog::draw(SDL_Surface*)` | 0x34468 | 1416 | MainUI.cpp |
| `ProgressDialog::handleInput()` | 0x349f0 | 36 | MainUI.cpp |
| `GameDetailWindow::GameDetailWindow(int, int)` | 0x34a14 | 264 | MainUI.cpp |
| `GameDetailWindow::GameDetailWindow(int, int, char const*, char const*, char const*, char const*, int, int, bool)` | 0x34b1c | 496 | MainUI.cpp |
| `GameDetailWindow::~GameDetailWindow()` | 0x34d0c | 140 | MainUI.cpp |
| `GameDetailWindow::~GameDetailWindow() (deleting)` | 0x34d98 | 52 | MainUI.cpp |
| `GameDetailWindow::onHide()` | 0x34dcc | 160 | MainUI.cpp |
| `GameDetailWindow::onShow()` | 0x34e6c | 280 | MainUI.cpp |
| `GameDetailWindow::update()` | 0x34f84 | 252 | MainUI.cpp |
| `GameDetailWindow::loadInfo()` | 0x35080 | 800 | MainUI.cpp |
| `GameDetailWindow::resetDetails(char const*, char const*, char const*, char const*, int, int, bool)` | 0x353a0 | 408 | MainUI.cpp |
| `GameDetailWindow::resetFromItem(TextItem*, int)` | 0x35538 | 428 | MainUI.cpp |
| `GameDetailWindow::draw(SDL_Surface*)` | 0x356e4 | 1240 | MainUI.cpp |
| `GameDetailWindow::handleInput()` | 0x35bbc | 1064 | MainUI.cpp |
| `GameDetailWindow::saveState(cJSON*)` | 0x35fe4 | 348 | MainUI.cpp |
| `GameDetailWindow::restoreState(cJSON*)` | 0x36140 | 328 | MainUI.cpp |
| `GameDetailWindow::animateIn(SDL_Surface*, void (*)())` | 0x36288 | 684 | MainUI.cpp |
| `GameDetailWindow::animateOut(SDL_Surface*, void (*)())` | 0x36534 | 552 | MainUI.cpp |
| `GameDetailWindow::captureBackground(SDL_Surface*)` | 0x3675c | 124 | MainUI.cpp |
| `detailTimeout(unsigned int, void*)` | 0x367d8 | 96 | MainUI.cpp |
| `ADCInfoWindow::ADCInfoWindow(int, int)` | 0x36838 | 128 | MainUI.cpp |
| `ADCInfoWindow::~ADCInfoWindow()` | 0x368b8 | 92 | MainUI.cpp |
| `ADCInfoWindow::~ADCInfoWindow() (deleting)` | 0x36914 | 52 | MainUI.cpp |
| `ADCInfoWindow::handleInput()` | 0x36948 | 72 | MainUI.cpp |
| `ADCInfoWindow::draw(SDL_Surface*)` | 0x36990 | 472 | MainUI.cpp |
| `clearWindows(std::vector<TrimusWindow*, std::allocator<TrimusWindow*> >*)` | 0x36b68 | 184 | MainUI.cpp |
| `flipScreen()` | 0x36c20 | 56 | MainUI.cpp |
| `updateScreen()` | 0x36c58 | 96 | MainUI.cpp |
| `flushPendingWindows()` | 0x36cb8 | 208 | MainUI.cpp |
| `pushWindow(TrimusWindow*)` | 0x36d88 | 52 | MainUI.cpp |
| `initFramebuffer()` | 0x36dbc | 244 | MainUI.cpp |
| `closeFramebuffer()` | 0x36eb0 | 56 | MainUI.cpp |
| `runWindowLoop(std::vector<TrimusWindow*, std::allocator<TrimusWindow*> >*, std::atomic<bool>*)` | 0x36ee8 | 1244 | MainUI.cpp |
| `createRootWindow(std::vector<TrimusWindow*, std::allocator<TrimusWindow*> >*)` | 0x373c4 | 248 | MainUI.cpp |
| `windowTitle(int)` | 0x374bc | 340 | MainUI.cpp |
| `createRomListMenu(EmuInfo*, int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)` | 0x37610 | 336 | MainUI.cpp |
| `smallMenuFont()` | 0x37760 | 36 | MainUI.cpp |
| `createMenu(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)` | 0x37784 | 4272 | MainUI.cpp |
| `enableSwapAction(void*)` | 0x38834 | 52 | MainUI.cpp |
| `startBackgroundThread()` | 0x38868 | 64 | MainUI.cpp |
| `stopBackgroundThread()` | 0x388a8 | 72 | MainUI.cpp |
| `ensureSwap(void*)` | 0x388f0 | 228 | MainUI.cpp |
| `chargingThread(void*)` | 0x389d4 | 1572 | MainUI.cpp |
| `chargingScreenLoop()` | 0x38ff8 | 420 | MainUI.cpp |
| `__static_initialization_and_destruction_0(int, int)` | 0x3919c | 140 | MainUI.cpp |
| `_GLOBAL__sub_I_g_windowStack` | 0x39228 | 24 | MainUI.cpp |
| `TrimusWindow::animateIn(SDL_Surface*, FlipFn)` | 0x39478 | 40 | MainUI.cpp |
| `TrimusWindow::animateOut(SDL_Surface*, FlipFn)` | 0x394a0 | 40 | MainUI.cpp |
| `TrimusWindow::getId()` | 0x394c8 | 40 | MainUI.cpp |
| `onSIGTERM(int)` | 0x49c3c | 44 | Main.cpp |
| `appExit(int)` | 0x49c68 | 88 | Main.cpp |
| `pollBatteryStatus()` | 0x49cc0 | 356 | Main.cpp |
| `pollWifiStatus()` | 0x49e24 | 256 | Main.cpp |
| `pollVolumeChange()` | 0x49f24 | 300 | Main.cpp |
| `statusPollThread(void*)` | 0x4a050 | 184 | Main.cpp |
| `pushHotplugEvent(int, int)` | 0x4a108 | 104 | Main.cpp |
| `hotplugThread(void*)` | 0x4a170 | 44 | Main.cpp |
| `wifiStatusCallback(WifiStatus*)` | 0x4a19c | 352 | Main.cpp |
| `main` | 0x4a2fc | 2648 | Main.cpp |
| `ringAt(RingNode*, int)` | 0x4c418 | 180 | RingList.cpp |
| `freeRing(RingNode*, void (*)(void*))` | 0x4c4cc | 140 | RingList.cpp |
| `insertRingSorted(RingNode*, void*, int (*)(void*, void*), int)` | 0x4c558 | 472 | RingList.cpp |
| `insertRing(RingNode*, void*, int (*)(void*, void*))` | 0x4c730 | 60 | RingList.cpp |
| `insertRingUnique(RingNode*, void*, int (*)(void*, void*))` | 0x4c76c | 60 | RingList.cpp |
| `ringCount(RingNode*)` | 0x4c7a8 | 120 | RingList.cpp |
| `appendRing(RingNode*, void*)` | 0x4c820 | 200 | RingList.cpp |
| `prependRing(RingNode*, void*)` | 0x4c8e8 | 192 | RingList.cpp |
| `removeRingNode(RingNode*, RingNode*)` | 0x4c9a8 | 312 | RingList.cpp |
| `forEachRing(RingNode*, void (*)(void*))` | 0x4cae0 | 136 | RingList.cpp |
| `readTestInput()` | 0x4cb68 | 380 | HardwareTests.cpp |
| `SDLKeyTestWindow::SDLKeyTestWindow(int const*, int, int (*)(), char const*, int)` | 0x4cce4 | 164 | HardwareTests.cpp |
| `SDLKeyTestWindow::handleInput()` | 0x4cd88 | 308 | HardwareTests.cpp |
| `SDLKeyTestWindow::keyName(int)` | 0x4cebc | 248 | HardwareTests.cpp |
| `drawHighlightedText(SDL_Surface*, void*, SDL_Rect, char const*, SDL_Color, int, int, int)` | 0x4cfb4 | 1416 | HardwareTests.cpp |
| `SDLKeyTestWindow::draw(SDL_Surface*)` | 0x4d53c | 444 | HardwareTests.cpp |
| `testKeyTimer(unsigned int, void*)` | 0x4d6f8 | 212 | HardwareTests.cpp |
| `SDLKeyTestWindow::restartTimer()` | 0x4d7cc | 92 | HardwareTests.cpp |
| `SDLKeyTestWindow::onHide()` | 0x4d828 | 72 | HardwareTests.cpp |
| `SDLKeyTestWindow::onShow()` | 0x4d870 | 36 | HardwareTests.cpp |
| `TestKeyImage::TestKeyImage(char const*, char const*)` | 0x4d894 | 168 | HardwareTests.cpp |
| `TestKeyImage::~TestKeyImage()` | 0x4d93c | 96 | HardwareTests.cpp |
| `KeyTestWindow::KeyTestWindow(int const*, int, int (*)(), char const*, int)` | 0x4d99c | 408 | HardwareTests.cpp |
| `KeyTestWindow::keyName(int)` | 0x4db34 | 272 | HardwareTests.cpp |
| `KeyTestWindow::expecting(int)` | 0x4dc44 | 84 | HardwareTests.cpp |
| `KeyTestWindow::draw(SDL_Surface*)` | 0x4dc98 | 1324 | HardwareTests.cpp |
| `KeyTestWindow2::KeyTestWindow2()` | 0x4e1c4 | 108 | HardwareTests.cpp |
| `KeyTestWindow2::~KeyTestWindow2()` | 0x4e230 | 316 | HardwareTests.cpp |
| `KeyTestWindow2::loadMasks()` | 0x4e3a0 | 572 | HardwareTests.cpp |
| `KeyTestWindow2::mapKey(SDLKey)` | 0x4e5dc | 408 | HardwareTests.cpp |
| `KeyTestWindow2::handleInput()` | 0x4e774 | 372 | HardwareTests.cpp |
| `keyMask(KeyTestWindow2*, int)` | 0x4e8e8 | 212 | HardwareTests.cpp |
| `keyMaskRect(KeyTestWindow2*, int, SDL_Rect*)` | 0x4e9bc | 1084 | HardwareTests.cpp |
| `drawKeyTestFooter(SDL_Surface*)` | 0x4edf8 | 312 | HardwareTests.cpp |
| `KeyTestWindow2::draw(SDL_Surface*)` | 0x4ef30 | 476 | HardwareTests.cpp |
| `LCDTestWindow::LCDTestWindow()` | 0x4f10c | 192 | HardwareTests.cpp |
| `LCDTestWindow::handleInput()` | 0x4f1cc | 304 | HardwareTests.cpp |
| `LCDTestWindow::draw(SDL_Surface*)` | 0x4f2fc | 328 | HardwareTests.cpp |
| `LCDTestWindow::onHide()` | 0x4f444 | 32 | HardwareTests.cpp |
| `LCDTestWindow::onShow()` | 0x4f464 | 32 | HardwareTests.cpp |
| `SoundTestWindow::SoundTestWindow()` | 0x4f484 | 92 | HardwareTests.cpp |
| `SoundTestWindow::handleInput()` | 0x4f4e0 | 168 | HardwareTests.cpp |
| `SoundTestWindow::draw(SDL_Surface*)` | 0x4f588 | 616 | HardwareTests.cpp |
| `soundTestTimer(unsigned int, void*)` | 0x4f7f0 | 112 | HardwareTests.cpp |
| `SoundTestWindow::playNext()` | 0x4f860 | 280 | HardwareTests.cpp |
| `SoundTestWindow::onHide()` | 0x4f978 | 204 | HardwareTests.cpp |
| `SoundTestWindow::onShow()` | 0x4fa44 | 288 | HardwareTests.cpp |
| `readUsbTestInput()` | 0x4fb64 | 112 | HardwareTests.cpp |
| `SDCardTestWindow::SDCardTestWindow()` | 0x4fbd4 | 164 | HardwareTests.cpp |
| `SDCardTestWindow::handleInput()` | 0x4fc78 | 180 | HardwareTests.cpp |
| `SDCardTestWindow::draw(SDL_Surface*)` | 0x4fd2c | 684 | HardwareTests.cpp |
| `SDCardTestWindow::testWrite()` | 0x4ffd8 | 564 | HardwareTests.cpp |
| `SDCardTestWindow::testRead()` | 0x5020c | 668 | HardwareTests.cpp |
| `SDCardTestWindow::onHide()` | 0x504a8 | 64 | HardwareTests.cpp |
| `sdCardTestThread(void*)` | 0x504e8 | 68 | HardwareTests.cpp |
| `SDCardTestWindow::onShow()` | 0x5052c | 56 | HardwareTests.cpp |
| `TestMenuItem::TestMenuItem(char const*, char const*)` | 0x50564 | 192 | HardwareTests.cpp |
| `TestMenuItem::~TestMenuItem()` | 0x50624 | 80 | HardwareTests.cpp |
| `HardwareTestMenu::HardwareTestMenu()` | 0x50674 | 352 | HardwareTests.cpp |
| `HardwareTestMenu::~HardwareTestMenu()` | 0x507d4 | 68 | HardwareTests.cpp |
| `HardwareTestMenu::onSelect()` | 0x5084c | 1180 | HardwareTests.cpp |
| `testResultLabel(int)` | 0x50ce8 | 88 | HardwareTests.cpp |
| `testResultColor(int)` | 0x50d40 | 148 | HardwareTests.cpp |
| `HardwareTestMenu::drawList(SDL_Surface*)` | 0x50dd4 | 1468 | HardwareTests.cpp |
| `HardwareTestMenu::freeListItem()` | 0x51390 | 196 | HardwareTests.cpp |
| `HardwareTestMenu::initializeTests()` | 0x51454 | 540 | HardwareTests.cpp |
| `scalePixels(int, int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, int, int*, int*)` | 0x11cdc8 | 376 | Config.cpp |
| `sdCardSpace[abi:cxx11]()` | 0x11cf40 | 600 | Config.cpp |
| `sdCardFreeMB()` | 0x11d198 | 332 | Config.cpp |
| `isMounted(char const*)` | 0x11d2e4 | 220 | Config.cpp |
| `sdCardMounted()` | 0x11d3c0 | 32 | Config.cpp |
| `moduleLoaded(char const*)` | 0x11d3e0 | 240 | Config.cpp |
| `isSpinand()` | 0x11d4d0 | 52 | Config.cpp |
| `fileExists(char const*, char const*, int)` | 0x11d504 | 400 | Config.cpp |
| `loadPlatformPaths()` | 0x11d694 | 2316 | Config.cpp |
| `dualPartition()` | 0x11dfa0 | 140 | Config.cpp |
| `copyNeoGeo(char const*)` | 0x11e02c | 196 | Config.cpp |
| `platformPath(int)` | 0x11e0f0 | 304 | Config.cpp |
| `verifyGameList(char const*, char const*)` | 0x11e220 | 140 | Config.cpp |
| `mountP2Ext()` | 0x11e2ac | 228 | Config.cpp |
| `removeAllDBFiles()` | 0x11e390 | 5028 | Config.cpp |
| `removeEmulatorConfigs()` | 0x11f734 | 92 | Config.cpp |
| `initSdCardFolders()` | 0x11f790 | 2072 | Config.cpp |
| `swapSize()` | 0x11ffa8 | 180 | Config.cpp |
| `enableSwap()` | 0x12005c | 248 | Config.cpp |
| `maxCpuMHz()` | 0x120154 | 140 | Config.cpp |
| `memoryMB()` | 0x1201e0 | 176 | Config.cpp |
| `removeMax(std::__cxx11::list<int, std::allocator<int> >*)` | 0x120290 | 276 | Config.cpp |
| `removeMin(std::__cxx11::list<int, std::allocator<int> >*)` | 0x1203a4 | 288 | Config.cpp |
| `addRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*, char const*, char const*, char const*, char const*, int, bool)` | 0x1204c4 | 724 | Config.cpp |
| `removeRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*, char const*)` | 0x120798 | 264 | Config.cpp |
| `loadRecentList(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x1208a0 | 596 | Config.cpp |
| `saveRecentList(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x120af4 | 812 | Config.cpp |
| `clearRecent(std::__cxx11::list<RecentItem, std::allocator<RecentItem> >*)` | 0x120e20 | 56 | Config.cpp |
| `removeApp(std::vector<AppInfo, std::allocator<AppInfo> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x120e58 | 296 | Config.cpp |
| `findEmuByLaunch(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x120f80 | 244 | Config.cpp |
| `findEmuByLabelPrefix(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x121074 | 216 | Config.cpp |
| `findApp(std::vector<AppInfo, std::allocator<AppInfo> >*, char const*)` | 0x12114c | 224 | Config.cpp |
| `parseApp(void*, char const*, char const*, cJSON*)` | 0x12122c | 1208 | Config.cpp |
| `loadConfigFolder(void*, char const*, char const*, void (*)(void*, char const*, char const*, cJSON*))` | 0x1216e4 | 628 | Config.cpp |
| `loadAppFolder(std::vector<AppInfo, std::allocator<AppInfo> >*, char const*)` | 0x121958 | 64 | Config.cpp |
| `removeEmu(std::vector<EmuInfo, std::allocator<EmuInfo> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x121998 | 296 | Config.cpp |
| `parseEmu(void*, char const*, char const*, cJSON*)` | 0x121ac0 | 2480 | Config.cpp |
| `loadEmuFolder(std::vector<EmuInfo, std::allocator<EmuInfo> >*, char const*)` | 0x122470 | 64 | Config.cpp |
| `parseThemeColor(char const*, char const*, SDL_Color*)` | 0x1224b0 | 260 | Config.cpp |
| `parseFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, SDL_Color*)` | 0x1225b4 | 440 | Config.cpp |
| `parseListFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, SDL_Color*, SDL_Color*, SDL_Color*)` | 0x12276c | 640 | Config.cpp |
| `parseGridFont(char const*, cJSON*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, int*, int*, int*, int*, int*, SDL_Color*, SDL_Color*)` | 0x1229ec | 908 | Config.cpp |
| `parseTheme(void*, char const*, char const*, cJSON*)` | 0x122d78 | 1316 | Config.cpp |
| `loadThemeFolder(std::vector<Theme, std::allocator<Theme> >*, char const*)` | 0x12329c | 64 | Config.cpp |
| `findTheme(std::vector<Theme, std::allocator<Theme> >*, char const*)` | 0x1232dc | 212 | Config.cpp |
| `replaceAll(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x1233b0 | 148 | Config.cpp |
| `loadTestResults(int*, char*)` | 0x123444 | 532 | Config.cpp |
| `saveTestResults(int*)` | 0x123658 | 336 | Config.cpp |
| `getSerial(char*)` | 0x1237a8 | 176 | Config.cpp |
| `languageFontExists()` | 0x123858 | 84 | Config.cpp |
| `milliseconds()` | 0x1238ac | 92 | Config.cpp |
| `printSurfaceFormat(char const*, SDL_Surface*)` | 0x123908 | 304 | Config.cpp |
| `loadImage(char const*, SDL_Surface*, bool)` | 0x123a38 | 184 | Config.cpp |
| `loadSkinImage(char const*, char const*, char const*, SDL_Surface*, bool)` | 0x123af0 | 412 | Config.cpp |
| `readTextFile(char const*, char*, int)` | 0x123c8c | 156 | Config.cpp |
| `findProcess(char const*, int)` | 0x123d28 | 620 | Config.cpp |
| `openThemeFont(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&, char const*, char const*, int)` | 0x129f9c | 420 | Theme.cpp |
| `loadImeImage(char const*, char const*, char const*)` | 0x12a140 | 316 | Theme.cpp |
| `changeLanguageFonts(Theme*, int*)` | 0x12a27c | 1640 | Theme.cpp |
| `compositeThemeImage(SDL_Surface*, SDL_Surface*)` | 0x12a8e4 | 384 | Theme.cpp |
| `setDefaultTheme(Theme*)` | 0x12aa64 | 456 | Theme.cpp |
| `loadTheme(Theme*, int*)` | 0x12ac2c | 6864 | Theme.cpp |
| `freeTheme()` | 0x12c6fc | 1836 | Theme.cpp |
| `FileBrowserCache::FileBrowserCache(bool (*)(char const*, char const*))` | 0x12ce28 | 76 | FileBrowser.cpp |
| `FileBrowserCache::~FileBrowserCache()` | 0x12ce74 | 68 | FileBrowser.cpp |
| `FileBrowserCache::clear()` | 0x12ceb8 | 256 | FileBrowser.cpp |
| `compareFileItems(TextItem*, TextItem*)` | 0x12cfb8 | 148 | FileBrowser.cpp |
| `insertFileItem(std::vector<void*, std::allocator<void*> >*, TextItem*)` | 0x12d04c | 320 | FileBrowser.cpp |
| `countDirectory(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool (*)(char const*, char const*), bool, char*)` | 0x12d18c | 628 | FileBrowser.cpp |
| `countDirectoryItems(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool (*)(char const*, char const*))` | 0x12d400 | 180 | FileBrowser.cpp |
| `loadDirectory(std::vector<void*, std::allocator<void*> >*, char const*, bool (*)(char const*, char const*), int, int)` | 0x12d4b4 | 792 | FileBrowser.cpp |
| `FileBrowserCache::freeItems(std::vector<void*, std::allocator<void*> >*)` | 0x12d7cc | 164 | FileBrowser.cpp |
| `FileBrowserCache::totalItems()` | 0x12d870 | 168 | FileBrowser.cpp |
| `FileBrowserCache::trim()` | 0x12d918 | 272 | FileBrowser.cpp |
| `FileBrowserCache::find(char const*)` | 0x12da28 | 464 | FileBrowser.cpp |
| `FileBrowserCache::open(char const*)` | 0x12dbf8 | 320 | FileBrowser.cpp |
| `FileBrowserCache::parent()` | 0x12dd38 | 504 | FileBrowser.cpp |
| `FileBrowserCache::child(char const*)` | 0x12df30 | 420 | FileBrowser.cpp |
| `FileAction::FileAction(char const*, int)` | 0x12e0d4 | 156 | FileBrowser.cpp |
| `zerror(char const*)` | 0x1302cc | 40 | stb_zlib.cpp |
| `bit_reverse16(int)` | 0x1302f4 | 188 | stb_zlib.cpp |
| `bit_reverse(int, int)` | 0x1303b0 | 100 | stb_zlib.cpp |
| `zbuild_huffman(zhuffman*, unsigned char*, int)` | 0x130414 | 1016 | stb_zlib.cpp |
| `zget8(zbuf*)` | 0x13080c | 172 | stb_zlib.cpp |
| `fill_bits(zbuf*)` | 0x1308b8 | 172 | stb_zlib.cpp |
| `zreceive(zbuf*, int)` | 0x130964 | 144 | stb_zlib.cpp |
| `zhuffman_decode(zbuf*, zhuffman*)` | 0x1309f4 | 540 | stb_zlib.cpp |
| `compute_huffman_codes(zbuf*)` | 0x130c10 | 992 | stb_zlib.cpp |
| `init_default_huffman()` | 0x130ff0 | 304 | stb_zlib.cpp |
| `historyWrite(zbuf*, unsigned char)` | 0x131120 | 108 | stb_zlib.cpp |
| `historyRead(zbuf*, int)` | 0x13118c | 100 | stb_zlib.cpp |
| `parse_zlib_header(zbuf*)` | 0x1311f0 | 232 | stb_zlib.cpp |
| `parse_huffman_block(zbuf*, int)` | 0x1312d8 | 876 | stb_zlib.cpp |
| `parse_uncompressed_block(zbuf*, int)` | 0x131644 | 352 | stb_zlib.cpp |
| `parse_header(zbuf*)` | 0x1317a4 | 708 | stb_zlib.cpp |
| `openZlib(unsigned char*, FILE*, int)` | 0x131a68 | 252 | stb_zlib.cpp |
| `readZlib(zbuf*, void*, int)` | 0x131b64 | 388 | stb_zlib.cpp |
| `stbi_zlib_decode_malloc(unsigned char*, int, int*)` | 0x131ce8 | 332 | stb_zlib.cpp |
| `LruCache::LruCache(void (*)(SDL_Surface*))` | 0x131e34 | 96 | MenuHelpers.cpp |
| `LruCache::~LruCache()` | 0x131e94 | 68 | MenuHelpers.cpp |
| `LruCache::clear()` | 0x131ed8 | 216 | MenuHelpers.cpp |
| `LruCache::put(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, void*)` | 0x131fb0 | 300 | MenuHelpers.cpp |
| `LruCache::get(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)` | 0x1320dc | 384 | MenuHelpers.cpp |
| `LruCache::count()` | 0x13225c | 44 | MenuHelpers.cpp |
| `workerThread(void*)` | 0x132288 | 344 | MenuHelpers.cpp |
| `WorkerQueue::WorkerQueue(int)` | 0x1323e0 | 188 | MenuHelpers.cpp |
| `WorkerQueue::~WorkerQueue()` | 0x13249c | 308 | MenuHelpers.cpp |
| `WorkerQueue::addWorker(BGWorker*)` | 0x1325d0 | 240 | MenuHelpers.cpp |
| `WorkerQueue::takeWorker()` | 0x1326c0 | 160 | MenuHelpers.cpp |
| `genShortLangFile(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x134080 | 836 | Language.cpp |
| `createEnglishLangFile()` | 0x1343c4 | 420 | Language.cpp |
| `readLanguageFile(char const*, Language&)` | 0x134568 | 664 | Language.cpp |
| `loadLanguages(char const*)` | 0x134800 | 1844 | Language.cpp |
| `languageChoices()` | 0x134f34 | 540 | Language.cpp |
| `languageString(int)` | 0x135150 | 300 | Language.cpp |
| `openInputDevice(char const*)` | 0x1376fc | 744 | KeySettingWindow.cpp |
| `touchInputThread(void*)` | 0x1379e4 | 988 | KeySettingWindow.cpp |
| `startTouchInput()` | 0x137dc0 | 64 | KeySettingWindow.cpp |
| `stopTouchInput()` | 0x137e00 | 56 | KeySettingWindow.cpp |
| `waitMappingKey()` | 0x137e38 | 280 | KeySettingWindow.cpp |
| `KeySettingWindow::KeySettingWindow()` | 0x137f50 | 248 | KeySettingWindow.cpp |
| `KeySettingWindow::~KeySettingWindow()` | 0x138048 | 216 | KeySettingWindow.cpp |
| `KeySettingWindow::~KeySettingWindow()` | 0x138120 | 52 | KeySettingWindow.cpp |
| `KeySettingWindow::handleInput()` | 0x138154 | 948 | KeySettingWindow.cpp |
| `drawKeySettingFooter(SDL_Surface*, SDL_Surface*)` | 0x138508 | 236 | KeySettingWindow.cpp |
| `KeySettingWindow::draw(SDL_Surface*)` | 0x1385f4 | 380 | KeySettingWindow.cpp |
| `KeySettingWindow::loadResources()` | 0x138770 | 952 | KeySettingWindow.cpp |
| `KeySettingWindow::onHide()` | 0x138ba4 | 32 | KeySettingWindow.cpp |
| `KeySettingWindow::onShow()` | 0x138bc4 | 32 | KeySettingWindow.cpp |
| `KeySettingWindow::saveState(cJSON*)` | 0x138be4 | 36 | KeySettingWindow.cpp |
| `KeySettingWindow::restoreState(cJSON*)` | 0x138c08 | 40 | KeySettingWindow.cpp |
| `KeySettingWindow::update()` | 0x138c30 | 32 | KeySettingWindow.cpp |
| `wifiMode()` | 0x138c50 | 116 | WifiMenus.cpp |
| `wlanAddress(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)` | 0x138cc4 | 304 | WifiMenus.cpp |
| `switch2WpaMode(int)` | 0x138df4 | 288 | WifiMenus.cpp |
| `swicth2ApMode()` | 0x138f14 | 320 | WifiMenus.cpp |
| `WifiOnOffLabel::WifiOnOffLabel(Action*, bool)` | 0x139054 | 204 | WifiMenus.cpp |
| `WifiOnOffLabel::~WifiOnOffLabel()` | 0x139120 | 124 | WifiMenus.cpp |
| `WifiOnOffLabel::~WifiOnOffLabel()` | 0x13919c | 52 | WifiMenus.cpp |
| `WifiOnOffLabel::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x1391d0 | 936 | WifiMenus.cpp |
| `WifiLabel::WifiLabel(Action*)` | 0x139578 | 68 | WifiMenus.cpp |
| `WifiLabel::~WifiLabel()` | 0x1395bc | 60 | WifiMenus.cpp |
| `WifiLabel::~WifiLabel()` | 0x1395f8 | 52 | WifiMenus.cpp |
| `WifiLabel::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x13962c | 224 | WifiMenus.cpp |
| `WifiItem::draw(SDL_Surface*, SDL_Rect*, bool)` | 0x13970c | 904 | WifiMenus.cpp |
| `WifiConnectAction::execute(void*)` | 0x139a94 | 544 | WifiMenus.cpp |
| `WifiConnectDialogAction::execute(void*)` | 0x139cb4 | 160 | WifiMenus.cpp |
| `WifiItemAction::execute(void*)` | 0x139d54 | 428 | WifiMenus.cpp |
| `toggleWifi()` | 0x139f00 | 428 | WifiMenus.cpp |
| `WifiOnOffAction::execute(void*)` | 0x13a0ac | 448 | WifiMenus.cpp |
| `WifiOnOffDialogAction::execute(void*)` | 0x13a26c | 140 | WifiMenus.cpp |
| `WifiMenuDrawer::WifiMenuDrawer()` | 0x13a2f8 | 224 | WifiMenus.cpp |
| `WifiMenuDrawer::~WifiMenuDrawer()` | 0x13a3d8 | 116 | WifiMenus.cpp |
| `WifiMenuDrawer::~WifiMenuDrawer()` | 0x13a44c | 52 | WifiMenus.cpp |
| `WifiMenuDrawer::handleKey()` | 0x13a480 | 300 | WifiMenus.cpp |
| `WifiMenuDrawer::onSelect()` | 0x13a5ac | 184 | WifiMenus.cpp |
| `WifiMenuDrawer::getItem(int)` | 0x13a664 | 136 | WifiMenus.cpp |
| `WifiMenuDrawer::drawList(SDL_Surface*)` | 0x13a6ec | 1256 | WifiMenus.cpp |
| `WifiMenuDrawer::count()` | 0x13abd4 | 48 | WifiMenus.cpp |
| `WifiMenuDrawer::clearItems()` | 0x13ac04 | 308 | WifiMenus.cpp |
| `WifiMenu::WifiMenu()` | 0x13ad38 | 832 | WifiMenus.cpp |
| `WifiMenu::~WifiMenu()` | 0x13b078 | 260 | WifiMenus.cpp |
| `WifiMenu::~WifiMenu()` | 0x13b17c | 52 | WifiMenus.cpp |
| `WifiMenu::drawList(SDL_Surface*)` | 0x13b1b0 | 160 | WifiMenus.cpp |
| `WifiMenu::onSelect()` | 0x13b250 | 112 | WifiMenus.cpp |
| `WifiMenu::onUnhandledEvent(int)` | 0x13b2c0 | 2212 | WifiMenus.cpp |
| `HotspotSwitchAction::execute(void*)` | 0x13bb64 | 348 | WifiMenus.cpp |
| `writeConfigValue(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x13bcc0 | 488 | WifiMenus.cpp |
| `readConfigValue(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)` | 0x13bea8 | 524 | WifiMenus.cpp |
| `HotspotWriteAction::execute(void*)` | 0x13c0b4 | 232 | WifiMenus.cpp |
| `HotspotImePopupAction::execute(void*)` | 0x13c19c | 200 | WifiMenus.cpp |
| `HotspotMenu::HotspotMenu()` | 0x13c264 | 284 | WifiMenus.cpp |
| `HotspotMenu::~HotspotMenu()` | 0x13c380 | 100 | WifiMenus.cpp |
| `HotspotMenu::~HotspotMenu()` | 0x13c3e4 | 52 | WifiMenus.cpp |
| `HotspotMenu::drawList(SDL_Surface*)` | 0x13c418 | 48 | WifiMenus.cpp |
| `HotspotMenu::onUnhandledEvent(int)` | 0x13c448 | 40 | WifiMenus.cpp |
| `ImeWindow::ImeWindow(int title, Action *callback, int label)` | 0x13edfc | 1060 | ImeWindow.cpp |
| `ImeWindow::~ImeWindow()` | 0x13f220 | 688 | ImeWindow.cpp |
| `ImeWindow::~ImeWindow() [deleting]` | 0x13f4d0 | 52 | ImeWindow.cpp |
| `void ImeWindow::rowKeys(std::vector<int>&, int key)` | 0x13f504 | 460 | ImeWindow.cpp |
| `void ImeWindow::columnKeys(std::vector<int>&, int key, bool up)` | 0x13f6d0 | 404 | ImeWindow.cpp |
| `void ImeWindow::moveHorizontal(bool left)` | 0x13f864 | 652 | ImeWindow.cpp |
| `void ImeWindow::moveVertical(bool up)` | 0x13faf0 | 652 | ImeWindow.cpp |
| `int ImeWindow::handleInput()` | 0x13fd7c | 1716 | ImeWindow.cpp |
| `void ImeWindow::drawKeys(SDL_Surface*, int x, int y)` | 0x140430 | 1756 | ImeWindow.cpp |
| `int ImeWindow::draw(SDL_Surface*)` | 0x140b0c | 328 | ImeWindow.cpp |
| `void ImeWindow::setKeyIcons(int key, SDL_Surface *icon, SDL_Surface *hint)` | 0x140c54 | 244 | ImeWindow.cpp |
| `void ImeWindow::initKey(ImeKey&, int id, SDL_Surface *focused, SDL_Surface *normal, SDL_Surface *icon, SDL_Surface *hint)` | 0x140d48 | 3716 | ImeWindow.cpp |
| `void ImeWindow::buildKeys()` | 0x141bcc | 1036 | ImeWindow.cpp |
| `void ImeWindow::onHide()` | 0x141fd8 | 32 | ImeWindow.cpp |
| `void ImeWindow::onShow()` | 0x141ff8 | 32 | ImeWindow.cpp |
| `void ImeWindow::saveState(cJSON*)` | 0x142018 | 36 | ImeWindow.cpp |
| `int ImeWindow::restoreState(cJSON*)` | 0x14203c | 40 | ImeWindow.cpp |
| `void ImeWindow::update()` | 0x142064 | 32 | ImeWindow.cpp |
| `int parsePinyinLine(tiny_utf8::string, char32_t*, std::string&)` | 0x143720 | 512 | Pinyin.cpp |
| `void loadPinyinTable()` | 0x143920 | 452 | Pinyin.cpp |
| `char* pinyinInitials(const char*)` | 0x143ae4 | 528 | Pinyin.cpp |
| `int pinyinFind(const char*, const char*, const char*, const char**, int*)` | 0x143cf4 | 308 | Pinyin.cpp |
| `deleteTextItem(SDL_Surface*)` | 0x1484d4 | 68 | GlobalSearch.cpp |
| `freeData(SDL_Surface*)` | 0x148518 | 44 | GlobalSearch.cpp |
| `lastPathComponent(char*)` | 0x148544 | 184 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::MultiDBCachedTextMenu(std::string)` | 0x1485fc | 416 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::~MultiDBCachedTextMenu()` | 0x14879c | 256 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::~MultiDBCachedTextMenu() [deleting]` | 0x14889c | 52 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::openCache(EmuInfo&, char*, size_t)` | 0x1488d0 | 756 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::prepare()` | 0x148bc4 | 744 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::fetchForward()` | 0x148eac | 132 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::fetchBackward()` | 0x148f30 | 124 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::pageUp()` | 0x148fac | 60 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::pageDown()` | 0x148fe8 | 60 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::moveDown()` | 0x149024 | 60 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::moveUp()` | 0x149060 | 60 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::restorePosition(int, int, int)` | 0x14909c | 156 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::prepareBGWorkers(int, int)` | 0x149138 | 988 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::getItem(int)` | 0x149514 | 784 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::getItemData(int)` | 0x149824 | 816 | GlobalSearch.cpp |
| `MultiDBCachedTextMenu::count()` | 0x149b54 | 40 | GlobalSearch.cpp |
| `int unusedDeviceHook1()` | 0x14acfc | 28 | Device.c |
| `int exitDeviceHook()` | 0x14ad18 | 28 | Device.c |
| `int unusedDeviceHook2()` | 0x14ad34 | 28 | Device.c |
| `int setVolumeTinymix(int vol)` | 0x14ad50 | 152 | Device.c |
| `int unusedDeviceHook3()` | 0x14ade8 | 28 | Device.c |
| `int writeFile(const char *path, const char *text)` | 0x14ae04 | 112 | Device.c |
| `void initBacklightPwm()` | 0x14ae74 | 96 | Device.c |
| `int setBacklight(int level)` | 0x14aed4 | 336 | Device.c |
| `int backlightOff()` | 0x14b024 | 40 | Device.c |
| `bool isBacklightOff()` | 0x14b04c | 176 | Device.c |
| `bool bootedByPowerKey()` | 0x14b0fc | 156 | Device.c |
| `int readAxpStatus(int *battery, int *voltage, int *charging)` | 0x14b198 | 476 | Device.c |
| `int fdList_find(int fd)` | 0x14b374 | 120 | Device.c |
| `int fdList_add(int fd)` | 0x14b3ec | 120 | Device.c |
| `int fdList_remove(int fd)` | 0x14b464 | 216 | Device.c |
| `int hasDsp1(int card)` | 0x14b53c | 80 | Device.c |
| `int hotplugLoop(void (*cb)(int, int))` | 0x14b58c | 648 | Device.c |
| `int stopHotplug()` | 0x14b814 | 76 | Device.c |
| `long fileSize(FILE *f)` | 0x14b860 | 68 | Settings.c |
| `int readWholeFile(const char *name, FILE *f, char **out)` | 0x14b8a4 | 372 | Settings.c |
| `const char *getLanguageFile()` | 0x14ba18 | 64 | Settings.c |
| `void setLanguageFile(const char *lang)` | 0x14ba58 | 96 | Settings.c |
| `const char *getKeymap()` | 0x14bab8 | 64 | Settings.c |
| `void setKeymap(const char *keymap)` | 0x14baf8 | 108 | Settings.c |
| `void setupKeymap(const char *keymap)` | 0x14bb64 | 120 | Settings.c |
| `int getWifiOn()` | 0x14bbdc | 36 | Settings.c |
| `void setWifiOn(int on)` | 0x14bc00 | 84 | Settings.c |
| `int getHotspotOn()` | 0x14bc54 | 36 | Settings.c |
| `void setHotspotOn(int on)` | 0x14bc78 | 48 | Settings.c |
| `void loadSystemJson(void *shm, char **theme, int *fontSize)` | 0x14bca8 | 1976 | Settings.c |
| `void loadThemeAndFontSize(char **theme, int *fontSize)` | 0x14c460 | 412 | Settings.c |
| `void saveSystemJson(void *shm, const char *theme, int fontSize)` | 0x14c5fc | 1320 | Settings.c |
| `int readByte(FILE *f)` | 0x14cb24 | 84 | ZipReader.c |
| `unsigned char readU8(FILE *f)` | 0x14cb78 | 44 | ZipReader.c |
| `void skipBytes(FILE *f, int n)` | 0x14cba4 | 112 | ZipReader.c |
| `unsigned short readU16BE(FILE *f)` | 0x14cc14 | 80 | ZipReader.c |
| `unsigned int readU32BE(FILE *f)` | 0x14cc64 | 68 | ZipReader.c |
| `unsigned short readU16LE(FILE *f)` | 0x14cca8 | 80 | ZipReader.c |
| `unsigned int readU32LE(FILE *f)` | 0x14ccf8 | 72 | ZipReader.c |
| `void readFields(FILE *f, const char *fmt, ...)` | 0x14cd40 | 412 | ZipReader.c |
| `int findSignature(ZipFile *zip, const unsigned char *sig)` | 0x14cedc | 276 | ZipReader.c |
| `int zipReadCentralDir(ZipFile *zip)` | 0x14cff0 | 492 | ZipReader.c |
| `int zipFindEntry(ZipFile *zip, const char *name, unsigned int crc)` | 0x14d1dc | 572 | ZipReader.c |
| `ZipEntry *zipOpenEntry(ZipFile *zip, const char *name, unsigned int crc)` | 0x14d418 | 544 | ZipReader.c |
| `int zipRead(ZipEntry *e, void *buf, unsigned int len)` | 0x14d638 | 248 | ZipReader.c |
| `void *zipReadFile(ZipFile *zip, const char *name, unsigned int crc, unsigned int *size)` | 0x14d730 | 296 | ZipReader.c |
| `void zipCloseEntry(ZipEntry *e)` | 0x14d858 | 88 | ZipReader.c |
| `ZipFile *zipOpen(const char *path)` | 0x14d8b0 | 364 | ZipReader.c |
| `void zipClose(ZipFile *zip)` | 0x14da1c | 52 | ZipReader.c |
| `cJSON *loadLanguageStrings(const char *lang)` | 0x14da50 | 260 | BuiltinStrings.c |
| `void freeLanguageStrings(cJSON *strings)` | 0x14db54 | 48 | BuiltinStrings.c |
| `const char *getLanguageString(cJSON *strings, int id)` | 0x14db84 | 148 | BuiltinStrings.c |
| `unsigned char i2cRead(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len)` | 0x14dc18 | 540 | I2c.c |
| `unsigned char i2cWrite(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len)` | 0x14de34 | 356 | I2c.c |
| `void msleep(int ms)` | 0x14df98 | 48 | I2c.c |
| `unsigned char randomByte()` | 0x14dfc8 | 140 | I2c.c |
| `unsigned char waitForProbe()` | 0x14e054 | 120 | I2c.c |
| `void printThreadInfo(const char *name)` | 0x14e0cc | 76 | Wifi.c |
| `int wifiSignalLevel(int rssi)` | 0x14e118 | 108 | Wifi.c |
| `int readFileTo(const char *path, void *buf)` | 0x14e184 | 156 | Wifi.c |
| `int setWifiScanEnable(int on)` | 0x14e220 | 64 | Wifi.c |
| `int wifiConnectOpen(const char *ssid)` | 0x14e260 | 316 | Wifi.c |
| `int wifiConnectPsk(const char *ssid, const char *psk)` | 0x14e39c | 324 | Wifi.c |
| `void runUdhcpc()` | 0x14e4e0 | 72 | Wifi.c |
| `int wifiStartConnect()` | 0x14e528 | 60 | Wifi.c |
| `WifiStatus *getWifiStatus()` | 0x14e564 | 32 | Wifi.c |
| `unsigned char hexDigit(unsigned char v)` | 0x14e584 | 116 | Wifi.c |
| `unsigned char hexValue(unsigned char c)` | 0x14e5f8 | 168 | Wifi.c |
| `void unescapeSsid(char *s, int len)` | 0x14e6a0 | 388 | Wifi.c |
| `int parseScanLine(const char *text, WifiAP *ap)` | 0x14e824 | 680 | Wifi.c |
| `int isScannedSsid(const char *ssid)` | 0x14eacc | 208 | Wifi.c |
| `int copyScanResults(WifiAP *out, int max)` | 0x14eb9c | 140 | Wifi.c |
| `void parseScanResults()` | 0x14ec28 | 644 | Wifi.c |
| `void rx_wifi_do_scan()` | 0x14eeac | 236 | Wifi.c |
| `WifiStatus queryWifiStatus()` | 0x14ef98 | 1432 | Wifi.c |
| `void *wifiMonitorThread(void *arg)` | 0x14f530 | 260 | Wifi.c |
| `void startWifiMonitor(void (*callback)(WifiStatus *))` | 0x14f634 | 192 | Wifi.c |
| `void stopWifiMonitor()` | 0x14f6f4 | 60 | Wifi.c |
| `byte readDeviceKeys()` | 0x14f730 | 552 | DeviceAuth.c |
| `void invertSwapHalves(byte *data)` | 0x14f958 | 220 | DeviceAuth.c |
| `void xorSwapHalves(byte *data, byte *key)` | 0x14fa34 | 268 | DeviceAuth.c |
| `void rotateBytesRight(byte *data)` | 0x14fb40 | 188 | DeviceAuth.c |
| `void transformDeviceBlock(byte *input, byte *output, byte *seed, byte rounds, int bits, byte mode)` | 0x14fbfc | 1208 | DeviceAuth.c |
| `byte beginDeviceAuth()` | 0x1500b4 | 620 | DeviceAuth.c |
| `byte sendDeviceKey()` | 0x150320 | 484 | DeviceAuth.c |
| `byte authenticateDevice(byte *challenge, byte *response)` | 0x150504 | 1492 | DeviceAuth.c |

Embedded libraries, verified as whole objects by `scripts/verify-libs.sh`:

| Library | Range | Bytes |
| --- | --- | --- |
| SQLite 3.30.1 (C) | 0x57920–0x11cdc8 | 808,104 |
| cJSON 1.7.10 (compiled as C++) | 0x51b60–0x5791c | 23,996 |
