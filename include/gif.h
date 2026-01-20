#ifndef IMJ_GIF_H
#define IMJ_GIF_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "core.h"
#include "logging.h"

// lzw
#define INVALID_CODE__ 0xFFFF

typedef uint16_t ImjLzwCode;

typedef struct {
    ImjLzwCode prefix;
    uint16_t suffix;
} ImjGifLzwDictEntry;

// bit reader
typedef struct {
    byte *data;
    size_t curIdx;
    size_t dataSize;
    size_t dataCap;

    uint64_t buff;
    uint8_t buffSize;
} ImjLzwBitReader;

void imj_lzwBitReader_init(ImjLzwBitReader *br, FILE *f);
ImjLzwCode imj_lzwBitReader_getCode(ImjLzwBitReader *br, uint8_t codeSize);
void imj_lzwBitReader_reset(ImjLzwBitReader *br);

// gif
#define IMJ_GIF_BLOCK_IMGDESC 0x2C
#define IMJ_GIF_BLOCK_EXT     0x21
#define IMJ_GIF_BLOCK_TRAILER 0x3B

#define IMJ_GIF_SUBBLOCK_GCE  0xF9

bool imj_gif_read(FILE *f, ImjImg *img, char err[100]);

#endif
