#ifndef CORE_H
#define CORE_H

#ifdef IMJ_EXPORTS
    #define IMJ __declspec(dllexport)
#else
    #define IMJ __declspec(dllimport)
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "logging.h"

typedef uint8_t byte;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ImjPix;

typedef ImjPix Clr;

typedef struct
{
    uint64_t width;
    uint64_t height;
    ImjPix *data;
} ImjImg;

void imj_swap_uint16(uint16_t *val);
void imj_swap_uint32(uint32_t *val);

#endif