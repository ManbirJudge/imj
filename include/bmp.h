#ifndef IMJ_BMP_H
#define IMJ_BMP_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core.h"

#define IMJ_BMP_CORE_HEADER     12
#define IMJ_BMP_CORE_EXT_HEADER 16
#define IMJ_BMP_OS2_HEADER      64
#define IMJ_BMP_INFO_HEADER     40
#define IMJ_BMP_V2_INFO_HEADER  52
#define IMJ_BMP_V3_INFO_HEADER  56
#define IMJ_BMP_V4_HEADER       108
#define IMJ_BMP_V5_HEADER       124

#define IMJ_BMP_COMPRESSION_RGB             0
#define IMJ_BMP_COMPRESSION_RLE8            1
#define IMJ_BMP_COMPRESSION_RLE4            2
#define IMJ_BMP_COMPRESSION_BITFIELDS       3
#define IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS 4
#define IMJ_BMP_COMPRESSION_PNG             5
#define IMJ_BMP_COMPRESSION_JPEG            6

typedef enum {
    IMJ_BMP_LAYOUT_DEFAULT,
    IMJ_BMP_LAYOUT_MASKED,
    IMJ_BMP_LAYOUT_RLE,
    IMJ_BMP_LAYOUT_EMBEDDED
} ImjBmpLayout;

typedef enum {
    IMJ_BMP_PLT_RGB,
    IMJ_BMP_PLT_RGB0
} ImjBmpPltType;

typedef struct {
    int32_t x, y, z;
} ImjCieXYZ;

typedef struct {
    ImjCieXYZ r;
    ImjCieXYZ g;
    ImjCieXYZ b;
} ImjCieXYZTriple;

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} ImjBmpFileHeader;

typedef struct {
    uint32_t size;
    uint16_t width;
    uint16_t height;
    uint16_t nPlanes;
    uint16_t bpp;
} ImjBmpCoreHeader;

typedef struct {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
} ImjBmpCoreExtHeader; // TODO: check if this is the valid spec. it works so ... idc. BUT, i couldn't find anything about it on the internet

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imageSize;
    int32_t  xPpm;
    int32_t  yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
    uint16_t units;
    uint16_t reserved;
    uint16_t recording;
    uint16_t rendering;         // halftoning info
    uint32_t size1;             // usually 0
    uint32_t size2;             // usually 0
    uint32_t colorEncoding;     // 0 = RGB
    uint32_t identifier;        // usually 0
} ImjBmpOS2Header;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imgSize;
    int32_t xPpm;
    int32_t yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
} ImjBmpInfoHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imgSize;
    int32_t xPpm;
    int32_t yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
} ImjBmpV2InfoHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imgSize;
    int32_t xPpm;
    int32_t yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;
} ImjBmpV3InfoHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imgSize;
    int32_t xPpm;
    int32_t yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;
    uint32_t csType;
    ImjCieXYZTriple endpoints;
    uint32_t gammaR;
    uint32_t gammaG;
    uint32_t gammaB;
} ImjBmpV4Header;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t nPlanes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imgSize;
    int32_t xPpm;
    int32_t yPpm;
    uint32_t nClrUsed;
    uint32_t nClrImp;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;
    uint32_t csType;
    ImjCieXYZTriple endpoints;
    uint32_t gammaR;
    uint32_t gammaG;
    uint32_t gammaB;
    uint32_t renderingIntent;
    uint32_t iccProfileOffset;
    uint32_t iccProfileSize;
    uint32_t reserved;
} ImjBmpV5Header;

typedef struct {
    uint32_t width;
    uint32_t height;
    bool topDown;  // !!!NOTE!!!: logic is inverted. variable name should be 'bottomsUp'

    uint16_t bpp;
    ImjBmpLayout layout;

    bool isMasked;
    uint32_t rMask;
    uint32_t gMask;
    uint32_t bMask;
    uint32_t aMask;

    bool hasPalette;
    uint32_t paletteSize;
    ImjBmpPltType pltType;

    size_t pixDataOffset;
    size_t rowStride;
} ImjBmpDecodeInfo;

bool imj_bmp_read(FILE *f, ImjImg *img, char err[100]);
bool imj_bmp_get_decode_info(FILE *f, ImjBmpDecodeInfo *di, char err[100]);

#endif
