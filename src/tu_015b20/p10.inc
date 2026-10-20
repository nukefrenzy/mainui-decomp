// p10 0x3225c-0x36838: PopUpWindow, NotifyWindow, CountDownWindow, DialogWindow, ProgressDialog, GameDetailWindow
// Functions in address order; see docs/plan-tu_015b20.md.

// 0x3225c-0x34a14: not written yet.

// Written early by p03: restoreWindowState (0x1c1fc) needs this definition in the unit, because GCC
// marks a constructor without throwing calls nothrow and then omits the caller's cleanup pad.
GameDetailWindow::GameDetailWindow(int title, int id) : TrimusWindow(title, id)
{
    type = 0;
    param = 0;
    favorite = false;
    field84 = 0;
    field88 = 0;
    flag94 = true;
    field8c = 0;
    field98 = 0;
    param90 = 0;
}

// 0x34b1c-0x36838: not written yet.
