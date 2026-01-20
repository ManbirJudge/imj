#include "imj.h"

static bool imj_extCmp__(const char *path, const char *ext)
{
    const char *dot = strrchr(path, '.');

    if (!dot || dot == path)
        return false;
    return strcasecmp(dot + 1, ext) == 0;
}

bool imj_img_from_file(char *path, ImjImg *img, char *err) {
    bool (*read)(FILE *f, ImjImg *img, char *err);

    if (imj_extCmp__(path, "jpg") || imj_extCmp__(path, "jpeg"))
        read = &imj_jpeg_read;
    else if (imj_extCmp__(path, "png"))
        read = &imj_png_read;
    else if (imj_extCmp__(path, "qoi"))
        read = &imj_qoi_read;
    else if (imj_extCmp__(path, "pnm") || imj_extCmp__(path, "pbm") || imj_extCmp__(path, "pgm") || imj_extCmp__(path, "ppm") || imj_extCmp__(path, "pam"))
        read = &imj_pnm_read;
    else if (imj_extCmp__(path, "bmp"))
        read = &imj_bmp_read;
    else if (imj_extCmp__(path, "gif"))
        read = &imj_gif_read;
    else {
        snprintf(err, 100, "Unsupported image format in '%s'.", path);
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(err, 100, "Can't open file.");
        return false;
    }

    const bool res = read(f, img, err);
    fclose(f);

    return res;
}

void imj_img_free(ImjImg *img) {
    if (img->data) {
        free(img->data);
        img->data = NULL;
    }
}
