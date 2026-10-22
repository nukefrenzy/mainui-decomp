// Shared scaler API. Sizes/stride are unsigned at callers, with signed size conversions inside.
#pragma once
void scalePixels(int sx, int sy, unsigned sw, unsigned sh, unsigned dw, unsigned dh,
                 unsigned stride, int padding, int *src, int *dst);
