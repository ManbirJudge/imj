#include "bmp.h"

static uint8_t imj_extract_channel_16__(const uint16_t p, const uint32_t mask) {
    // TODO: understand logic
    const int shift = __builtin_ctz(mask);
    const int bits = __builtin_popcount(mask);
    const uint32_t v = (p & mask) >> shift;
    return (v * 255) / ((1u << bits) - 1);
}

static uint8_t imj_extract_channel_32__(const uint32_t p, const uint32_t mask) {
    if (mask == 0) return 255;

    const int shift = __builtin_ctz(mask);
    const int bits = __builtin_popcount(mask);
    const uint32_t v = (p & mask) >> shift;
    return (v * 255) / ((1u << bits) - 1);
}

bool imj_bmp_read(FILE *f, ImjImg *img, char err[100]) {
    if (!f || !img) {
        if (err) snprintf(err, 100, "One of the arguments is nullptr.");
        return false;
    }

    // headers and masks
    ImjBmpDecodeInfo di;
    if (!imj_bmp_get_decode_info(f, &di, err)) return false;

    // palette
    ImjClr *palette = NULL;

    if (di.hasPalette) {
        palette = malloc(di.paletteSize * sizeof(ImjClr));
        const uint8_t pltSize = di.pltType == IMJ_BMP_PLT_RGB ? 3 : 4;

        uint8_t bgr0[pltSize];
        for (uint32_t i = 0; i < di.paletteSize; i++) {
            fread(bgr0, pltSize, 1, f);
            palette[i].r = bgr0[2];
            palette[i].g = bgr0[1];
            palette[i].b = bgr0[0];
            palette[i].a = 255;
        }
    }

    // decoding
    ImjPix *data = malloc(di.width * di.height * sizeof(ImjPix));
    byte *rowBuf = malloc(di.rowStride);
    fseek(f, di.pixDataOffset, SEEK_SET);

    switch (di.layout) {
        case IMJ_BMP_LAYOUT_DEFAULT: {
            switch (di.bpp) {
                case 1: {
                    if (!palette) {
                        free(data);
                        free(rowBuf);
                        return false;
                    }
                    for (uint32_t y = 0; y < di.height; y++) {
                        uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, 1, di.rowStride, f);
                        for (uint32_t x = 0; x < di.width; x++) {
                            uint8_t byte = rowBuf[x >> 3];
                            uint8_t bit  = 7 - (x & 7);
                            uint8_t idx  = (byte >> bit) & 1;
                            data[dstY * di.width + x] = palette[idx];
                        }
                    }
                    break;
                }
                case 4: {
                    if (!palette) {
                        free(data);
                        free(rowBuf);
                        return false;
                    }
                    for (uint32_t y = 0; y < di.height; y++) {
                        uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, 1, di.rowStride, f);
                        for (uint32_t x = 0; x < di.width; x++) {
                            uint8_t byte = rowBuf[x >> 1];
                            data[dstY * di.width + x] = palette[(x & 1) ? (byte & 0x0F) : (byte >> 4)];
                        }
                    }
                    break;
                }
                case 8: {
                    if (!palette) {
                        free(data);
                        free(rowBuf);
                        return false;
                    }
                    for (uint32_t y = 0; y < di.height; y++) {
                        uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, 1, di.rowStride, f);
                        for (uint32_t x = 0; x < di.width; x++) {
                            data[dstY * di.width + x] = palette[rowBuf[x]];
                        }
                    }
                    break;
                }
                case 16: {
                    uint16_t buff_3;
                    for (size_t y = 0; y < di.height; y++) {
                        const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, di.rowStride, 1, f);
                        for (size_t x = 0; x < di.width; x++) {
                            // TODO: understand logic
                            const size_t pixI = dstY * di.width + x;
                            memcpy(&buff_3, rowBuf + x * 2, 2);
                            data[pixI].r = imj_extract_channel_16__(buff_3, 0x7C00);
                            data[pixI].g = imj_extract_channel_16__(buff_3, 0x03E0);
                            data[pixI].b = imj_extract_channel_16__(buff_3, 0x001F);
                            data[pixI].a = 255;
                        }
                    }
                    break;
                }
                case 24: {
                    for (size_t y = 0; y < di.height; y++) {
                        const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, di.rowStride, 1, f);
                        for (size_t x = 0; x < di.width; x++) {
                            const size_t pixI = dstY * di.width + x;
                            const byte *src = rowBuf + x * 3;
                            data[pixI].b = src[0];
                            data[pixI].g = src[1];
                            data[pixI].r = src[2];
                            data[pixI].a = 255;
                        }
                    }
                    break;
                }
                case 32: {
                    for (size_t y = 0; y < di.height; y++) {
                        const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, di.rowStride, 1, f);
                        for (size_t x = 0; x < di.width; x++) {
                            const size_t pixI = dstY * di.width + x;
                            const byte *src = rowBuf + x * 4;
                            data[pixI].r = src[2];
                            data[pixI].g = src[1];
                            data[pixI].b = src[0];
                            data[pixI].a = 255; // TODO: check if this is the correct behaviour
                        }
                    }
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP 1.");
                    free(data);
                    if (palette) free(palette);
                    free(rowBuf);
                    return false;
                }
            }
            break;
        }
        case IMJ_BMP_LAYOUT_MASKED: {
            switch (di.bpp) {
                case 16: {
                    uint16_t buff_3;
                    for (size_t y = 0; y < di.height; y++) {
                        const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, di.rowStride, 1, f);
                        for (size_t x = 0; x < di.width; x++) {
                            const size_t pixI = dstY * di.width + x;
                            memcpy(&buff_3, rowBuf + x * 2, 2);
                            data[pixI].r = imj_extract_channel_16__(buff_3, di.rMask);
                            data[pixI].g = imj_extract_channel_16__(buff_3, di.gMask);
                            data[pixI].b = imj_extract_channel_16__(buff_3, di.bMask);
                            data[pixI].a = 255;
                        }
                    }
                    break;
                }
                case 32: {
                    uint32_t buff_3;
                    for (size_t y = 0; y < di.height; y++) {
                        const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                        fread(rowBuf, di.rowStride, 1, f);
                        for (size_t x = 0; x < di.width; x++) {
                            const size_t pixI = dstY * di.width + x;
                            memcpy(&buff_3, rowBuf + x * 4, 4);
                            data[pixI].r = imj_extract_channel_32__(buff_3, di.rMask);
                            data[pixI].g = imj_extract_channel_32__(buff_3, di.gMask);
                            data[pixI].b = imj_extract_channel_32__(buff_3, di.bMask);
                            data[pixI].a = imj_extract_channel_32__(buff_3, di.aMask); // TODO: check if this is the correct behaviour
                        }
                    }
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP 3.");
                    free(data);
                    if (palette) free(palette);
                    free(rowBuf);
                    return false;
                }
            }
            break;
        }
        case IMJ_BMP_LAYOUT_RLE: {
            if (!palette) {
                free(data);
                free(rowBuf);
                return false;
            }

            switch (di.bpp) {
                case 8: {
                    byte buff_3[2];
                    size_t x = 0, y = 0;
                    bool run = true;
                    while (run && fread(&buff_3, 1, 2, f) == 2) {
                        if (buff_3[0] > 0) {
                            for (uint8_t i = 0; i < buff_3[0]; i++) {
                                const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                                data[dstY * di.width + x] = palette[buff_3[1]];
                                x++;
                            }
                        } else {
                            switch (buff_3[1]) {
                                case 0: {
                                    x = 0, y++;
                                    if (y >= di.height) run = false;
                                    break;
                                }
                                case 1: run = false; break;
                                case 2: {
                                    fread(&buff_3, 1, 2, f);
                                    x += buff_3[0];
                                    y += buff_3[1];
                                    if (y >= di.height) run = false;
                                    break;
                                }
                                default: {
                                    const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;

                                    const uint8_t n = buff_3[1];

                                    uint8_t idx;
                                    for (uint8_t i = 0; i < n; i++) {
                                        fread(&idx, 1, 1, f);
                                        data[dstY * di.width + x++] = palette[idx];
                                    }
                                    if (n & 1) fseek(f, 1, SEEK_CUR);
                                }
                            }
                        }
                    }
                    break;
                }
                case 4: {
                    byte buff_3[2];
                    size_t x = 0, y = 0;
                    bool run = true;
                    while (run && fread(&buff_3, 1, 2, f) == 2) {
                        if (buff_3[0] > 0) {
                            uint8_t idx1 = buff_3[1] >> 4;
                            uint8_t idx2 = buff_3[1] & 0x0F;
                            for (uint8_t i = 0; i < buff_3[0]; i++) {
                                const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;
                                data[dstY * di.width + x] = palette[i & 1 ? idx2 : idx1];
                                x++;
                            }
                        } else {
                            switch (buff_3[1]) {
                                case 0: {
                                    x = 0, y++;
                                    if (y >= di.height) run = false;
                                    break;
                                }
                                case 1: run = false; break;
                                case 2: {
                                    fread(&buff_3, 1, 2, f);
                                    x += buff_3[0];
                                    y += buff_3[1];
                                    if (y >= di.height) run = false;
                                    break;
                                }
                                default: {
                                    const uint32_t dstY = di.topDown ? (di.height - 1 - y) : y;

                                    const uint8_t n = buff_3[1];
                                    const uint8_t bytes = (n + 1) / 2;

                                    uint8_t px = 0;
                                    uint8_t packed;
                                    for (uint8_t i = 0; i < bytes; i++) {
                                        fread(&packed, 1, 1, f);
                                        data[dstY * di.width + x++] = palette[packed >> 4], px++;
                                        if (px < n) data[dstY * di.width + x++] = palette[packed & 0x0F], px++;
                                    }

                                    if (bytes & 1) fseek(f, 1, SEEK_CUR);
                                }
                            }
                        }
                    }
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "SHAT!");
                    free(data);
                    free(rowBuf);
                    free(palette);
                    return false;
                }
            }
            break;
        }
        default: {
            if (err) snprintf(err, 100, "Unsupported BMP 2.");
            free(data);
            if (palette) free(palette);
            free(rowBuf);
            return false;
        }
    }

    // ---
    free(rowBuf);
    if (palette) free(palette);

    img->width = di.width;
    img->height = di.height;
    img->data = data;

    return true;
}

bool imj_bmp_get_decode_info(FILE *f, ImjBmpDecodeInfo *di, char err[100]) {
    ImjBmpFileHeader fileHeader;
    fread(&fileHeader.type, 2, 1, f);
    fread(&fileHeader.size, 4, 1, f);
    fread(&fileHeader.reserved1, 2, 1, f);
    fread(&fileHeader.reserved2, 2, 1, f);
    fread(&fileHeader.offset, 4, 1, f);

    if (fileHeader.type != 0x4D42) {
        if (err) snprintf(err, 100, "Invalid BMP file.");
        return false;
    }

    uint32_t dibHeaderSize;
    fread(&dibHeaderSize, 4, 1, f);
    fseek(f, -4, SEEK_CUR);

    switch (dibHeaderSize) {
        case IMJ_BMP_CORE_HEADER: {
            DBG("Opening OS/2 core BMP.");

            ImjBmpCoreHeader header;
            fread(&header, 12, 1, f);

            di->width = header.width;
            di->height = header.height;
            di->bpp = header.bpp;
            di->topDown = true;

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = 1 << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB;
            }

            di->layout = IMJ_BMP_LAYOUT_DEFAULT;
            di->pixDataOffset = fileHeader.offset;
            // di->rowStride = ((di->width * di->bpp + 15) / 16) * 2;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_CORE_EXT_HEADER: {
            ImjBmpCoreExtHeader header;
            fread(&header, 16, 1, f);

            di->width = header.width;
            di->height = header.height;
            di->bpp = header.bpp;
            di->topDown = true;

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = 1 << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            di->layout = IMJ_BMP_LAYOUT_DEFAULT;
            di->pixDataOffset = fileHeader.offset;
            // di->rowStride = ((di->width * di->bpp + 15) / 16) * 2;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_OS2_HEADER: {
            ImjBmpOS2Header header;
            fread(&header, 64, 1, f);

            di->width = header.width;
            di->height = header.height;
            di->bpp = header.bpp;
            di->topDown = header.recording == 0;

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            di->layout = (header.compression == IMJ_BMP_COMPRESSION_RGB) ? IMJ_BMP_LAYOUT_DEFAULT: IMJ_BMP_LAYOUT_RLE;

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;
            break;
        }
        case IMJ_BMP_INFO_HEADER: {
            ImjBmpInfoHeader header;
            fread(&header, 40, 1, f);

            di->width = abs(header.width);
            di->height = abs(header.height);
            di->topDown = header.height > 0;
            di->bpp = header.bpp;

            switch (header.compression) {
                case IMJ_BMP_COMPRESSION_RGB: {
                    di->layout = IMJ_BMP_LAYOUT_DEFAULT;
                    break;
                }
                case IMJ_BMP_COMPRESSION_BITFIELDS: {
                    di->layout = IMJ_BMP_LAYOUT_MASKED;
                    di->isMasked = true;
                    break;
                }
                case IMJ_BMP_COMPRESSION_RLE4:
                case IMJ_BMP_COMPRESSION_RLE8: {
                    di->layout = IMJ_BMP_LAYOUT_RLE;
                    break;
                }
                case IMJ_BMP_COMPRESSION_PNG:
                case IMJ_BMP_COMPRESSION_JPEG: {
                    di->layout = IMJ_BMP_LAYOUT_EMBEDDED;
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP compression %i.", header.compression);
                    return false;
                }
            }

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            if (di->layout == IMJ_BMP_LAYOUT_MASKED) {
                fread(&di->rMask, 4, 1, f);
                fread(&di->gMask, 4, 1, f);
                fread(&di->bMask, 4, 1, f);
                di->aMask = 0;
            }

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_V2_INFO_HEADER: {
            ImjBmpV2InfoHeader header;
            fread(&header, 52, 1, f);

            di->width = abs(header.width);
            di->height = abs(header.height);
            di->topDown = header.height > 0;
            di->bpp = header.bpp;

            switch (header.compression) {
                case IMJ_BMP_COMPRESSION_RGB: {
                    di->layout = IMJ_BMP_LAYOUT_DEFAULT;
                    break;
                }
                case IMJ_BMP_COMPRESSION_BITFIELDS: {
                    di->layout = IMJ_BMP_LAYOUT_MASKED;
                    di->isMasked = true;
                    break;
                }
                case IMJ_BMP_COMPRESSION_RLE4:
                case IMJ_BMP_COMPRESSION_RLE8: {
                    di->layout = IMJ_BMP_LAYOUT_RLE;
                    break;
                }
                case IMJ_BMP_COMPRESSION_PNG:
                case IMJ_BMP_COMPRESSION_JPEG: {
                    di->layout = IMJ_BMP_LAYOUT_EMBEDDED;
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP compression %i.", header.compression);
                    return false;
                }
            }

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            if (di->layout == IMJ_BMP_LAYOUT_MASKED) {
                di->rMask = header.rMask;
                di->gMask = header.gMask;
                di->bMask = header.bMask;
                di->aMask = 0;
            }

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_V3_INFO_HEADER: {
            ImjBmpV3InfoHeader header;
            fread(&header, 56, 1, f);

            di->width = abs(header.width);
            di->height = abs(header.height);
            di->topDown = header.height > 0;
            di->bpp = header.bpp;

            switch (header.compression) {
                case IMJ_BMP_COMPRESSION_RGB: {
                    di->layout = IMJ_BMP_LAYOUT_DEFAULT;
                    break;
                }
                case IMJ_BMP_COMPRESSION_BITFIELDS:
                case IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS: {
                    di->layout = IMJ_BMP_LAYOUT_MASKED;
                    di->isMasked = true;
                    break;
                }
                case IMJ_BMP_COMPRESSION_RLE4:
                case IMJ_BMP_COMPRESSION_RLE8: {
                    di->layout = IMJ_BMP_LAYOUT_RLE;
                    break;
                }
                case IMJ_BMP_COMPRESSION_PNG:
                case IMJ_BMP_COMPRESSION_JPEG: {
                    di->layout = IMJ_BMP_LAYOUT_EMBEDDED;
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP compression %i.", header.compression);
                    return false;
                }
            }

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            if (di->layout == IMJ_BMP_LAYOUT_MASKED) {
                di->isMasked = true;
                di->rMask = header.rMask;
                di->gMask = header.gMask;
                di->bMask = header.bMask;
                if (header.compression == IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS) di->aMask = header.aMask; else di->aMask = 0;
            }

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_V4_HEADER: {
            ImjBmpV4Header header;
            fread(&header, 108, 1, f);

            di->width = abs(header.width);
            di->height = abs(header.height);
            di->topDown = header.height > 0;
            di->bpp = header.bpp;

            switch (header.compression) {
                case IMJ_BMP_COMPRESSION_RGB: {
                    di->layout = IMJ_BMP_LAYOUT_DEFAULT;
                    break;
                }
                case IMJ_BMP_COMPRESSION_BITFIELDS:
                case IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS: {
                    di->layout = IMJ_BMP_LAYOUT_MASKED;
                    di->isMasked = true;
                    break;
                }
                case IMJ_BMP_COMPRESSION_RLE4:
                case IMJ_BMP_COMPRESSION_RLE8: {
                    di->layout = IMJ_BMP_LAYOUT_RLE;
                    break;
                }
                case IMJ_BMP_COMPRESSION_PNG:
                case IMJ_BMP_COMPRESSION_JPEG: {
                    di->layout = IMJ_BMP_LAYOUT_EMBEDDED;
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP compression %i.", header.compression);
                    return false;
                }
            }

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            if (di->layout == IMJ_BMP_LAYOUT_MASKED) {
                di->isMasked = true;
                di->rMask = header.rMask;
                di->gMask = header.gMask;
                di->bMask = header.bMask;
                if (header.compression == IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS) di->aMask = header.aMask; else di->aMask = 0;
            }

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        case IMJ_BMP_V5_HEADER: {
            ImjBmpV5Header header;
            fread(&header, 124, 1, f);

            di->width = abs(header.width);
            di->height = abs(header.height);
            di->topDown = header.height > 0;
            di->bpp = header.bpp;

            switch (header.compression) {
                case IMJ_BMP_COMPRESSION_RGB: {
                    di->layout = IMJ_BMP_LAYOUT_DEFAULT;
                    break;
                }
                case IMJ_BMP_COMPRESSION_BITFIELDS:
                case IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS: {
                    di->layout = IMJ_BMP_LAYOUT_MASKED;
                    di->isMasked = true;
                    break;
                }
                case IMJ_BMP_COMPRESSION_RLE4:
                case IMJ_BMP_COMPRESSION_RLE8: {
                    di->layout = IMJ_BMP_LAYOUT_RLE;
                    break;
                }
                case IMJ_BMP_COMPRESSION_PNG:
                case IMJ_BMP_COMPRESSION_JPEG: {
                    di->layout = IMJ_BMP_LAYOUT_EMBEDDED;
                    break;
                }
                default: {
                    if (err) snprintf(err, 100, "Unsupported BMP compression %i.", header.compression);
                    return false;
                }
            }

            di->hasPalette = di->bpp <= 8;
            if (di->hasPalette) {
                di->paletteSize = header.nClrUsed ? header.nClrUsed : 1u << di->bpp;
                di->pltType = IMJ_BMP_PLT_RGB0;
            }

            if (di->layout == IMJ_BMP_LAYOUT_MASKED) {
                di->isMasked = true;
                di->rMask = header.rMask;
                di->gMask = header.gMask;
                di->bMask = header.bMask;
                if (header.compression == IMJ_BMP_COMPRESSION_ALPHA_BITFIELDS) di->aMask = header.aMask; else di->aMask = 0;
            }

            di->pixDataOffset = fileHeader.offset;
            di->rowStride = ((di->width * di->bpp + 31) / 32) * 4;

            break;
        }
        default: {
            if (err) snprintf(err, 100, "BMP DIB header of size %i not supported.", dibHeaderSize);
            return false;
        }
    }

    // checks
    VER("%ix%i | %i bits/pix\n", di->width, di->height, di->bpp);

    switch (di->bpp) {
        case 1:
        case 4:
        case 8:
        case 16:
        case 24:
        case 32:
            break;
        default: {
            if (err) snprintf(err, 100, "Unsupported bit-depth %i.", di->bpp);
            return false;
        }
    }

    if (di->layout == IMJ_BMP_LAYOUT_EMBEDDED) {
        if (err) snprintf(err, 100, "Embedded BMPs not supported.");
        return false;
    }
    if (di->layout == IMJ_BMP_LAYOUT_RLE && !di->hasPalette) {
        if (err) snprintf(err, 100, "RLE without palette is invalid.");
        return false;
    }
    if (di->layout == IMJ_BMP_LAYOUT_MASKED && !di->isMasked) {
        if (err) snprintf(err, 100, "Bit fields without masks is invalid.");
        return false;
    }

    // ---
    return true;
}
