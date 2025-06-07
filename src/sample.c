#include "sample.h"

void upsample_scale_int(size_t srcW, size_t srcH, size_t scaleX, size_t scaleY, int src[srcH][srcW], int dst[srcH * scaleX][srcW * scaleY])
{
    size_t a = srcW * scaleX;
    size_t b = srcH * scaleY;

    for (size_t y = 0; y < b; y++)
    {
        for (size_t x = 0; x < a; x++)
        {
            dst[y][x] = src[y / scaleY][x / scaleX];
        }
    }
}