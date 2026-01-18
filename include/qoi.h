#ifndef IMJ_QOI_H
#define IMJ_QOI_H

#include <stdbool.h>
#include <string.h>

#include "core.h"

#define IMJ_QOI_OP_RGB   0b11111110
#define IMJ_QOI_OP_RGBA  0b11111111
#define IMJ_QOI_OP_INDEX 0b00000000
#define IMJ_QOI_OP_DIFF  0b01000000
#define IMJ_QOI_OP_LUMA  0b10000000
#define IMJ_QOI_OP_RUN   0b11000000

#define IMJ_QOI_PIX_HASH(p) (((p).r * 3u + (p).g * 5u + (p).b * 7u + (p).a * 11u) & 63u)

typedef struct {
    char     magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t  channels;
    uint8_t  colorSpace;
} ImjQoiHeader;

bool imj_qoi_read(FILE *f, ImjImg *img, char err[100]);

#endif