// Nearest-neighbor scaling of 32-bit pixels (tu_11cdc8.cpp). Callers pass sizes and stride as
// unsigned; the function converts them to signed internally.
#pragma once
void scalePixels(int sx, int sy, unsigned sw, unsigned sh, unsigned dw, unsigned dh,
                 unsigned stride, int padding, int *src, int *dst);
