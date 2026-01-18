#ifndef IMJ_PNM_H
#define IMJ_PNM_H

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "core.h"

bool imj_pnm_read(FILE *f, ImjImg *img, char err[100]);

#endif