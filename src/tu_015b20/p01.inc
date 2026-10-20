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

// 0x15d00-0x17fc0: not written yet.

void writeCmdToRun(const char *cmd)
{
    FILE *f = fopen("/tmp/cmd_to_run.sh", "w+");
    if (f != NULL) {
        fputs(cmd, f);
        fclose(f);
    }
}
