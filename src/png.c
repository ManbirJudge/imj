#include "png.h"

static uint8_t imj_png_paeth__(const uint8_t a, const uint8_t b, const uint8_t c) {
    const int p = (int) a + (int) b - (int) c;
    const int pa = abs(p - a);
    const int pb = abs(p - b);
    const int pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

bool imj_png_read(FILE *f, ImjImg *img, char err[100]) {
    if (!f || !img) {
        if (err) snprintf(err, 100, "One of the arguments is NULL.");
        return false;
    }

    // --- signature
    DBG("Checking signature...");

    const byte PNG_SIGN[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    byte buff1[8];

    fread(buff1, 1, 8, f);
    if (memcmp(buff1, PNG_SIGN, 8) != 0) {
        if (err) snprintf(err, 100, "Invalid PNG file signature: \'%.*s\'.", 4, buff1);
        return false;
    }

    // --- chunks
    DBG("Reading chunks...");

    ImjPngIhdr ihdr = {0};
    ImjClr *palette = NULL;
    ImjPngDataInfo di = {0};

    bool isTransparent = false;  // for gray, rgb and indexed
    uint16_t trnsGrayKey = 0;
    uint16_t trnsRKey, trnsGKey, trnsBKey = 0;

    ImjPix *data = NULL;

    uint32_t chunkLen;
    unsigned char chunkType[4];
    while (fread(&chunkLen, 4, 1, f)) {
        imj_swap_uint32(&chunkLen);
        fread(chunkType, 1, 4, f);

        if (memcmp(chunkType, "IHDR", 4) == 0) {
            if (!imj_png_read_IHDR(f, &ihdr, err)) return false;

            di.compressedData = (byte *) malloc(1024);
            di.compressedDataCap = 1024;
        } else if (memcmp(chunkType, "PLTE", 4) == 0) {
            if (!imj_png_read_PLTE(f, &palette, chunkLen, err)) {
                if (di.compressedData) free(di.compressedData);
                return false;
            }
        } else if (memcmp(chunkType, "IDAT", 4) == 0) {
            while (di.compressedDataSize + chunkLen >= di.compressedDataCap) {
                di.compressedDataCap *= 2;
                di.compressedData = realloc(di.compressedData, di.compressedDataCap);
            }
            fread(di.compressedData + di.compressedDataSize, 1, chunkLen, f);
            di.compressedDataSize += chunkLen;
        } else if (memcmp(chunkType, "tRNS", 4) == 0) {
            isTransparent = true;
            switch (ihdr.colorType) {
                case IMJ_PNG_CLRTYPE_GRAY:{
                    fread(&trnsGrayKey, 2, 1, f);
                    imj_swap_uint16(&trnsGrayKey);
                    break;
                }
                case IMJ_PNG_CLRTYPE_RGB: {
                    fread(&trnsRKey, 2, 1, f);
                    fread(&trnsGKey, 2, 1, f);
                    fread(&trnsBKey, 2, 1, f);
                    imj_swap_uint16(&trnsRKey);
                    imj_swap_uint16(&trnsGKey);
                    imj_swap_uint16(&trnsBKey);
                    break;
                }
                case IMJ_PNG_CLRTYPE_IDX: {
                    for (size_t i = 0; i < chunkLen; i++)
                        fread(&palette[i].a, 1, 1, f);
                    break;
                }
            }
        } else if (memcmp(chunkType, "IEND", 4) == 0) break;
        else {
            if (isupper(chunkType[0])) {
                if (err) snprintf(err, 100, "Unknown critical chunk: %4s", chunkType);
                return false;
            }
            WRN("Ignoring unknown ancillary chunk of length: %i", chunkLen);
            fseek(f, chunkLen, SEEK_CUR);
        }

        fseek(f, 4, SEEK_CUR); // crc
    }

    // --- image data
    // decompression
    DBG("Decompressing...");

    di.decompressedData = (byte *) malloc(ihdr.height * (1 + ihdr.width * 8));
    // TODO: where does '4' come from? i guess maximum possible color type (RGBA) + bit-depth (16) combination?
    mz_ulong decompressedSize = ihdr.height * (1 + ihdr.width * 8);

    int status = mz_uncompress(
        di.decompressedData,
        &decompressedSize,
        di.compressedData,
        di.compressedDataSize
    );

    free(di.compressedData);
    di.compressedData = NULL;

    if (status != MZ_OK) {
        if (err) snprintf(err, 100, "IDAT decompression failed. (code: %i)", status);
        free(di.decompressedData);
        return false;
    }

    // parameters
    uint8_t samplesPerPixel = 0;
    switch (ihdr.colorType) {
        case IMJ_PNG_CLRTYPE_GRAY:
            samplesPerPixel = 1;
            break;
        case IMJ_PNG_CLRTYPE_RGB:
            samplesPerPixel = 3;
            break;
        case IMJ_PNG_CLRTYPE_IDX:
            samplesPerPixel = 1;
            break;
        case IMJ_PNG_CLRTYPE_GRAYALPHA:
            samplesPerPixel = 2;
            break;
        case IMJ_PNG_CLRTYPE_RGBA:
            samplesPerPixel = 4;
            break;
    }

    size_t filterBpp = (samplesPerPixel * ihdr.bitDepth + 7) / 8;
    if (filterBpp < 1) filterBpp = 1;

    size_t bytesPerScanline = (size_t) ceil((double) ihdr.width * samplesPerPixel * ihdr.bitDepth / 8.);

    // un-filtering
    DBG("Un-filtering...");

    di.filteredData = (byte *) malloc(ihdr.height * bytesPerScanline);

    byte *prevUnfilteredScanline = malloc(bytesPerScanline);
    memset(prevUnfilteredScanline, 0, bytesPerScanline);
    byte *unfilteredScanline = malloc(bytesPerScanline);
    byte *scanline = malloc(bytesPerScanline);

    for (uint32_t i = 0, nScanlines = ihdr.height; i < nScanlines; i++) {
        size_t scanlineStart = i * (bytesPerScanline + 1);
        memcpy(scanline, di.decompressedData + scanlineStart + 1, bytesPerScanline);
        switch (di.decompressedData[scanlineStart]) {
            case IMJ_PNG_FILTER_NONE: {
                memcpy(unfilteredScanline, scanline, bytesPerScanline);
                break;
            }
            case IMJ_PNG_FILTER_SUB: {
                size_t j = 0;
                for (; j < filterBpp && j < bytesPerScanline; j++)
                    unfilteredScanline[j] = scanline[j];
                for (; j < bytesPerScanline; j++)
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + unfilteredScanline[j - filterBpp]);
                break;
            }
            case IMJ_PNG_FILTER_UP: {
                for (size_t j = 0; j < bytesPerScanline; j++)
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + prevUnfilteredScanline[j]);
                break;
            }
            case IMJ_PNG_FILTER_AVG: {
                size_t j = 0;
                for (; j < filterBpp && j < bytesPerScanline; j++) {
                    uint16_t y = prevUnfilteredScanline[j];
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + (y / 2));
                }
                for (; j < bytesPerScanline; j++) {
                    uint16_t x = unfilteredScanline[j - filterBpp];
                    uint16_t y = prevUnfilteredScanline[j];
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + ((x + y) / 2));
                }
                break;
            }
            case IMJ_PNG_FILTER_PAETH: {
                size_t j = 0;
                for (; j < filterBpp && j < bytesPerScanline; j++) {
                    uint16_t b = prevUnfilteredScanline[j];
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + imj_png_paeth__(0, b, 0));
                }
                for (; j < bytesPerScanline; j++) {
                    uint16_t a = unfilteredScanline[j - filterBpp];
                    uint16_t b = prevUnfilteredScanline[j];
                    uint16_t c = prevUnfilteredScanline[j - filterBpp];
                    unfilteredScanline[j] = (uint8_t) (scanline[j] + imj_png_paeth__(a, b, c));
                }
                break;
            }
            default: {
                snprintf(err, 100, "Unknown filter type.");
                free(di.filteredData);
                free(prevUnfilteredScanline);
                free(unfilteredScanline);
                free(scanline);
                return false;
            }
        }

        memcpy(prevUnfilteredScanline, unfilteredScanline, bytesPerScanline);
        memcpy(di.filteredData + i * bytesPerScanline, unfilteredScanline, bytesPerScanline);
    }

    free(di.decompressedData);
    di.decompressedData = NULL;
    free(prevUnfilteredScanline);
    free(unfilteredScanline);
    free(scanline);

    // assembling image data | TODO: use pointer incrementation instead of calculating k
    DBG("Assembling image data...");
    data = (ImjPix *) malloc(ihdr.width * ihdr.height * sizeof(ImjPix));

    ImjPngSamplerInfo si;
    imj_png_sampler_init(&si, &ihdr, &di);

    trnsGrayKey &= si.mask;
    trnsRKey &= si.mask;
    trnsGKey &= si.mask;
    trnsBKey &= si.mask;

    switch (ihdr.colorType) {
        case IMJ_PNG_CLRTYPE_GRAY: {
            if (isTransparent) {
                for (uint32_t i = 0; i < ihdr.height; i++) {
                    si.buff = 0;
                    si.buffSize = 0;
                    for (uint32_t j = 0; j < ihdr.width; j++) {
                        size_t k = (size_t) i * ihdr.width + j;
                        uint16_t raw = imj_png_sampler_nextSample(&si);
                        uint8_t s = ((uint32_t)raw * 255) / si.maxVal;
                        data[k].r = s;
                        data[k].g = s;
                        data[k].b = s;
                        data[k].a = raw == trnsGrayKey ? 0 : 255;
                    }
                }
            } else {
                for (uint32_t i = 0; i < ihdr.height; i++) {
                    si.buff = 0;
                    si.buffSize = 0;
                    for (uint32_t j = 0; j < ihdr.width; j++) {
                        size_t k = (size_t) i * ihdr.width + j;
                        uint8_t s = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;

                        data[k].r = s;
                        data[k].g = s;
                        data[k].b = s;
                        data[k].a = 255;
                    }
                }
            }
            break;
        }
        case IMJ_PNG_CLRTYPE_RGB: {
            if (isTransparent) {
                for (uint32_t i = 0; i < ihdr.height; i++) {
                    si.buff = 0;
                    si.buffSize = 0;
                    for (uint32_t j = 0; j < ihdr.width; j++) {
                        size_t k = (size_t) i * ihdr.width + j;

                        uint16_t rawR = imj_png_sampler_nextSample(&si);
                        uint16_t rawG = imj_png_sampler_nextSample(&si);
                        uint16_t rawB = imj_png_sampler_nextSample(&si);

                        data[k].r = ((uint32_t) rawR * 255) / si.maxVal;
                        data[k].g = ((uint32_t) rawG * 255) / si.maxVal;
                        data[k].b = ((uint32_t) rawB * 255) / si.maxVal;
                        data[k].a = (((rawR ^ trnsRKey) | (rawG ^ trnsGKey) | (rawB ^ trnsBKey)) == 0) ? 0 : 255;
                    }
                }
            } else {
                for (uint32_t i = 0; i < ihdr.height; i++) {
                    si.buff = 0;
                    si.buffSize = 0;
                    for (uint32_t j = 0; j < ihdr.width; j++) {
                        size_t k = (size_t) i * ihdr.width + j;

                        data[k].r = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                        data[k].g = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                        data[k].b = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                        data[k].a = 255;
                    }
                }
            }
            break;
        }
        case IMJ_PNG_CLRTYPE_IDX: {
            if (!palette) {
                snprintf(err, 100, "Pallete chunk didn't appear for indexed PNG.");
                return false;
            }
            for (uint32_t i = 0; i < ihdr.height; i++) {
                si.buff = 0;
                si.buffSize = 0;
                for (uint32_t j = 0; j < ihdr.width; j++) {
                    size_t k = (size_t) i * ihdr.width + j;
                    data[k] = palette[imj_png_sampler_nextSample(&si)];
                }
            }
            break;
        }
        case IMJ_PNG_CLRTYPE_GRAYALPHA: {
            for (uint32_t i = 0; i < ihdr.height; i++) {
                si.buff = 0;
                si.buffSize = 0;
                for (uint32_t j = 0; j < ihdr.width; j++) {
                    size_t k = (size_t) i * ihdr.width + j;
                    uint8_t s = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;

                    data[k].r = s;
                    data[k].g = s;
                    data[k].b = s;
                    data[k].a = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                }
            }
            break;
        }
        case IMJ_PNG_CLRTYPE_RGBA: {
            for (uint32_t i = 0; i < ihdr.height; i++) {
                si.buff = 0;
                si.buffSize = 0;
                for (uint32_t j = 0; j < ihdr.width; j++) {
                    size_t k = (size_t) i * ihdr.width + j;

                    data[k].r = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                    data[k].g = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                    data[k].b = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                    data[k].a = ((uint32_t)imj_png_sampler_nextSample(&si) * 255) / si.maxVal;
                }
            }
            break;
        }
    }

    // ---
    img->width = ihdr.width;
    img->height = ihdr.height;
    img->data = data;

    // --- cleanup
    if (palette) free(palette);
    if (di.filteredData) free(di.filteredData);

    // ---
    return true;
}

bool imj_png_read_IHDR(FILE *f, ImjPngIhdr *ihdr, char err[100]) {
    // reading
    fread(&ihdr->width, 4, 1, f);
    fread(&ihdr->height, 4, 1, f);
    fread(&ihdr->bitDepth, 1, 1, f);
    fread(&ihdr->colorType, 1, 1, f);
    fread(&ihdr->compressionMethod, 1, 1, f);
    fread(&ihdr->filterMethod, 1, 1, f);
    fread(&ihdr->interlaceMethod, 1, 1, f);

    imj_swap_uint32(&ihdr->width);
    imj_swap_uint32(&ihdr->height);

    imj_png_dbg_IHDR(ihdr);

    // width, height check | TODO: check before swapping endian-ness
    if (ihdr->width == 0 || (ihdr->width & 0x80000000) != 0) {
        snprintf(err, 100, "Invalid width.");
        return false;
    }
    if (ihdr->height == 0 || (ihdr->height & 0x80000000) != 0) {
        snprintf(err, 100, "Invalid width.");
        return false;
    }

    // bit-depht check
    if (!(ihdr->bitDepth == 1 || ihdr->bitDepth == 2 || ihdr->bitDepth == 4 || ihdr->bitDepth == 8 || ihdr->bitDepth ==
          16)) {
        snprintf(err, 100, "Invalid bit-depth.");
        return false;
    }

    // color type check | TODO: use a 'switch'
    switch (ihdr->colorType) {
        case 0:
            break;
        case 2:
            if (!(ihdr->bitDepth == 8 || ihdr->bitDepth == 16)) {
                snprintf(err, 100, "Invalid bit-depth for RGB color type (2).");
                return false;
            }
            break;
        case 3:
            if (!(ihdr->bitDepth == 1 || ihdr->bitDepth == 2 || ihdr->bitDepth == 4 || ihdr->bitDepth ==
                  8)) {
                snprintf(err, 100, "Invalid bit-depth for palette-index color type (3).");
                return false;
            }
            break;
        case 4:
            if (!(ihdr->bitDepth == 8 || ihdr->bitDepth == 16)) {
                snprintf(err, 100, "Invalid bit-depth for grayscale-alpha color type (4).");
                return false;
            }
            break;
        case 6:
            if (!(ihdr->bitDepth == 8 || ihdr->bitDepth == 16)) {
                snprintf(err, 100, "Invalid bit-depth for RGBA color type (6).");
                return false;
            }
            break;
        default:
            snprintf(err, 100, "Invalid color type.");
            return false;
    }

    // other checks
    if (ihdr->compressionMethod != 0) {
        snprintf(err, 100, "Invalid compression method.");
        return false;
    }
    if (ihdr->filterMethod != 0) {
        snprintf(err, 100, "Invalid filther method.");
        return false;
    }
    if (ihdr->interlaceMethod != 0) {
        snprintf(err, 100, "Only non-interlaced PNGs are supported (yet).");
        return false;
    }

    // ---
    return true;
}

bool imj_png_read_PLTE(FILE *f, ImjClr **palette, const uint32_t len, char err[100]) {
    if (len % 3 != 0) {
        snprintf(err, 100, "PLTE chunk size must be a multiple of 3.");
        return false;
    }
    if (*palette) {
        snprintf(err, 100, "There must only be 1 PLTE chunk.");
        return false;
    }

    // TODO: max entries < possible due to bit-depth
    *palette = malloc(len / 3 * sizeof(ImjClr));

    for (size_t i = 0, n = len / 3; i < n; i++) {
        ImjPix clr = {};

        fread(&clr.r, 1, 1, f);
        fread(&clr.g, 1, 1, f);
        fread(&clr.b, 1, 1, f);
        clr.a = 255;

        memcpy(&(*palette)[i], &clr, sizeof(ImjClr));
    }

    return true;
}

void imj_png_sampler_init(ImjPngSamplerInfo *si, ImjPngIhdr *ihdr, ImjPngDataInfo *di) {
    si->bitDepth = ihdr->bitDepth;
    si->mask = (1 << ihdr->bitDepth) - 1;
    si->maxVal = si->mask; // coincidently, it's the same
    si->filteredData = di->filteredData; // TODO: check for null
    si->curPos = 0;
    si->buff = 0;
    si->buffSize = 0;
}

uint16_t imj_png_sampler_nextSample(ImjPngSamplerInfo *si) {
    while (si->buffSize < si->bitDepth) {
        si->buff <<= 8;
        si->buff |= si->filteredData[si->curPos++];
        si->buffSize += 8;
    }

    const uint16_t shift = si->buffSize - si->bitDepth;
    const uint16_t ret = (si->buff >> shift) & si->mask;
    si->buffSize -= si->bitDepth;
    si->buff &= (1 << si->buffSize) - 1;

    return ret;
}

void imj_png_dbg_IHDR(ImjPngIhdr *ihdr) {
    DBG("IHDR:");
    VER("Width: %i\n", ihdr->width);
    VER("Height: %i\n", ihdr->height);
    VER("Bit-depth: %i\n", ihdr->bitDepth);
    VER("Color type: %i\n", ihdr->colorType);
    VER("Compression method: %i\n", ihdr->compressionMethod);
    VER("Filter method: %i\n", ihdr->filterMethod);
    VER("Interlace method: %i\n", ihdr->interlaceMethod);
}
