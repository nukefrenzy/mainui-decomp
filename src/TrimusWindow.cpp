#include "TrimusWindow.h"

TrimusWindow::TrimusWindow(int titleLabelId, int id)
{
    this->titleLabelId = titleLabelId;
    this->id = id;
}

TrimusWindow::~TrimusWindow()
{
}

void TrimusWindow::animateIn(SDL_Surface *dst, FlipFn flip)
{
}

void TrimusWindow::animateOut(SDL_Surface *dst, FlipFn flip)
{
}

int TrimusWindow::getId()
{
    return id;
}
