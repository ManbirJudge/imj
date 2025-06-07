#ifndef JPEG_H
#define JPEG_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "logging.h"

#define SOI (uint16_t)0xffd8
#define APP0 (uint16_t)0xffe0
#define APP1 (uint16_t)0xffe1
#define APP2 (uint16_t)0xffe2
#define APP13 (uint16_t)0xffed
#define DQT (uint16_t)0xffdb
#define SOF (uint16_t)0xffc0
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
    // byte info;
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
} jpeg_SOF0_comp;

typedef struct
{
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t nComp;
    jpeg_SOF0_comp components[3];
} jpeg_SOF0_data;

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
bool jpeg_read_SOF(FILE *f, jpeg_SOF0_data *sof0Data, char err[100]);
bool jpeg_read_DHT(FILE *f, jpeg_HT dcTables[4], jpeg_HT acTables[4], char err[100]);

IMJ char *jpeg_get_marker_name(uint16_t marker);

#endif