# Read-only-data recovery

2026-10-22, following the linked-text layout and ARM call/patch corrections.
The complete linked `.rodata` is byte-identical over **0x1518e8–0x164a50**
(78,184 bytes), including relocated vtable/RTTI pointers and padding. Both sections
have SHA-256 `21ccb7647a7a9b28a7b2f74a29a236696020b25471536f23d28aee123638115d`.
`tools/linkaudit.py` enforces address, size and every byte without masks.

## Source and literal identity

- The main UI unit retains one `./` identity, the original 32-byte patched shutdown
  command slot, and deferred `__func__` labels for `donoting_action` and `~IconMenu`.
  The 29-pointer legacy extension table restores its suffix strings, including `.unif`.
- Theme colors belong in the theme unit. Defining them as `SDL_Color` structs and
  assigning their bytes preserves the original loads/stores; union definitions in
  the consuming unit had folded constants even at `-O0`. The unused nine-color
  palette and the original ordered theme literal pool are now explicit source data.
  Five battery paths patched to the same text retain separate identities and their
  original 28/24-byte extents instead of collapsing into one pooled string.
- Decoder assertion signatures and six Wi-Fi function labels retain deferred
  function-name placement. Duplicate `dowork` labels remain distinct objects.
  The key-input diagnostic retains its original 44-byte allocation.
- Wi-Fi inline code refers to named literals whose definitions occur at the original
  first-use positions. IME switch cases follow the original character-literal order;
  numeric case order had matched instructions while arranging constants differently.
- PWM literals form a readable ordered pool. The patched `/dev/null` duty path
  occupies the original 40-byte slot; ordinary pooling had removed 28 bytes.

These are source constants and recovered allocation extents, not section bytes
extracted during the build. No arbitrary linker padding, aliases, extra instruction
masks or relaxed unit ends are used. `src/data.tsv` now checks 58 extents / 8,689 bytes /
185 strings independently, including the writable legacy suffix table.

## Shared-header changes

| Header | Change and evidence |
| --- | --- |
| `src/tu_015b20.h` | Scope RapidXml's assertion diagnostic filename to its original SDK path. Standard declarations are included first and `RAPIDXML_NO_STDLIB` prevents their reinclusion within the scoped macro. The fetched vendor header remains unchanged. |
| `src/HardwareTestWindows.h` | Include `KeyTestWindow2.h` before declaring `SDLKeyTestWindow`, restoring reverse class emission order for vtables and RTTI. |
| `src/WifiMenus.h` | Declare shared literal identities and place WifiMenu/HotspotMenu before WifiConnectAction to restore data emission order. Bind WifiMenuDrawer's renderer to existing slot 7 (`vfunc7`) and declare existing event slot 15 pure virtual. Derived renderers call the base renderer explicitly. |

No object fields or virtual slots were added. Raw pointer comparison exposed the two
incorrect base vtable entries that masked text comparisons could not establish.
The registered symbols for `donoting_action` and `WifiMenuDrawer::vfunc7` were updated
and `src/MATCHES.md` regenerated.

## Validation and limits

- All **689/689 registered matches** and **22/22 whole units** pass.
- All **6/6 ARM audit tests** pass. The new real-ELF fixture rejects changed strings,
  relocated pointers, section addresses and sizes.
- `linkaudit.py --require-link` passes: raw `.rodata` equality, masked linked `.text`
  equality over 1,294,000 bytes, zero raw ARM call/patch differences, all body anchors,
  3,095 resolved body calls, storage and final import/version/library-order checks.

Writable data remains 488 bytes short and BSS 160 bytes short. Exception-table
addresses and sizes agree, but contents differ; some GOT and other relocation values
also remain unproven. See [link-audit.md](link-audit.md). This is not a byte-identical
whole executable or a device-tested replacement.
