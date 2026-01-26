#include "gif.h"

void imj_lzwBitReader_init(ImjLzwBitReader *br, FILE *f) {
    // ---
    br->dataCap = 256;
    br->dataSize = 0;
    br->data = malloc(br->dataCap);

    while (true) {
        uint8_t subBlockSize;
        fread(&subBlockSize, 1, 1, f);
        if (subBlockSize == 0) break;

        if (br->dataSize + subBlockSize > br->dataCap) {
            br->dataCap *= 2;
            br->data = realloc(br->data, br->dataCap);
        }

        fread(br->data + br->dataSize, 1, subBlockSize, f);
        br->dataSize += subBlockSize;
    }

    // ---
    br->curIdx = 0;
    br->buffSize = 0;
    br->buff = 0;
}

ImjLzwCode imj_lzwBitReader_getCode(ImjLzwBitReader *br, const uint8_t codeSize) {
    while (br->buffSize < codeSize) {
        const byte b = br->data[br->curIdx++];
        br->buff |= (uint64_t)b << br->buffSize;
        br->buffSize += 8;
    }

    const ImjLzwCode code = br->buff & ((1u << codeSize) - 1);
    br->buff >>= codeSize;
    br->buffSize -= codeSize;

    return code;
}

void imj_lzwBitReader_reset(ImjLzwBitReader *br) {
    if (!br->data) free(br->data);
    br->curIdx = 0;
    br->buff = 0;
    br->buffSize = 0;
}

static size_t imj_lzw_expandCode(
    uint16_t code,
    const ImjGifLzwDictEntry *dict,
    uint16_t *out
) {
    size_t len = 0;

    while (code != INVALID_CODE__) {
        out[len++] = dict[code].suffix;
        code = dict[code].prefix;
    }

    // reverse in-place
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t tmp = out[i];
        out[i] = out[len - 1 - i];
        out[len - 1 - i] = tmp;
    }

    return len;
}

bool imj_gif_read(FILE *f, ImjImg *img, char err[100]) {
    if (!f || !img) {
        if (err) snprintf(err, 100, "One of the argument is null.");
        return false;
    }

    char magic[3];
    char ver[3];

    // header
    fread(magic, 1, 3, f);
    fread(ver, 1, 3, f);

    if (strncmp(magic, "GIF", 3) != 0) {
        if (err) snprintf(err, 100, "File isn't GIF.");
        return false;
    }
    if (!(strncmp(ver, "87a", 3) == 0 || strncmp(ver, "89a", 3) == 0)) {
        if (err) snprintf(err, 100, "Invalid GIF version.");
        return false;
    }

    // logical screen descriptor
    struct {
        uint16_t width;
        uint16_t height;
        byte packedFields;
        uint8_t bgClrIdx;
        uint8_t pixAspectRatio;
    } lsd;
    fread(&lsd.width, 2, 1, f);
    fread(&lsd.height, 2, 1, f);
    fread(&lsd.packedFields, 1, 1, f);
    fread(&lsd.bgClrIdx, 1, 1, f);
    fread(&lsd.pixAspectRatio, 1, 1, f);

    const uint16_t gctSize = 2 << (lsd.packedFields & 0b00000111);
    // const bool isGctSorted = (lsd.packedFields >> 3) & 1;
    // const uint8_t clrRes = (lsd.packedFields >> 4) & 0b00000111;
    const bool isGctPresent = (lsd.packedFields >> 7) & 1;

    DBG("Width: %i", lsd.width);
    DBG("Height: %i", lsd.height);
    DBG("GCT present: %s", isGctPresent ? "true" : "false");

    // global color table
    ImjClr *gct = NULL;

    if (isGctPresent) {
        gct = malloc(gctSize * sizeof(ImjClr));
        for (uint16_t i = 0; i < gctSize; i++) {
            fread(&gct[i], 1, 3, f);
            gct[i].a = 255;
        }
    }

    // ---
    bool oneImgRead = false;
    bool done = false;
    ImjPix *data = calloc(lsd.width * lsd.height, sizeof(ImjPix));

    // blocks
    byte blockId;

    bool transparency = false;
    uint16_t transparentClrIdx = 0;

    while (fread(&blockId, 1, 1, f) && !done) {
        switch (blockId) {
            case IMJ_GIF_BLOCK_IMGDESC: {
                if (oneImgRead) {
                    // TODO: skip only this block but still read following blocks in case they contain metadata
                    done = true;
                    break;
                }

                // image descriptor
                struct {
                    uint16_t x, y, w, h;
                    byte packedFields;
                } imgDesc;
                fread(&imgDesc.x, 2, 1, f);
                fread(&imgDesc.y, 2, 1, f);
                fread(&imgDesc.w, 2, 1, f);
                fread(&imgDesc.h, 2, 1, f);
                fread(&imgDesc.packedFields, 1, 1, f);

                const uint16_t lctSize = 2 << (imgDesc.packedFields & 0b00000111);
                // const bool isLctSorted = (imgDesc.packedFields >> 5) & 1;
                const bool isInterlaced = (imgDesc.packedFields >> 6) & 1;
                const bool isLctPresent = (imgDesc.packedFields >> 7) & 1;

                // local color table
                ImjClr *ct = NULL;
                ImjClr *lct = NULL;

                if (isLctPresent) {
                    lct = malloc(lctSize * sizeof(ImjClr));
                    for (uint16_t i = 0; i < lctSize; i++) {
                        fread(&lct[i], 1, 3, f);
                        lct[i].a = 255;
                    }

                    ct = lct;
                } else ct = gct;

                // image data
                ImjPix* decoded = malloc(imgDesc.w * imgDesc.h * sizeof(ImjPix));

                struct {
                    uint8_t minCodeSize;
                    ImjLzwCode clearCode, endCode, firstFreeCode;

                    uint8_t codeSize;
                    uint16_t dictSize;

                    ImjGifLzwDictEntry dict[4096];
                    uint16_t stack[4096];

                    ImjLzwCode prevCode;
                    ImjLzwCode code;

                    uint16_t firstChar;
                } ld;  // LZW decoder

                fread(&ld.minCodeSize, 1, 1, f);
                ld.clearCode = 1 << ld.minCodeSize;
                ld.endCode = ld.clearCode + 1;
                ld.firstFreeCode = ld.clearCode + 2;
                ld.codeSize = ld.minCodeSize + 1;
                ld.dictSize = ld.firstFreeCode;
                for (uint16_t i = 0; i < ld.clearCode; i++) {
                    ld.dict[i].prefix = INVALID_CODE__;
                    ld.dict[i].suffix = i;
                }
                ld.prevCode = INVALID_CODE__;
                ld.code = INVALID_CODE__;
                ld.firstChar = 0;

                ImjLzwBitReader br;
                imj_lzwBitReader_init(&br, f);

                size_t c = 0;

                while (true) {
                    ld.code = imj_lzwBitReader_getCode(&br, ld.codeSize);

                    if (ld.code == ld.clearCode) {
                        // memset(lzwDecoder.dict, 0, 4096);
                        for (uint16_t i = 0; i < ld.clearCode; i++) {
                            ld.dict[i].prefix = INVALID_CODE__;
                            ld.dict[i].suffix = i;
                        }
                        ld.codeSize = ld.minCodeSize + 1;
                        ld.dictSize = ld.firstFreeCode;
                        ld.prevCode = INVALID_CODE__;
                        continue;
                    }

                    if (ld.code == ld.endCode) {
                        break;
                    }

                    size_t outLen;
                    if (ld.code < ld.dictSize) {
                        outLen = imj_lzw_expandCode(ld.code, ld.dict, ld.stack);
                        ld.firstChar = ld.stack[0];
                    } else if (ld.code == ld.dictSize) {
                        outLen = imj_lzw_expandCode(ld.prevCode, ld.dict, ld.stack);
                        ld.stack[outLen++] = ld.firstChar;
                        ld.firstChar = ld.stack[0];
                    } else {
                        if (err) snprintf(err, 100, "Error while decoding GIF LZW data.");
                        if (lct) free(lct);
                        free(decoded);
                        free(data);
                        return false;
                    }

                    for (size_t i = 0; i < outLen; i++) {
                        decoded[c++] = ct[ld.stack[i]];
                        if (transparency && ld.stack[i] == transparentClrIdx)
                            decoded[c - 1].a = 0;
                    }

                    if (ld.prevCode != INVALID_CODE__ && ld.dictSize < 4096) {
                        ld.dict[ld.dictSize].prefix = ld.prevCode;
                        ld.dict[ld.dictSize].suffix = ld.firstChar;
                        ld.dictSize++;

                        if (ld.dictSize == (1 << ld.codeSize) && ld.codeSize < 12) {
                            ld.codeSize++;
                        }
                    }

                    ld.prevCode = ld.code;
                }

                // interlacing
                c = 0;
                if (isInterlaced) {
                    static const int passStart[4] = { 0, 4, 2, 1 };
                    static const int passStep [4] = { 8, 8, 4, 2 };
                    for (int pass = 0; pass < 4; pass++) {
                        for (int y = passStart[pass]; y < imgDesc.h; y += passStep[pass]) {
                            for (int x = 0; x < imgDesc.w; x++) {
                                int dstX = imgDesc.x + x;
                                int dstY = imgDesc.y + y;
                                data[dstY * lsd.width + dstX] = decoded[c++];
                            }
                        }
                    }
                } else {
                    for (int y = 0; y < imgDesc.h; y++) {
                        for (int x = 0; x < imgDesc.w; x++) {
                            int dstX = imgDesc.x + x;
                            int dstY = imgDesc.y + y;
                            data[dstY * lsd.width + dstX] = decoded[c++];
                        }
                    }
                }

                // ---
                oneImgRead = true;
                transparency = false;
                transparentClrIdx = 0;

                if (lct) free(lct);
                imj_lzwBitReader_reset(&br);
                free(decoded);

                break;
            }
            case IMJ_GIF_BLOCK_EXT: {
                byte extLbl;
                fread(&extLbl, 1, 1, f);

                switch (extLbl) {
                    case IMJ_GIF_SUBBLOCK_GCE: {
                        uint8_t packed;

                        fseek(f, 1, SEEK_CUR); // block size
                        fread(&packed, 1, 1, f);
                        fseek(f, 2, SEEK_CUR);  // delay
                        fread(&transparentClrIdx, 1, 1, f);
                        fseek(f, 1, SEEK_CUR);  // terminator

                        transparency = packed & 0x01;

                        break;
                    }
                    default: {
                        uint8_t subBlockSize;
                        fread(&subBlockSize, 1, 1, f);
                        while (subBlockSize != 0x00) {
                            fseek(f, subBlockSize, SEEK_CUR);
                            fread(&subBlockSize, 1, 1, f);
                        }

                        VER("Extension block %x skipped.\n", extLbl);
                    }
                }
                break;
            }
            case IMJ_GIF_BLOCK_TRAILER: {
                done = true;
                break;
            }
            default: {
                if (gct) free(gct);
                free(data);
                if (err) snprintf(err, 100, "Unknown GIF block %x.", blockId);
                return false;
            }
        }
    }

    // ---
    img->width = lsd.width;
    img->height = lsd.height;
    img->data = data;

    // ---
    if (gct) free(gct);
    return true;
}
