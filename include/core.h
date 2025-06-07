#ifndef CORE_H
#define CORE_H

#ifdef _WIN32
    #ifdef IMJ_EXPORTS
        #define IMJ __declspec(dllexport)
    #else
        #define IMJ __declspec(dllimport)
    #endif
#else
    #define IMJ
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// basic types
typedef uint8_t byte;

// complex types
typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Pix;

typedef Pix Clr;

typedef struct
{
    unsigned int width;
    unsigned int height;
    Pix *data;
} Img;

// basic functions
void swap_uint16(uint16_t *val);
void swap_uint32(uint32_t *val);

IMJ void print_bytes(byte bytes[], size_t n);
IMJ void print_chars(byte chars[], size_t n);
void printb_uint16(uint16_t val);

// ---
Pix YCbCr_to_pix(int Y, int Cb, int Cr);

// image functions
IMJ void imj_img_free(Img *img);

#endif