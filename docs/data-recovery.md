# Referenced storage recovery

All 54 missing data names from the initial link audit are now resolved: 13 alias names were
reconciled and 41 storage definitions were added to the existing units. Code coverage remains
22 proven units / 689 individual matches; no extra code unit or linker alias was introduced.

The data audit checks **8,421 bytes across 53 declared/consumed storage extents** and **156 English
strings** against the hash-pinned original ELF. This is initial-storage verification, not a
claim that the final executable's addresses, object order or entire data sections already match.

Run `python3 tools/linkaudit.py`; inspect its `storage` and `storage_mismatches` fields in
`build/link-audit/audit.json`. Every manifest byte is compared without a relocation mask.
String-table entries must use ABS32 relocations that point to the correct NUL-terminated string;
the checker compares the contents and entry order, rather than claiming final pointer addresses.
BSS and COMMON are accepted only when the original storage is zero. Object symbol sizes and
section bounds are checked. Tests deliberately corrupt an initializer and a string to confirm
both failures are detected. CI runs the audit and the ARM fixtures; known linkage, call-target
and storage mismatches fail it even while SDK imports remain pending. No ARM code is executed.

The ledger includes the 41 new definitions and 12 existing canonical alias-provider objects.
The current emitted definitions comprise 31 BSS objects, six COMMON objects, twelve initialized
data objects and four read-only objects. Actual final placement/alignment/padding still needs
an executable linker layout; C COMMON alignment is not a recovered original address.

Two recorded extents are deliberately limited:

- `g_deviceId`: five bytes consumed by getSerial; the original full allocation size is unknown.
- `g_panelInfo`: the 116-byte prefix read by adjustColor, with the original `K101_IM2BVL` header
  and timing fields. The full SigmaStar SDK object extent and trailing layout are unknown.

These limits remain in source comments and manifest evidence. Bytes outside the referenced
extents were not fabricated. Other original data, padding and the initialization of globals
that were already defined before this pass remain part of final-image reconstruction.

The language table holds 156 pointer entries (624 bytes) and their original strings. Read-only
theme colors are stored outside their consuming TU so GCC keeps the original loads at -O0.
The authentication round tables preserve both 8-byte rounds; its keys/buffers and init flag
match original zero storage. DEFLATE tables retain the original unused zero entries.

| Symbol | Target address | Checked bytes | Check | Source |
| --- | --- | --- | --- | --- |
| g_skinSubdir | 0x151a48 | 3 | bytes | tu_015b20.cpp |
| g_themeGray | 0x161808 | 4 | bytes | tu_015b20.cpp |
| g_themeWhite | 0x16180c | 4 | bytes | tu_015b20.cpp |
| g_themeDim | 0x161810 | 4 | bytes | tu_015b20.cpp |
| g_fbFd | 0x17e488 | 4 | bytes | tu_015b20.cpp |
| g_panelInfo | 0x17e48c | 116 | bytes | tu_015b20.cpp |
| g_customDisplayTiming | 0x17e6ec | 1 | bytes | tu_015b20.cpp |
| g_lengthBase | 0x17f9c0 | 124 | bytes | tu_1302cc.cpp |
| g_lengthExtra | 0x17fa3c | 124 | bytes | tu_1302cc.cpp |
| g_distanceBase | 0x17fab8 | 128 | bytes | tu_1302cc.cpp |
| g_distanceExtra | 0x17fb38 | 128 | bytes | tu_1302cc.cpp |
| g_codeLengthOrder | 0x17fbb8 | 19 | bytes | tu_1302cc.cpp |
| g_imeKeyMap | 0x17fbd4 | 200 | bytes | tu_13edfc.cpp |
| g_defaultStrings | 0x17fc9c | 624 | cstring_table | tu_14da50.c |
| roundTableA | 0x17ff0c | 16 | bytes | tu_14f730.c |
| roundTableB | 0x17ff1c | 16 | bytes | tu_14f730.c |
| g_windowStack | 0x17ff38 | 4 | bytes | tu_015b20.cpp |
| g_deviceId | 0x17ff48 | 5 | bytes | tu_015b20.cpp |
| g_launchCmd | 0x17ff54 | 2048 | bytes | tu_015b20.cpp |
| g_countBuffer | 0x180754 | 256 | bytes | tu_015b20.cpp |
| g_loadingThread | 0x180854 | 4 | bytes | tu_015b20.cpp |
| g_fbVar | 0x18085c | 160 | bytes | tu_015b20.cpp |
| g_settingsDirty | 0x1808fc | 1 | bytes | tu_015b20.cpp |
| g_powerDownCount | 0x180900 | 4 | bytes | tu_015b20.cpp |
| g_powerSent | 0x180904 | 1 | bytes | tu_015b20.cpp |
| g_startHeld | 0x180905 | 1 | bytes | tu_015b20.cpp |
| g_selectHeld | 0x180906 | 1 | bytes | tu_015b20.cpp |
| g_statusIndicatorFlag | 0x180907 | 1 | bytes | tu_015b20.cpp |
| g_cwdBuf | 0x180908 | 4096 | bytes | tu_015b20.cpp |
| g_busy | 0x181908 | 4 | bytes | tu_015b20.cpp |
| g_progressCur | 0x18190c | 4 | bytes | tu_015b20.cpp |
| g_progressTotal | 0x181910 | 4 | bytes | tu_015b20.cpp |
| g_charging | 0x181914 | 1 | bytes | tu_015b20.cpp |
| g_iconFont | 0x182180 | 4 | bytes | tu_129f9c.cpp |
| g_emptyImage | 0x1821b4 | 4 | bytes | tu_129f9c.cpp |
| g_gameFocused | 0x1821c0 | 4 | bytes | tu_129f9c.cpp |
| g_gameNormal | 0x1821c4 | 4 | bytes | tu_129f9c.cpp |
| g_raRow | 0x1821d0 | 4 | bytes | tu_129f9c.cpp |
| g_previewBg | 0x1821fc | 4 | bytes | tu_129f9c.cpp |
| g_volumeIcons | 0x182214 | 84 | bytes | tu_129f9c.cpp |
| g_clearColor | 0x182278 | 4 | bytes | tu_129f9c.cpp |
| g_highlightColor | 0x18228c | 4 | bytes | tu_129f9c.cpp |
| g_selectedTextColor | 0x1822a0 | 4 | bytes | tu_129f9c.cpp |
| g_hintColor | 0x1822a8 | 4 | bytes | tu_129f9c.cpp |
| g_listColor | 0x1822ac | 4 | bytes | tu_129f9c.cpp |
| g_wlanStatusBuffer | 0x182418 | 128 | bytes | tu_138c50.cpp |
| g_imeInputColor | 0x182498 | 4 | bytes | tu_13edfc.cpp |
| deviceInitialized | 0x18267c | 1 | bytes | tu_14f730.c |
| deviceKeyA | 0x188a24 | 8 | bytes | tu_14f730.c |
| deviceTx | 0x188a2c | 16 | bytes | tu_14f730.c |
| deviceKeyB | 0x188a3c | 8 | bytes | tu_14f730.c |
| deviceCounter | 0x188a44 | 2 | bytes | tu_14f730.c |
| deviceRx | 0x188a48 | 16 | bytes | tu_14f730.c |
