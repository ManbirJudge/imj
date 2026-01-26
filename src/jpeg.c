#include "jpeg.h"

static uint8_t imj_clamp__(const int val) {
    if (val < 0)
        return 0;
    if (val > 255)
        return 255;
    return (uint8_t) val;
}

static ImjPix imj_YCbCr2Pix__(const int Y, const int Cb, const int Cr) {
    const double r = Cr * (2.0 - 2.0 * 0.299) + Y;
    const double b = Cb * (2.0 - 2.0 * 0.114) + Y;
    const double g = (Y - 0.114 * b - 0.299 * r) / 0.587;

    return (ImjPix){
        imj_clamp__((int) (r + 128)),
        imj_clamp__((int) (g + 128)),
        imj_clamp__((int) (b + 128)),
        255
    };
}

static void imj_upsampleScalInt__(size_t srcW, size_t srcH, size_t scaleX, size_t scaleY, int src[srcH][srcW], int dst[srcH * scaleX][srcW * scaleY]) {
    size_t a = srcW * scaleX;
    size_t b = srcH * scaleY;

    for (size_t y = 0; y < b; y++) {
        for (size_t x = 0; x < a; x++) {
            dst[y][x] = src[y / scaleY][x / scaleX];
        }
    }
}

// ---
uint16_t imj_jpeg_stream_read(ImjJpegStream *s, const size_t n) {
    uint32_t res = 0;

    for (uint8_t i = 0; i < n; i++) {
        if (s->pos >= s->len * 8) return res;

        const byte b = s->data[s->pos >> 3];
        const uint8_t shift = 7 - (s->pos & 0x7);

        res = (res << 1) | ((b >> shift) & 1);
        s->pos++;
    }

    return res;
}

void jpeg_stream_align(ImjJpegStream *s) {
    s->pos = (s->pos + 7) & ~7;
}

// ---
uint16_t imj_jpeg_HT_getCode(ImjJpegHt *ht, ImjJpegStream *s) {
    uint16_t code = 0;
    for (int x = 1; x <= 16; x++) {
        uint16_t bit = imj_jpeg_stream_read(s, 1);
        code = (code << 1) | bit;
        for (int y = 0; y < ht->size; y++) {
            const ImjJpegHtEntry z = ht->table[y];
            if (z.codeLen == x && z.code == code) return z.symbol;
        }
    }
    return -1;
}

int jpeg_decode_num(uint16_t bits, uint8_t len) {
    if (bits >= (1 << (len - 1)))
        return bits;
    return bits - ((1 << len) - 1);
}

int imj_jpeg_buildMat(ImjJpegStream *s, ImjJpegHt *dcHt, ImjJpegHt *acHt, uint8_t qt[64], int oldDcCoeff, int mat[8][8]) {
    // setup
    static const uint8_t precision = 8;
    static const float idctTable[8][8] = {
        {
            0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475,
            0.7071067811865475, 0.7071067811865475, 0.7071067811865475
        },
        {
            0.9807852804032304, 0.8314696123025452, 0.5555702330196023, 0.19509032201612833, -0.1950903220161282,
            -0.555570233019602, -0.8314696123025453, -0.9807852804032304
        },
        {
            0.9238795325112867, 0.38268343236508984, -0.3826834323650897, -0.9238795325112867, -0.9238795325112868,
            -0.38268343236509034, 0.38268343236509, 0.9238795325112865
        },
        {
            0.8314696123025452, -0.1950903220161282, -0.9807852804032304, -0.5555702330196022, 0.5555702330196018,
            0.9807852804032304, 0.19509032201612878, -0.8314696123025451
        },
        {
            0.7071067811865476, -0.7071067811865475, -0.7071067811865477, 0.7071067811865474, 0.7071067811865477,
            -0.7071067811865467, -0.7071067811865471, 0.7071067811865466
        },
        {
            0.5555702330196023, -0.9807852804032304, 0.1950903220161283, 0.8314696123025456, -0.8314696123025451,
            -0.19509032201612803, 0.9807852804032307, -0.5555702330196015
        },
        {
            0.38268343236508984, -0.9238795325112868, 0.9238795325112865, -0.3826834323650899, -0.38268343236509056,
            0.9238795325112867, -0.9238795325112864, 0.38268343236508956
        },
        {
            0.19509032201612833, -0.5555702330196022, 0.8314696123025456, -0.9807852804032307, 0.9807852804032305,
            -0.831469612302545, 0.5555702330196015, -0.19509032201612858
        },
    };
    static const uint8_t zigzag[64] = {
        0, 1, 5, 6, 14, 15, 27, 28,
        2, 4, 7, 13, 16, 26, 29, 42,
        3, 8, 12, 17, 25, 30, 41, 43,
        9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
    };

    // reading
    int _buffMat1[64] = {0}; // init reading
    int _buffMat2[8][8]; // zigzag

    uint8_t _htCode = imj_jpeg_HT_getCode(dcHt, s);
    uint16_t _bits;
    int _dcCoeff;
    if (_htCode != 0) {
        _bits = imj_jpeg_stream_read(s, _htCode);
        _dcCoeff = jpeg_decode_num(_bits, _htCode) + oldDcCoeff;
    } else {
        _bits = 0;
        _dcCoeff = oldDcCoeff;
    }

    _buffMat1[0] = _dcCoeff * qt[0];

    int l = 1;
    while (l < 64) {
        _htCode = imj_jpeg_HT_getCode(acHt, s);
        if (_htCode == 0)
            break;

        if (_htCode == 0xF0) {
            l += 16;
            continue;
        }

        uint8_t run = _htCode >> 4;
        uint8_t size = _htCode & 0x0F;
        l += run;

        if (size > 0 && l < 64) {
            _bits = imj_jpeg_stream_read(s, size);
            _buffMat1[l] = jpeg_decode_num(_bits, size) * qt[l];
            l++;
        }
    }

    // zigzag
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            _buffMat2[i][j] = _buffMat1[zigzag[i * 8 + j]];
        }
    }

    // inverse DCT
    for (uint8_t x = 0; x < 8; x++) {
        for (uint8_t y = 0; y < 8; y++) {
            float S = 0;

            for (uint8_t u = 0; u < precision; u++) {
                for (uint8_t v = 0; v < precision; v++) {
                    S += _buffMat2[v][u] * idctTable[u][x] * idctTable[v][y];
                }
            }

            mat[y][x] = S / 4;
        }
    }

    // ---
    return _dcCoeff;
}

// ---
bool imj_jpeg_read(FILE *f, ImjImg *img, char err[100]) {
    ImjJpegHt dcTables[4] = {
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL}
    };
    ImjJpegHt acTables[4] = {
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL}
    };
    uint8_t quantTables[4][64];
    ImjJpegFrameData frameData = {};

    ImjPix *imgData = NULL;

    // ---
    uint16_t marker;
    uint16_t len;

    while (fread(&marker, 1, 2, f)) {
        imj_swap_uint16(&marker);
        switch (marker) {
            case IMJ_JPEG_MARKER_SOI: continue;
            case IMJ_JPEG_MARKER_DQT: {
                if (!imj_jpeg_read_DQT(f, quantTables, err)) return false;
                continue;
            }
            case IMJ_JPEG_MARKER_SOF0: {
                if (!imj_jpeg_read_SOF0(f, &frameData, err)) return false;
                imgData = (ImjPix *) malloc(frameData.width * frameData.height * sizeof(ImjPix));
                continue;
            }
            case IMJ_JPEG_MARKER_DHT: {
                if (!imj_jpeg_read_DHT(f, dcTables, acTables, err)) return false;
                continue;
            }
            case IMJ_JPEG_MARKER_SOS: {
                if (imgData == NULL) {
                    if (err) snprintf(err, 100, "Image data is not initialized before SOS.");
                    return false;
                }

                // ---
                fread(&len, 2, 1, f);
                imj_swap_uint16(&len);
                len -= 2;

                // ---
                fseek(f, 1, SEEK_CUR);

                ImjJpegSosComponent components[frameData.nComp];

                for (uint8_t i = 0; i < frameData.nComp; i++) {
                    fread(&(components[i].id), 1, 1, f);
                    fread(&(components[i].dcHtId), 1, 1, f);

                    components[i].acHtId = components[i].dcHtId & 0x0f;
                    components[i].dcHtId >>= 4;
                }

                fseek(f, 3, SEEK_CUR); // TODO: for progressive JPEGs

                // 0xff00 -> 0xff
                byte _buff1;
                byte _buff2;

                uint32_t cap = 1024;
                byte *data = (byte *) malloc(cap);
                uint32_t dataLen = 0;

                while (true) {
                    if (dataLen >= cap) {
                        cap *= 2;
                        data = realloc(data, cap);
                    }
                    if (!fread(&_buff1, 1, 1, f)) return false;

                    if (_buff1 == 0xff) {
                        if (!fread(&_buff2, 1, 1, f)) return false;
                        if (_buff2 != 0x00) {
                            if (_buff2 >= 0xd0 && _buff2 <= 0xd7) {  // reset marker
                                data[dataLen++] = 0xFF; // NOTE: custom special byte
                                data[dataLen++] = _buff2;
                                continue;
                            } else {
                                fseek(f, -2L, SEEK_CUR);

                                data[dataLen - 2] = 0;
                                dataLen--;

                                break;
                            }
                        }
                    }
                    data[dataLen++] = _buff1;
                }

                ImjJpegStream st = {
                    .data = data,
                    .pos = 0,
                    .len = dataLen
                };

                // decoding and upsampling
                uint8_t maxH = 1;
                uint8_t maxV = 1;
                for (uint8_t c = 0; c < frameData.nComp; c++) {
                    if (frameData.components[c].samplingFactorH > maxH)
                        maxH = frameData.components[c].samplingFactorH;
                    if (frameData.components[c].samplingFactorV > maxV)
                        maxV = frameData.components[c].samplingFactorV;
                }

                const size_t nMcuCols = (frameData.width + (8 * maxH - 1)) / (8 * maxH);
                const size_t nMcuRows = (frameData.height + (8 * maxV - 1)) / (8 * maxV);

                int oldDcCoeff[frameData.nComp];
                memset(oldDcCoeff, 0, frameData.nComp * sizeof(int));

                int (*raw)[frameData.nComp] = malloc(sizeof(int) * frameData.height * frameData.width * frameData.nComp);
                memset(raw, (byte) 100, sizeof(int) * frameData.height * frameData.width * frameData.nComp);

                for (size_t mcuRow = 0; mcuRow < nMcuRows; mcuRow++) {
                    for (size_t mcuCol = 0; mcuCol < nMcuCols; mcuCol++) {
                        for (uint8_t c = 0; c < frameData.nComp; c++) {
                            const uint8_t h = frameData.components[c].samplingFactorH;
                            const uint8_t v = frameData.components[c].samplingFactorV;
                            for (uint8_t blockRow = 0; blockRow < v; blockRow++) {
                                for (uint8_t blockCol = 0; blockCol < h; blockCol++) {
                                    int mat[8][8];

                                    oldDcCoeff[c] = imj_jpeg_buildMat(
                                        &st,
                                        &dcTables[components[c].dcHtId],
                                        &acTables[components[c].acHtId],
                                        quantTables[frameData.components[c].qtId],
                                        oldDcCoeff[c],
                                        mat
                                    );

                                    // upsampling
                                    const size_t a = 8 * maxH / h;
                                    const size_t b = 8 * maxV / v;
                                    int upsampled[b][a];

                                    imj_upsampleScalInt__(8, 8, maxH / h, maxV / v, mat, upsampled);

                                    // storing
                                    for (uint8_t yy = 0; yy < b; yy++) {
                                        for (uint8_t xx = 0; xx < a; xx++) {
                                            size_t m = (mcuCol * maxH + blockCol) * 8 + xx;
                                            size_t n = (mcuRow * maxV + blockRow) * 8 + yy;

                                            if (n < frameData.height && m < frameData.width)
                                                raw[n * frameData.width + m][c] = upsampled[yy][xx];
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // converting
                if (frameData.nComp != 3) {
                    snprintf(err, 100, "Only YCbCr images are supported.");
                    free(data);
                    if (raw) free(raw);
                    return false;
                }

                for (size_t i = 0, n1 = frameData.height; i < n1; i++) {
                    for (size_t j = 0, n2 = frameData.width; j < n2; j++) {
                        const int *rawPix = raw[i * n2 + j];
                        imgData[i * n2 + j] = imj_YCbCr2Pix__(rawPix[0], rawPix[1], rawPix[2]);
                    }
                }

                // ---
                marker = 0;
                free(data);
                if (raw) free(raw);

                // ---
                continue;
            }
            case IMJ_JPEG_MARKER_EOI: break;
            default: {
                if (marker >= 0xffe0 && marker <= 0xffef) {  // application
                    fread(&len, 2, 1, f);
                    imj_swap_uint16(&len);
                    fseek(f, len - 2, SEEK_CUR);
                    continue;
                } else {
                    if (err) snprintf(err, 100, "Unknown marker: 0x%x", marker);
                    return false;
                }
            }
        }
        break;
    }

    // ---
    img->width = frameData.width;
    img->height = frameData.height;
    img->data = imgData;

    // --- cleanup
    for (uint8_t i = 0; i < 4; i++) {
        if (dcTables[i].table) free(dcTables[i].table);
        if (acTables[i].table) free(acTables[i].table);
    }

    // ---
    return true;
}

// ---
bool imj_jpeg_read_DQT(FILE *f, uint8_t quantTables[4][64], char err[100]) {
    uint16_t len;
    fread(&len, 2, 1, f);
    imj_swap_uint16(&len);
    len -= 2;

    uint8_t header;
    for (uint8_t i = 0, n = len / 65; i < n; i++) {
        fread(&header, 1, 1, f);

        const uint8_t precision = (header >> 4) ? 16 : 8;
        const uint8_t id = header & 0xf;

        if (precision == 16) {
            snprintf(err, 100, "16-bits precision is not supported for quantization tables.");
            return false;
        }

        fread(&(quantTables[id]), 1, 64, f);
    }

    return true;
}

bool imj_jpeg_read_SOF0(FILE *f, ImjJpegFrameData *fd, char err[100]) {
    fseek(f, 2, SEEK_CUR);  // length

    fread(&fd->precision, 1, 1, f);
    fread(&fd->height, 2, 1, f);
    fread(&fd->width, 2, 1, f);
    fread(&fd->nComp, 1, 1, f);

    imj_swap_uint16(&fd->height);
    imj_swap_uint16(&fd->width);

    for (uint8_t i = 0; i < fd->nComp; i++) {
        fread(fd->components + i, 1, 3, f);
        fd->components[i].samplingFactorV = fd->components[i].samplingFactorH & 0x0f;
        fd->components[i].samplingFactorH >>= 4;
    }

    // ---
    if (fd->precision != 8) {
        snprintf(err, 100, "Only JPEGs with  8-bits precision are supported.");
        return false;
    }
    if (fd->nComp != 3) {
        snprintf(err, 100, "Only YCbCr JPEGs are supported.");
        return false;
    }

    return true;
}

bool imj_jpeg_read_DHT(FILE *f, ImjJpegHt dcTables[4], ImjJpegHt acTables[4], char err[100]) {
    uint16_t len;
    fread(&len, 2, 1, f);
    imj_swap_uint16(&len);
    len -= 2;

    uint16_t nBytesRead = 0;
    while (nBytesRead < len) {
        uint8_t htInfo;
        ImjJpegHt ht = {{}, 0, NULL};

        fread(&htInfo, 1, 1, f);
        fread(&ht.lengths, 1, 16, f);

        const uint8_t class = htInfo >> 4;
        const uint8_t id = htInfo & 0x0f;

        for (uint8_t i = 0; i < 16; i++)
            ht.size += ht.lengths[i];

        byte symbols[256];
        fread(symbols, 1, ht.size, f);

        ht.table = (ImjJpegHtEntry *) malloc(ht.size * sizeof(ImjJpegHtEntry));

        uint16_t curCode = 0;
        int j = 0;
        for (uint8_t i = 1; i <= 16; i++) {
            for (int k = 0, n = ht.lengths[i - 1]; k < n; k++) {
                if (j >= 256) {
                    snprintf(err, 100, "Huffman table exceeded increased 256.");
                    return false;
                }

                ht.table[j].code = curCode;
                ht.table[j].codeLen = i;
                ht.table[j].symbol = symbols[j];

                curCode++;
                j++;
            }
            curCode <<= 1; // why? because JPEG doesn't allow codes to be prefixed of each other
        }

        if (class) memcpy(acTables + id, &ht, sizeof(ImjJpegHt));
        else memcpy(dcTables + id, &ht, sizeof(ImjJpegHt));

        nBytesRead += 17 + ht.size;
    }

    return true;
}
