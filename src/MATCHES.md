# Byte-exact functions

Each compiles with Arm GNU Toolchain 8.2-2018.08, `-O0 -fsigned-char`, to the same bytes as
`MainUI-354-clean`. Only link-time words are ignored. CI re-checks all of them
(`scripts/run_matches.sh`). The list itself is `src/matches.tsv`; regenerate this table with
`python3 tools/progress.py --matches-md src/MATCHES.md`.

| Function | Address | Size | File |
| --- | --- | --- | --- |
| `writeCmdToRun(const char*)` | 0x17fc0 | 88 | writeCmdToRun.cpp |
| `TrimusWindow::TrimusWindow(int titleLabelId, int id)` | 0x1d7dc | 84 | TrimusWindow.cpp |
| `TrimusWindow::~TrimusWindow()` | 0x1d830 | 52 | TrimusWindow.cpp |
| `MenuWindow::MenuWindow(int, int, ListMenu*)` | 0x2e6d4 | 676 | MenuWindow.cpp |
| `MenuWindow::~MenuWindow() (complete)` | 0x2ec3c | 316 | MenuWindow.cpp |
| `MenuWindow::~MenuWindow() (deleting)` | 0x2ed78 | 52 | MenuWindow.cpp |
| `TrimusWindow::animateIn(SDL_Surface*, FlipFn)` | 0x39478 | 40 | TrimusWindow.cpp |
| `TrimusWindow::animateOut(SDL_Surface*, FlipFn)` | 0x394a0 | 40 | TrimusWindow.cpp |
| `TrimusWindow::getId()` | 0x394c8 | 40 | TrimusWindow.cpp |
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

Embedded libraries, verified as whole objects by `scripts/verify-libs.sh`:

| Library | Range | Bytes |
| --- | --- | --- |
| SQLite 3.30.1 (C) | 0x57920–0x11cdc8 | 808,104 |
| cJSON 1.7.10 (compiled as C++) | 0x51b60–0x5791c | 23,996 |
