#ifndef IMJ_H
#define IMJ_H

#include <stdio.h>

#include "core.h"
#include "png.h"
#include "jpeg.h"
#include "webp.h"

IMJ bool imj_img_from_file(char* path, Img* img, char* err);
// IMJ Img imj_img_from_mem();
// IMJ Img imj_img_from_stdin();
// IMJ Img imj_img_from_fd();

#endif