#ifndef JPEG_H
#define JPEG_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "logging.h"

#define IMJ_JPEG_MARKER_SOF0  0xffc0  // baseline dct
#define IMJ_JPEG_MARKER_SOF1  0xffc1  // extended sequential dct
#define IMJ_JPEG_MARKER_SOF2  0xffc2  // progressive dct
#define IMJ_JPEG_MARKER_SOF3  0xffc3  // lossless
#define IMJ_JPEG_MARKER_SOF5  0xffc5  // differential sequential dct
#define IMJ_JPEG_MARKER_SOF6  0xffc6  // differential progressive dct
#define IMJ_JPEG_MARKER_SOF7  0xffc7  // differential lossless dct
#define IMJ_JPEG_MARKER_SOF9  0xffc9  // extended sequential dct - with arithmetic coding
#define IMJ_JPEG_MARKER_SOF10 0xffca  // progressive dct - with arithmetic coding
#define IMJ_JPEG_MARKER_SOF11 0xffcb  // lossless - with arithmetic coding
#define IMJ_JPEG_MARKER_SOF13 0xffcd  // differential sequential dct - with arithmetic coding
#define IMJ_JPEG_MARKER_SOF14 0xffce  // differential progressive dct - with arithmetic coding
#define IMJ_JPEG_MARKER_SOF15 0xffcf  // differential lossless dct - with arithmetic coding
#define IMJ_JPEG_MARKER_SOI   0xffd8  // start of image
#define IMJ_JPEG_MARKER_DQT   0xffdb  // quantization table
#define IMJ_JPEG_MARKER_DRI   0xffdd  // reset interval
#define IMJ_JPEG_MARKER_DHT   0xffc4  // huffman table
#define IMJ_JPEG_MARKER_SOS   0xffda  // start of scan
#define IMJ_JPEG_MARKER_EOI   0xffd9  // end of image

typedef struct
{
    byte *data;
    size_t pos;
    size_t len;
} ImjJpegStream;

typedef struct
{
    uint16_t code;
    uint8_t codeLen;
    byte symbol;
} ImjJpegHtEntry;

typedef struct
{
    uint8_t lengths[16];
    uint8_t size;
    ImjJpegHtEntry *table;
} ImjJpegHt; // TODO: array of structs -> struct of arrays

typedef struct
{
    uint8_t id;
    uint8_t samplingFactorH;
    uint8_t samplingFactorV;
    uint8_t qtId;
} ImjJpegFrameComponent;

typedef struct
{
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t nComp;
    ImjJpegFrameComponent components[3];
} ImjJpegFrameData;

typedef struct
{
    uint8_t id;
    uint8_t dcHtId;
    uint8_t acHtId;
} ImjJpegSosComponent;

uint16_t imj_jpeg_stream_read(ImjJpegStream *s, size_t n);
void imj_jpeg_stream_align(ImjJpegStream *s);

uint16_t imj_jpeg_HT_getCode(ImjJpegHt *ht, ImjJpegStream *s);
int imj_jpeg_decodeNum(uint16_t bits, uint8_t len);
int imj_jpeg_buildMat(ImjJpegStream *s, ImjJpegHt *dcHt, ImjJpegHt *acHt, uint8_t qt[64], int oldDcCoeff, int mat[8][8]);

IMJ bool imj_jpeg_read(FILE *f, ImjImg *img, char err[100]);

bool imj_jpeg_read_DQT(FILE *f, uint8_t quantTables[4][64], char err[100]);
bool imj_jpeg_read_SOF0(FILE *f, ImjJpegFrameData *fd, char err[100]);
bool imj_jpeg_read_SOF2(FILE *f, ImjJpegFrameData *fd, char err[100]);
bool imj_jpeg_read_DHT(FILE *f, ImjJpegHt dcTables[4], ImjJpegHt acTables[4], char err[100]);

#endif