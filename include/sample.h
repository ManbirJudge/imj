#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>

void upsample_scale_int(size_t srcW, size_t srcH, size_t scaleX, size_t scaleY, int src[srcH][srcW], int dst[srcH * scaleX][srcW * scaleY]);

#endif