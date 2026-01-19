#ifndef PNG_H
#define PNG_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "miniz.h"

#include "logging.h"
#include "core.h"

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interlaceMethod;
} imj_png_IHDR;

typedef struct 
{
    byte* compressedData;
    byte* decompressedData;
    byte* filteredData;
    size_t compressedSize;    
    size_t decompressedSize;    
} imj_png_IDAT;

IMJ bool imj_png_read(FILE *f, ImjImg *img, char err[100]);

bool imj_png_read_IHDR(FILE *f, imj_png_IHDR *ihdrData, char err[100]);
bool imj_png_read_PLTE(FILE *f, ImjClr **pallete, uint32_t len, char err[100]);

#endif