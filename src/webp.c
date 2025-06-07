#include "webp.h"

const byte RIFF_SIGN[] = {0x52, 0x49, 0x46, 0x46};
const byte WEBP_SIGN[] = {0x57, 0x45, 0x42, 0x50};

IMJ bool webp_read(FILE *f, Img *img, char err[100])
{
    if (!f || !img)
    {
        if (err)
            snprintf(err, 100, "One of the arguments is NULL.");
        return false;
    }

    byte buff1[4];
    uint32_t size;

    // --- WebP file header
    fread(buff1, 1, 4, f);
    if (memcmp(buff1, RIFF_SIGN, 4) != 0)
    {
        if (err)
            snprintf(err, 100, "Invalid RIFF file signature: \'%.*s\'", 4, buff1);
        return false;
    }

    fread(&size, 4, 1, f);
    if (size % 2 != 0)
    {
        if (err)
            snprintf(err, 100, "File size must be even: \'%.*s\' bytes", 4, buff1);
        return false;
    }

    fread(buff1, 1, 4, f);
    if (memcmp(buff1, WEBP_SIGN, 4) != 0)
    {
        if (err)
            snprintf(err, 100, "Invalid WEBP file signature: \'%.*s\'", 4, buff1);
        return false;
    }

    // --- chunks
    char chunk4CC[4];
    uint32_t chunkSize;

    while(fread(&chunk4CC, 1, 4, f))
    {
        fread(&chunkSize, 4, 1, f);

        DBG("Reading chunk: \'%.*s\'", 4, chunk4CC);

        fseek(f, chunkSize, SEEK_CUR);
    }

    // ---
    return true;
}