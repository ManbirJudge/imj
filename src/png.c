#include "png.h"

static uint8_t imj_PaethPredictor__(uint8_t a, uint8_t b, uint8_t c)
{
    int p = (int)a + (int)b - (int)c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

bool imj_png_read(FILE *f, ImjImg *img, char err[100])
{
    if (!f || !img)
    {
        if (err)
            snprintf(err, 100, "One of the arguments is NULL.");
        return false;
    }

    byte buff1[8];

    // --- signature
    const byte PNG_SIGN[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

    fread(buff1, 1, 8, f);
    if (memcmp(buff1, PNG_SIGN, 8) != 0)
    {
        if (err)
            snprintf(err, 100, "Invalid PNG file signature: \'%.*s\'.", 4, buff1);
        return false;
    }

    // --- chunks
    uint32_t chunkLen;
    unsigned char chunkType[4];

    imj_png_IHDR ihdrData = {};
    ImjClr *pallete = NULL;
    imj_png_IDAT idatData = {
        .compressedData = NULL,
        .decompressedData = NULL,
        .filteredData = NULL,
        .compressedSize = 0,
        .decompressedSize = 0};

    ImjPix *data = NULL;

    // TODO: checks: IHDR 1st, IEND last, must contain least 1 IDAT (if multiple, they must be adjacent), only 1 PLTE before IDAT

    while (fread(&chunkLen, 4, 1, f))
    {
        imj_swap_uint32(&chunkLen);
        fread(chunkType, 1, 4, f);

        // if (!(('A' <= chunkType[3]) && (chunkType[3] <= 'Z')))
        //     continue;

        if (memcmp(chunkType, "IHDR", 4) == 0)
        {
            if (!imj_png_read_IHDR(f, &ihdrData, err))
                return false;

            idatData.compressedData = (byte *)malloc(ihdrData.height * (1 + ihdrData.width * 4));
            idatData.decompressedData = (byte *)malloc(ihdrData.height * (1 + ihdrData.width * 4));
            idatData.filteredData = (byte *)malloc(ihdrData.height * (1 + ihdrData.width * 4));
            idatData.decompressedSize = (size_t)ihdrData.height * (1 + ihdrData.width * 4);

            fseek(f, 4, SEEK_CUR);
        }
        else if (memcmp(chunkType, "PLTE", 4) == 0)
        {
            if (!imj_png_read_PLTE(f, &pallete, chunkLen, err))
                return false;
            fseek(f, 4, SEEK_CUR);
        }
        else if (memcmp(chunkType, "IDAT", 4) == 0)
        {
            fread(idatData.compressedData + idatData.compressedSize, 1, chunkLen, f);
            idatData.compressedSize += chunkLen;
            fseek(f, 4, SEEK_CUR);
        }
        else if (memcmp(chunkType, "IEND", 4) == 0)
        {
            break;
        }
        else
        {
            // if (('A' <= chunkType[0]) && (chunkType[0] <= 'Z'))
            // {
            //     char _[28] = "Unkown critical chunk: ";
            //     return false;
            // }
            // else
            // {
            WRN("Ignoring unknown chunk of length: %i", chunkLen);
            fseek(f, chunkLen + 4, SEEK_CUR);
            // }
        }
    }

    // --- image data
    if (ihdrData.bitDepth != 8)
    {
        snprintf(err, 100, "Only bit-depth of 8 bits/sample is supported.");
        return false;
    }

    // decompression
    mz_ulong actualDecompressedSize = idatData.decompressedSize;

    int status = mz_uncompress(
        idatData.decompressedData,
        &actualDecompressedSize,
        idatData.compressedData,
        idatData.compressedSize
    );

    if (status != MZ_OK)
    {
        snprintf(err, 100, "IDAT decompression failed. (code: %i)", status);
        return false;
    }

    // common parameters
    uint8_t samplesPerPixel = 0;
    switch (ihdrData.colorType)
    {
    case 0:
        samplesPerPixel = 1;
        break;
    case 2:
        samplesPerPixel = 3;
        break;
    case 3:
        samplesPerPixel = 1;
        break;
    case 4:
        samplesPerPixel = 2;
        break;
    case 6:
        samplesPerPixel = 4;
        break;
    }

    uint8_t bytesPerPixel = samplesPerPixel * 1;
    size_t bytesPerScanline = 1 + ihdrData.width * bytesPerPixel;
    uint32_t nScanlines = ihdrData.height;

    // un-filtering
    byte prevUnfilteredScanline[bytesPerScanline - 1];
    memset(prevUnfilteredScanline, 0, bytesPerScanline - 1);
    byte unfilteredScaline[bytesPerScanline - 1];

    byte scaline[bytesPerScanline];

    for (uint32_t i = 0; i < nScanlines; i++)
    {
        memcpy(scaline, idatData.decompressedData + i * bytesPerScanline, bytesPerScanline);

        uint8_t filterType = scaline[0];
        switch (filterType)
        {
        case 0:
            for (size_t j = 0; j < bytesPerScanline - 1; j++)
            {
                unfilteredScaline[j] = scaline[j + 1];
            }
            break;
        case 1:
            for (size_t j = 0; j < bytesPerScanline - 1; j++)
            {
                if (j < bytesPerPixel)
                    unfilteredScaline[j] = scaline[j + 1] + 0;
                else
                    unfilteredScaline[j] = (uint8_t)(scaline[j + 1] + unfilteredScaline[j - bytesPerPixel]);
            }
            break;
        case 2:
            for (size_t j = 0; j < bytesPerScanline - 1; j++)
            {
                unfilteredScaline[j] = (uint8_t)(scaline[j + 1] + prevUnfilteredScanline[j]);
            }
            break;
        case 3:
            for (size_t j = 0; j < bytesPerScanline - 1; j++)
            {
                uint8_t x = 0;
                uint8_t y = prevUnfilteredScanline[j];

                if (j >= bytesPerPixel)
                    x = unfilteredScaline[j - bytesPerPixel];

                unfilteredScaline[j] = (uint8_t)(scaline[j + 1] + (uint8_t)((x + y) / 2));
            }
            break;
        case 4:
            for (size_t j = 0; j < bytesPerScanline - 1; j++)
            {
                uint8_t a = 0;
                uint8_t b = prevUnfilteredScanline[j];
                uint8_t c = 0;

                if (j >= bytesPerPixel)
                {
                    a = unfilteredScaline[j - bytesPerPixel];
                    c = prevUnfilteredScanline[j - bytesPerPixel];
                }

                unfilteredScaline[j] = (uint8_t)(scaline[j + 1] + imj_PaethPredictor__(a, b, c));
            }
            break;
        default:
            snprintf(err, 100, "Unknown filter type.");
            return false;
        }

        memcpy(prevUnfilteredScanline, unfilteredScaline, bytesPerScanline - 1);
        memcpy(idatData.filteredData + i * (bytesPerScanline - 1), unfilteredScaline, bytesPerScanline - 1);
    }

    // creating final image data
    data = (ImjPix *)malloc(ihdrData.width * ihdrData.height * sizeof(ImjPix));

    switch (ihdrData.colorType)
    {
    case 0:
    {
        for (uint32_t i = 0; i < ihdrData.height; i++)
        {
            for (uint32_t j = 0; j < ihdrData.width; j++)
            {
                size_t k = (size_t)i * ihdrData.width + j;

                ImjPix pix;
                pix.r = idatData.filteredData[k];
                pix.g = pix.r;
                pix.b = pix.r;
                pix.a = 255;

                data[k] = pix;
            }
        }
        break;
    }
    case 2:
    {
        for (uint32_t i = 0; i < ihdrData.height; i++)
        {
            for (uint32_t j = 0; j < ihdrData.width; j++)
            {
                size_t k = (size_t)i * ihdrData.width + j;
                size_t off = k * 3;

                ImjPix pix;
                pix.r = idatData.filteredData[off];
                pix.g = idatData.filteredData[off + 1];
                pix.b = idatData.filteredData[off + 2];
                pix.a = 255;

                data[k] = pix;
            }
        }
        break;
    }
    case 3:
    {
        if (!pallete)
        {
            snprintf(err, 100, "Pallete chunk didn't appear for indexed PNG.");
            return false;
        }

        for (uint32_t i = 0; i < ihdrData.height; i++)
        {
            for (uint32_t j = 0; j < ihdrData.width; j++)
            {
                size_t k = (size_t)i * ihdrData.width + j;
                memcpy(data + k, pallete + idatData.filteredData[k], sizeof(ImjPix));
            }
        }
        break;
    }
    case 4:
    {
        for (uint32_t i = 0; i < ihdrData.height; i++)
        {
            for (uint32_t j = 0; j < ihdrData.width; j++)
            {
                size_t k = (size_t)i * ihdrData.width + j;
                size_t off = k * 2;

                ImjPix pix;
                pix.r = idatData.filteredData[off];
                pix.g = pix.r;
                pix.b = pix.r;
                pix.a = idatData.filteredData[off + 1];

                data[k] = pix;
            }
        }
        break;
    }
    case 6:
    {
        for (uint32_t i = 0; i < ihdrData.height; i++)
        {
            for (uint32_t j = 0; j < ihdrData.width; j++)
            {
                size_t k = (size_t)i * ihdrData.width + j;
                size_t off = k * 4;

                ImjPix pix;
                pix.r = idatData.filteredData[off];
                pix.g = idatData.filteredData[off + 1];
                pix.b = idatData.filteredData[off + 2];
                pix.a = idatData.filteredData[off + 3];

                data[k] = pix;
            }
        }
        break;
    }
    }

    // ---
    img->width = ihdrData.width;
    img->height = ihdrData.height;
    img->data = data;

    // --- cleanup
    if (pallete)
        free(pallete);
    if (idatData.compressedData)
        free(idatData.compressedData);
    if (idatData.decompressedData)
        free(idatData.decompressedData);
    if (idatData.filteredData)
        free(idatData.filteredData);

    // ---
    return true;
}

bool imj_png_read_IHDR(FILE *f, imj_png_IHDR *ihdrData, char err[100])
{
    // reading
    fread(&ihdrData->width, 4, 1, f);
    fread(&ihdrData->height, 4, 1, f);
    fread(&ihdrData->bitDepth, 1, 1, f);
    fread(&ihdrData->colorType, 1, 1, f);
    fread(&ihdrData->compressionMethod, 1, 1, f);
    fread(&ihdrData->filterMethod, 1, 1, f);
    fread(&ihdrData->interlaceMethod, 1, 1, f);

    imj_swap_uint32(&ihdrData->width);
    imj_swap_uint32(&ihdrData->height);

    // width, height check | TODO: check before swapping endian-ness
    if (ihdrData->width == 0 || (ihdrData->width & 0x80000000) != 0)
    {
        snprintf(err, 100, "Invalid width.");
        return false;
    }
    if (ihdrData->height == 0 || (ihdrData->height & 0x80000000) != 0)
    {
        snprintf(err, 100, "Invalid width.");
        return false;
    }

    // bit-depht check
    if (!(ihdrData->bitDepth == 1 ||
          ihdrData->bitDepth == 2 ||
          ihdrData->bitDepth == 4 ||
          ihdrData->bitDepth == 8 ||
          ihdrData->bitDepth == 16))
    {
        snprintf(err, 100, "Invalid bit-depth.");
        return false;
    }

    // color type check | TODO: use a 'switch'
    switch (ihdrData->colorType)
    {
    case 0:
        break;
    case 2:
        if (!(ihdrData->bitDepth == 8 || ihdrData->bitDepth == 16))
        {
            snprintf(err, 100, "Invalid bit-depth for RGB color type (2).");
            return false;
        }
        break;
    case 3:
        if (!(ihdrData->bitDepth == 1 || ihdrData->bitDepth == 2 || ihdrData->bitDepth == 4 || ihdrData->bitDepth == 8))
        {
            snprintf(err, 100, "Invalid bit-depth for pallete-index color type (3).");
            return false;
        }
        break;
    case 4:
        if (!(ihdrData->bitDepth == 8 || ihdrData->bitDepth == 16))
        {
            snprintf(err, 100, "Invalid bit-depth for grayscale-alpha color type (4).");
            return false;
        }
        break;
    case 6:
        if (!(ihdrData->bitDepth == 8 || ihdrData->bitDepth == 16))
        {
            snprintf(err, 100, "Invalid bit-depth for RGBA color type (6).");
            return false;
        }
        break;
    default:
        snprintf(err, 100, "Invalid color type.");
        return false;
    }

    // other checks
    if (ihdrData->compressionMethod != 0)
    {
        snprintf(err, 100, "Invalid compression method.");
        return false;
    }
    if (ihdrData->filterMethod != 0)
    {
        snprintf(err, 100, "Invalid filther method.");
        return false;
    }
    if (ihdrData->interlaceMethod != 0)
    {
        snprintf(err, 100, "Only non-interlaced PNGs are supported (yet).");
        return false;
    }

    // ---
    return true;
}

bool imj_png_read_PLTE(FILE *f, ImjClr **pallete, uint32_t len, char err[100])
{
    if (len % 3 != 0)
    {
        snprintf(err, 100, "PLTE chunk size must be a multiple of 3.");
        return false;
    }
    if (*pallete)
    {
        snprintf(err, 100, "There must only be 1 PLTE chunk.");
        return false;
    }

    // TODO: max entries < possible due to bit-depth
    *pallete = malloc(len / 3 * sizeof(ImjClr));

    for (size_t i = 0, n = len / 3; i < n; i++)
    {
        ImjPix clr = {};

        fread(&clr.r, 1, 1, f);
        fread(&clr.g, 1, 1, f);
        fread(&clr.b, 1, 1, f);
        clr.a = 255;

        memcpy(&(*pallete)[i], &clr, sizeof(ImjClr));
    }

    return true;
}