#include "imj.h"

bool ext_cmp(const char *path, const char *ext)
{
    const char *dot = strrchr(path, '.');

    if (!dot || dot == path)
        return false;
    return strcasecmp(dot + 1, ext) == 0;
}

IMJ bool imj_img_from_file(char *path, Img *img, char *err)
{
    bool (*read)(FILE* f, Img* img, char* err);

    if (ext_cmp(path, "jpg") || ext_cmp(path, "jpeg"))
        read = &jpeg_read;
    else if (ext_cmp(path, "png"))
        read = &png_read;
    else
    {
        snprintf(err, 100, "Unsupported image format in '%s'.", path);
        return false;
    }

    FILE* f = fopen(path, "rb");
    if (!f)
    {
        snprintf(err, 100, "Can't open file.");
        return false;
    }

    bool res = read(f, img, err);
    fclose(f);

    return res;
}