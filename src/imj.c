#include "imj.h"

static bool imj_extCmp__(const char *path, const char *ext) {
    const char *dot = strrchr(path, '.');

    if (!dot || dot == path)
        return false;
    return strcasecmp(dot + 1, ext) == 0;
}

bool imj_img_from_stream(FILE *stream, ImjImg *img, char *err, const ImjImgFormat fmt) {
    if (!stream) {
        if (err) snprintf(err, 100, "Stream is null");
        return false;
    }

    bool (*read)(FILE *f, ImjImg *img, char *err);

    switch (fmt) {
        case IMJ_IMG_FMT_PNG:
            read = &imj_png_read;
            break;
        case IMJ_IMG_FMT_JPEG:
            read = &imj_jpeg_read;
            break;
        case IMJ_IMG_FMT_GIF:
            read = &imj_gif_read;
            break;
        case IMJ_IMG_FMT_BMP:
            read = &imj_bmp_read;
            break;
        case IMJ_IMG_FMT_QOIF:
            read = &imj_qoi_read;
            break;
        case IMJ_IMG_FMT_PNM:
            read = &imj_pnm_read;
            break;
        case IMJ_IMG_FMT_UNKNOWN: {
            snprintf(err, 100, "Unknown image format.");
            return false;
        }
        default: {
            snprintf(err, 100, "Unsupported image format.");
            return false;
        }
    }

    return read(stream, img, err);
}

bool imj_img_from_file(char *path, ImjImg *img, char *err) {
    ImjImgFormat fmt;

    if (imj_extCmp__(path, "jpg") || imj_extCmp__(path, "jpeg"))
        fmt = IMJ_IMG_FMT_JPEG;
    else if (imj_extCmp__(path, "png"))
        fmt = IMJ_IMG_FMT_PNG;
    else if (imj_extCmp__(path, "qoi"))
        fmt = IMJ_IMG_FMT_QOIF;
    else if (imj_extCmp__(path, "pnm") || imj_extCmp__(path, "pbm") || imj_extCmp__(path, "pgm") ||
             imj_extCmp__(path, "ppm") || imj_extCmp__(path, "pam"))
        fmt = IMJ_IMG_FMT_PNM;
    else if (imj_extCmp__(path, "bmp"))
        fmt = IMJ_IMG_FMT_BMP;
    else if (imj_extCmp__(path, "gif"))
        fmt = IMJ_IMG_FMT_GIF;
    else {
        snprintf(err, 100, "Unsupported image format in '%s'.", path);
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(err, 100, "Can't open file.");
        return false;
    }
    const bool res = imj_img_from_stream(f, img, err, fmt);
    fclose(f);

    return res;
}

void imj_img_free(ImjImg *img) {
    if (img->data) {
        free(img->data);
        img->data = NULL;
    }
}
