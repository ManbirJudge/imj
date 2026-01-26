#ifndef IMJ_H
#define IMJ_H

#include <stdio.h>

#include "core.h"
#include "png.h"
#include "jpeg.h"
#include "qoi.h"
#include "pnm.h"
#include "bmp.h"
#include "gif.h"

typedef enum {
    IMJ_IMG_FMT_PNG,
    IMJ_IMG_FMT_JPEG,
    IMJ_IMG_FMT_GIF,
    IMJ_IMG_FMT_BMP,
    IMJ_IMG_FMT_QOIF,
    IMJ_IMG_FMT_PNM,
    IMJ_IMG_FMT_UNKNOWN
} ImjImgFormat;

IMJ bool imj_img_from_stream(FILE *stream, ImjImg *img, char *err, ImjImgFormat fmt);
IMJ bool imj_img_from_file(char *path, ImjImg *img, char *err);

// IMJ Img imj_img_from_mem();
// IMJ Img imj_img_from_stdin();
// IMJ Img imj_img_from_fd();

IMJ void imj_img_free(ImjImg *img);

#endif
