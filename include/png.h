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

extern const byte PNG_SIGN[8];

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interlaceMethod;
} png_IHDR;

typedef struct 
{
    byte* compressedData;
    byte* decompressedData;
    byte* filteredData;
    size_t compressedSize;    
    size_t decompressedSize;    
} png_IDAT;

IMJ bool png_read(FILE *f, Img *img, char err[100]);

IMJ bool png_read_IHDR(FILE *f, png_IHDR *ihdrData, char err[100]);
IMJ bool png_read_PLTE(FILE *f, Clr **pallete, uint32_t len, char err[100]);

IMJ void png_print_IHDR(png_IHDR ihdrData);

#endif