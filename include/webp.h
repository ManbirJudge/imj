#ifndef WEBP_H
#define WEBP_H

#include <string.h>

#include "logging.h"
#include "core.h"

extern const byte RIFF_SIGN[4];
extern const byte WEBP_SIGN[4];

IMJ bool webp_read(FILE *f, Img *img, char err[100]);

#endif