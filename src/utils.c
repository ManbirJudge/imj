#include "utils.h"

bool writeb_path(char* path, byte* data, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;

    if (!fwrite(data, 1, n, f))
        return false;

    fclose(f);
    return true;
}