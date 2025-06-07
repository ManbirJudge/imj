#ifndef JPEG_H
#define JPEG_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "logging.h"
#include "sample.h"

#define SOF0 (uint16_t)0xffc0
#define SOF1 (uint16_t)0xffc1
#define SOF2 (uint16_t)0xffc2
#define SOF3 (uint16_t)0xffc3
#define SOF5 (uint16_t)0xffc5
#define SOF6 (uint16_t)0xffc6
#define SOF7 (uint16_t)0xffc7
#define SOF9 (uint16_t)0xffc9
#define SOF10 (uint16_t)0xffca
#define SOF11 (uint16_t)0xffcb
#define SOF13 (uint16_t)0xffcd
#define SOF14 (uint16_t)0xffce
#define SOF15 (uint16_t)0xffcf

#define SOF0 (uint16_t)0xffc0
#define SOI (uint16_t)0xffd8
#define APP0 (uint16_t)0xffe0
#define APP1 (uint16_t)0xffe1
#define APP2 (uint16_t)0xffe2
#define APP13 (uint16_t)0xffed
#define DQT (uint16_t)0xffdb
#define DRI (uint16_t)0xffdd
#define DHT (uint16_t)0xffc4
#define SOS (uint16_t)0xffda
#define EOI (uint16_t)0xffd9

typedef struct
{
    byte *data;
    size_t pos;
    size_t len;
} jpeg_Stream;

typedef struct
{
    uint16_t code;
    uint8_t codeLen;
    byte symbol;
} jpeg_HT_Entry;

typedef struct
{
    uint8_t lengths[16];
    uint8_t size;
    jpeg_HT_Entry *table;
} jpeg_HT; // TODO: array of structs -> struct of arrays

typedef struct
{
    uint8_t id;
    uint8_t samplingFactorH;
    uint8_t samplingFactorV;
    uint8_t qtId;
} jpeg_frame_comp;

typedef struct
{
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t nComp;
    jpeg_frame_comp components[3];
} jpeg_frame_data;

typedef struct
{
    uint8_t id;
    uint8_t dcHtId;
    uint8_t acHtId;
} jpeg_SOS_comp;

uint16_t jpeg_stream_read(jpeg_Stream *s, size_t n);
void jpeg_align(jpeg_Stream *s);

uint16_t jpeg_HT_get_code(jpeg_HT *ht, jpeg_Stream *s);
int jpeg_decode_num(uint16_t bits, uint8_t len);
int jpeg_build_mat(jpeg_Stream *s, jpeg_HT *dcHt, jpeg_HT *acHt, uint8_t qt[64], int oldDcCoeff, int mat[8][8]);

IMJ bool jpeg_read(FILE *f, Img *img, char err[100]);

bool jpeg_read_DQT(FILE *f, uint8_t quantTables[4][64], char err[100]);
bool jpeg_read_SOF0(FILE *f, jpeg_frame_data *frameData, char err[100]);
bool jpeg_read_SOF2(FILE *f, jpeg_frame_data *frameData, char err[100]);
bool jpeg_read_DHT(FILE *f, jpeg_HT dcTables[4], jpeg_HT acTables[4], char err[100]);

IMJ char *jpeg_get_marker_name(uint16_t marker);

#endif