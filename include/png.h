#ifndef PNG_H
#define PNG_H

#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "miniz.h"

#include "logging.h"
#include "core.h"

//                                0b0ACP
#define IMJ_PNG_CLRTYPE_GRAY      0b0000
#define IMJ_PNG_CLRTYPE_RGB       0b0010
#define IMJ_PNG_CLRTYPE_IDX       0b0011
#define IMJ_PNG_CLRTYPE_GRAYALPHA 0b0100
#define IMJ_PNG_CLRTYPE_RGBA      0b0110

#define IMJ_PNG_COMPRESSION_METHOD 0

#define IMJ_PNG_FILTER_METHOD 0

#define IMJ_PNG_FILTER_NONE  0
#define IMJ_PNG_FILTER_SUB   1
#define IMJ_PNG_FILTER_UP    2
#define IMJ_PNG_FILTER_AVG   3
#define IMJ_PNG_FILTER_PAETH 4

#define IMJ_PNG_INTERLACE_NONE  0
#define IMJ_PNG_INTERLACE_ADAM7 1

#define IMJ_PNG_BYTES_PER_SAMPLE 1

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interlaceMethod;
} ImjPngIhdr;

typedef struct 
{
    size_t compressedDataSize;
    size_t compressedDataCap;

    byte* compressedData;
    byte* decompressedData;
    byte* filteredData;
} ImjPngDataInfo;

typedef struct {
    uint8_t bitDepth;
    uint16_t mask;
    uint16_t maxVal;

    byte* filteredData;
    size_t curPos;

    uint64_t buff;
    uint8_t buffSize;
} ImjPngSamplerInfo;

IMJ bool imj_png_read(FILE *f, ImjImg *img, char err[100]);

bool imj_png_read_IHDR(FILE *f, ImjPngIhdr *ihdr, char err[100]);
bool imj_png_read_PLTE(FILE *f, ImjClr **palette, uint32_t len, char err[100]);

void imj_png_sampler_init(ImjPngSamplerInfo *si, ImjPngIhdr *ihdr, ImjPngDataInfo *di);
uint16_t imj_png_sampler_nextSample(ImjPngSamplerInfo* si);

void imj_png_dbg_IHDR(ImjPngIhdr *ihdr);

#endif