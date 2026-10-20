// p03 0x1a44c-0x1d9d8: DeleteRomAction, helpers, TrimusWindow, ListMenu ctor
// Functions in address order; see docs/plan-tu_015b20.md.

// 0x1a44c-0x1d7dc: not written yet.

TrimusWindow::TrimusWindow(int titleLabelId, int id)
{
    this->titleLabelId = titleLabelId;
    this->id = id;
}

TrimusWindow::~TrimusWindow()
{
}
