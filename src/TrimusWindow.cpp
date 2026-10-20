// TrimusWindow::getId sits in tu_015b20's inline tail (0x394c8), so it belongs in TrimusWindow.h as
// an inline method. An inline function is emitted only when used, and its callers (0x35054 in p10,
// 0x37004 and 0x372b8 in p11) are not written yet; until then it is matched here on its own.
// The rest of TrimusWindow lives in src/tu_015b20/p03.inc and the header.
#include "TrimusWindow.h"

int TrimusWindow::getId()
{
    return id;
}
