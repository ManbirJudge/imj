#include "jpeg.h"
#include "utils.h"

// ---
uint16_t jpeg_stream_read(jpeg_Stream *s, size_t n)
{
    uint32_t res = 0;

    for (uint8_t i = 0; i < n; i++)
    {
        if (s->pos >= s->len * 8)
            return res;

        byte b = s->data[s->pos >> 3];
        uint8_t shift = 7 - (s->pos & 0x7);

        res = (res << 1) | ((b >> shift) & 1);
        s->pos++;
    }

    return res;
}

void jpeg_stream_align(jpeg_Stream *s)
{
    s->pos = (s->pos + 7) & ~7;
}

// ---
uint16_t jpeg_HT_get_code(jpeg_HT *ht, jpeg_Stream *s)
{
    uint16_t code = 0;

    for (int x = 1; x <= 16; x++)
    {
        uint16_t bit = jpeg_stream_read(s, 1);
        code = (code << 1) | bit;

        for (int y = 0; y < ht->size; y++)
        {
            const jpeg_HT_Entry z = ht->table[y];

            if (z.codeLen == x && z.code == code)
            {
                // VER("<");
                // printb_uint16(code);
                // VER(">");

                return z.symbol;
            }
        }
    }

    return -1;
}

int jpeg_decode_num(uint16_t bits, uint8_t len)
{
    if (bits >= (1 << (len - 1)))
        return bits;
    return bits - ((1 << len) - 1);
}

int jpeg_build_mat(jpeg_Stream *s, jpeg_HT *dcHt, jpeg_HT *acHt, uint8_t qt[64], int oldDcCoeff, int mat[8][8])
{
    // setup
    static const uint8_t precision = 8;
    static const float idctTable[8][8] = {
        {0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475, 0.7071067811865475},
        {0.9807852804032304, 0.8314696123025452, 0.5555702330196023, 0.19509032201612833, -0.1950903220161282, -0.555570233019602, -0.8314696123025453, -0.9807852804032304},
        {0.9238795325112867, 0.38268343236508984, -0.3826834323650897, -0.9238795325112867, -0.9238795325112868, -0.38268343236509034, 0.38268343236509, 0.9238795325112865},
        {0.8314696123025452, -0.1950903220161282, -0.9807852804032304, -0.5555702330196022, 0.5555702330196018, 0.9807852804032304, 0.19509032201612878, -0.8314696123025451},
        {0.7071067811865476, -0.7071067811865475, -0.7071067811865477, 0.7071067811865474, 0.7071067811865477, -0.7071067811865467, -0.7071067811865471, 0.7071067811865466},
        {0.5555702330196023, -0.9807852804032304, 0.1950903220161283, 0.8314696123025456, -0.8314696123025451, -0.19509032201612803, 0.9807852804032307, -0.5555702330196015},
        {0.38268343236508984, -0.9238795325112868, 0.9238795325112865, -0.3826834323650899, -0.38268343236509056, 0.9238795325112867, -0.9238795325112864, 0.38268343236508956},
        {0.19509032201612833, -0.5555702330196022, 0.8314696123025456, -0.9807852804032307, 0.9807852804032305, -0.831469612302545, 0.5555702330196015, -0.19509032201612858},
    };
    static const uint8_t zigzag[64] = {
        0, 1, 5, 6, 14, 15, 27, 28,
        2, 4, 7, 13, 16, 26, 29, 42,
        3, 8, 12, 17, 25, 30, 41, 43,
        9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63};

    // reading
    int _buffMat1[64] = {0}; // init reading
    int _buffMat2[8][8];     // zigzag

    uint8_t _htCode = jpeg_HT_get_code(dcHt, s);
    uint16_t _bits;
    int _dcCoeff;
    if (_htCode != 0)
    {
        _bits = jpeg_stream_read(s, _htCode);
        _dcCoeff = jpeg_decode_num(_bits, _htCode) + oldDcCoeff;
    }
    else
    {
        _bits = 0;
        _dcCoeff = oldDcCoeff;
    }
    // printb_uint16(_bits);
    // VER(" %i (%i), %i\n", _dcCoeff, oldDcCoeff, _htCode);

    _buffMat1[0] = _dcCoeff * qt[0];

    int l = 1;
    while (l < 64)
    {
        _htCode = jpeg_HT_get_code(acHt, s);
        if (_htCode == 0)
            break;

        uint8_t run = _htCode >> 4;
        uint8_t size = _htCode & 0x0F;
        l += run;

        if (size > 0 && l < 64)
        {
            _bits = jpeg_stream_read(s, size);
            _buffMat1[l] = jpeg_decode_num(_bits, size) * qt[l];
            l++;
        }
    }

    // zigzag
    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            _buffMat2[i][j] = _buffMat1[zigzag[i * 8 + j]];
        }
    }

    // inverse DCT
    for (uint8_t x = 0; x < 8; x++)
    {
        for (uint8_t y = 0; y < 8; y++)
        {
            float S = 0;

            for (uint8_t u = 0; u < precision; u++)
            {
                for (uint8_t v = 0; v < precision; v++)
                {
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
IMJ bool jpeg_read(FILE *f, Img *img, char err[100])
{
    // ---
    jpeg_HT dcTables[4] = {
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL}};
    jpeg_HT acTables[4] = {
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL},
        {{}, 0, NULL}};
    uint8_t quantTables[4][64];
    jpeg_frame_data frameData = {};
    uint16_t resetInterval = 0;

    Pix *imgData = NULL;

    // ---
    uint16_t marker;
    uint16_t len;

    while (fread(&marker, 1, 2, f))
    {
        swap_uint16(&marker);
        INF("Marker: %4x | %s", marker, jpeg_get_marker_name(marker));

        switch (marker)
        {
        case SOI:
            continue;
        case DQT:
        {
            if (!jpeg_read_DQT(f, quantTables, err))
                return false;
            continue;
        }
        case SOF0:
        {
            if (!jpeg_read_SOF0(f, &frameData, err))
                return false;

            imgData = (Pix *)malloc(frameData.width * frameData.height * sizeof(Pix));
            continue;
        }
        case SOF2:
        {
            if (!jpeg_read_SOF0(f, &frameData, err))  // TODO: implement seperate func
                return false;

            imgData = (Pix *)malloc(frameData.width * frameData.height * sizeof(Pix));
            continue;
        }
        case DRI:
        {
            fseek(f, 2, SEEK_CUR);
            fread(&resetInterval, 2, 1, f);
            swap_uint16(&resetInterval);
            continue;
        }
        case DHT:
        {
            if (!jpeg_read_DHT(f, dcTables, acTables, err))
                return false;
            continue;
        }
        case SOS:
        {
            fread(&len, 2, 1, f);
            swap_uint16(&len);
            len -= 2;

            // ---
            fseek(f, 1, SEEK_CUR);

            jpeg_SOS_comp components[frameData.nComp];

            for (uint8_t i = 0; i < frameData.nComp; i++)
            {
                fread(&(components[i].id), 1, 1, f);
                fread(&(components[i].dcHtId), 1, 1, f);

                components[i].acHtId = components[i].dcHtId & 0x0f;
                components[i].dcHtId >>= 4;

                VER("%i | %i | %i\n", components[i].id, components[i].dcHtId, components[i].acHtId);
            }

            fseek(f, 3, SEEK_CUR); // TODO: for progressive JPEGs

            DBG("Reset interval: %i", resetInterval);

            // 0xff00 -> 0xff
            byte _buff1;
            byte _buff2;

            uint32_t cap = 1024;
            byte *data = (byte *)malloc(cap);
            uint32_t dataLen = 0;

            while (true)
            {
                if (dataLen >= cap)
                {
                    cap *= 2;
                    data = realloc(data, cap);
                }

                if (!fread(&_buff1, 1, 1, f))
                    return false;

                if (_buff1 == 0xff)
                {
                    if (!fread(&_buff2, 1, 1, f))
                        return false;

                    if (_buff2 != 0x00)
                    {
                        if (
                            _buff2 == 0xd0 ||
                            _buff2 == 0xd1 ||
                            _buff2 == 0xd2 ||
                            _buff2 == 0xd3 ||
                            _buff2 == 0xd4 ||
                            _buff2 == 0xd5 ||
                            _buff2 == 0xd6 ||
                            _buff2 == 0xd7)
                        {
                            continue;
                        }
                        else
                        {
                            fseek(f, -2L, SEEK_CUR);

                            data[dataLen - 2] = 0;
                            dataLen--;

                            break;
                        }
                    }
                }

                data[dataLen++] = _buff1;
            }

            DBG("Actual data length: %i", dataLen);
            writeb_path("./x1.bin", data, dataLen);

            jpeg_Stream st = {
                .data = data,
                .pos = 0,
                .len = dataLen};

            // decoding (& upsampling)
            uint8_t maxH = 1;
            uint8_t maxV = 1;
            for (uint8_t c = 0; c < frameData.nComp; c++)
            {
                if (frameData.components[c].samplingFactorH > maxH)
                    maxH = frameData.components[c].samplingFactorH;
                if (frameData.components[c].samplingFactorV > maxV)
                    maxV = frameData.components[c].samplingFactorV;
            }

            size_t mcuCols = (frameData.width + (8 * maxH - 1)) / (8 * maxH);
            size_t mcuRows = (frameData.height + (8 * maxV - 1)) / (8 * maxV);

            size_t mcuCount = 0;

            int oldDcCoeff[frameData.nComp];
            memset(oldDcCoeff, 0, frameData.nComp * sizeof(int));

            int (*raw)[frameData.nComp] = malloc(sizeof(int) * frameData.height * frameData.width * frameData.nComp);
            memset(raw, (byte)100, sizeof(int) * frameData.height * frameData.width * frameData.nComp);

            for (size_t mcuRow = 0; mcuRow < mcuRows; mcuRow++)
            {
                for (size_t mcuCol = 0; mcuCol < mcuCols; mcuCol++)
                {
                    for (uint8_t c = 0; c < frameData.nComp; c++)
                    {
                        uint8_t h = frameData.components[c].samplingFactorH;
                        uint8_t v = frameData.components[c].samplingFactorV;

                        for (uint8_t blockRow = 0; blockRow < v; blockRow++)
                        {
                            for (uint8_t blockCol = 0; blockCol < h; blockCol++)
                            {
                                int mat[8][8];

                                oldDcCoeff[c] = jpeg_build_mat(
                                    &st,
                                    &dcTables[components[c].dcHtId],
                                    &acTables[components[c].acHtId],
                                    quantTables[frameData.components[c].qtId],
                                    oldDcCoeff[c],
                                    mat);

                                // upsampling
                                size_t a = 8 * maxH / h;
                                size_t b = 8 * maxV / v;
                                int upsampled[b][a];

                                upsample_scale_int(8, 8, maxH / h, maxV / v, mat, upsampled);

                                // storing
                                for (uint8_t yy = 0; yy < b; yy++)
                                {
                                    for (uint8_t xx = 0; xx < a; xx++)
                                    {
                                        size_t m = (mcuCol * maxH + blockCol) * 8 + xx;
                                        size_t n = (mcuRow * maxV + blockRow) * 8 + yy;

                                        if (n < frameData.height && m < frameData.width)
                                            raw[n * frameData.width + m][c] = upsampled[yy][xx];
                                    }
                                }
                            }
                        }
                    }

                    mcuCount++;
                    if (resetInterval > 0)
                    {
                        if (mcuCount % resetInterval == 0)
                        {
                            DBG("RESET --------------------");
                            memset(oldDcCoeff, 0, frameData.nComp * sizeof(int));
                            jpeg_stream_align(&st);
                        }
                    }
                }
            }

            // converting
            if (frameData.nComp != 3)
            {
                snprintf(err, 100, "Only YCbCr images are supported.");
                return false;
            }

            for (size_t i = 0, n1 = frameData.height; i < n1; i++)
            {
                for (size_t j = 0, n2 = frameData.width; j < n2; j++)
                {
                    int *rawPix = raw[i * n2 + j];
                    imgData[i * n2 + j] = YCbCr_to_pix(rawPix[0], rawPix[1], rawPix[2]);
                    // imgData[i * n2 + j] = YCbCr_to_pix(rawPix[0], 0, 0);
                }
            }

            // ---
            marker = 0;
            if (data)
                free(data);
            if (raw)
                free(raw);

            continue;

            // ---
            continue;
        }
        case EOI:
            break;
        default:
        {
            if (marker == APP0 || marker == APP1 || marker == APP2 || marker == APP13)
            {
                fread(&len, 2, 1, f);
                swap_uint16(&len);

                fseek(f, len - 2, SEEK_CUR);
                continue;
            }
            else
            {
                snprintf(err, 100, "Unknown marker.");
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
    for (uint8_t i = 0; i < 4; i++)
    {
        if (dcTables[i].table)
            free(dcTables[i].table);
        if (acTables[i].table)
            free(acTables[i].table);
    }

    INF("Cleanup done.");

    // ---
    return true;
}

// ---
bool jpeg_read_DQT(FILE *f, uint8_t quantTables[4][64], char err[100])
{
    uint16_t len;
    fread(&len, 2, 1, f);
    swap_uint16(&len);
    len -= 2;

    uint8_t header;
    for (uint8_t i = 0, nQT = len / 65; i < nQT; i++)
    {
        fread(&header, 1, 1, f);

        uint8_t precision = (header >> 4) ? 16 : 8;
        uint8_t id = header & 0xf;

        if (precision == 16)
        {
            snprintf(err, 100, "Precision of 16-bits is unsupported for quantization tables.");
            return false;
        }

        fread(&(quantTables[id]), 1, 64, f);

        // debugging
        DBG("ID: %x", id);
        DBG("Table:");
        for (uint8_t j = 0; j < 64; j++)
        {
            VER("%3i ", quantTables[id][j]);
            if (j % 8 == 7)
                VER("\n");
        }
    }

    // ---
    return true;
}

bool jpeg_read_SOF0(FILE *f, jpeg_frame_data *frameData, char err[100])
{
    uint16_t len;
    fread(&len, 2, 1, f);
    swap_uint16(&len);

    fread(&frameData->precision, 1, 1, f);
    fread(&frameData->height, 2, 1, f);
    fread(&frameData->width, 2, 1, f);
    fread(&frameData->nComp, 1, 1, f);

    swap_uint16(&frameData->height);
    swap_uint16(&frameData->width);

    for (uint8_t i = 0; i < frameData->nComp; i++)
    {
        fread(frameData->components + i, 1, 3, f);
        frameData->components[i].samplingFactorV = frameData->components[i].samplingFactorH & 0x0f;
        frameData->components[i].samplingFactorH >>= 4;
    }

    // debugging
    DBG("Size: %ix%i", frameData->width, frameData->height);
    DBG("Precision: %i", frameData->precision);
    DBG("Components (%i):", frameData->nComp);
    for (uint8_t i = 0; i < frameData->nComp; i++)
    {
        const jpeg_frame_comp x = frameData->components[i];
        VER("ID: %i | Sampling factors: %i, %i | QT ID: %i\n", x.id, x.samplingFactorH, x.samplingFactorV, x.qtId);
    }

    // ---
    if (frameData->precision != 8)
    {
        snprintf(err, 100, "Only JPEGs with precision of 8-bits are supported.");
        return false;
    }
    if (frameData->nComp != 3)
    {
        snprintf(err, 100, "Only YCbCr JPEGs are supported.");
        return false;
    }

    return true;
}

bool jpeg_read_DHT(FILE *f, jpeg_HT dcTables[4], jpeg_HT acTables[4], char err[100])
{
    uint16_t len;
    fread(&len, 2, 1, f);
    swap_uint16(&len);
    len -= 2;

    uint16_t bytesRead = 0;

    while (bytesRead < len)
    {
        uint8_t htInfo;
        jpeg_HT ht = {{}, 0, NULL};

        fread(&htInfo, 1, 1, f);
        fread(&ht.lengths, 1, 16, f);

        uint8_t class = htInfo >> 4;
        uint8_t id = htInfo & 0x0F;

        for (uint8_t i = 0; i < 16; i++)
            ht.size += ht.lengths[i];

        byte symbols[256];
        fread(symbols, 1, ht.size, f);

        ht.table = (jpeg_HT_Entry *)malloc(ht.size * sizeof(jpeg_HT_Entry));

        uint16_t curCode = 0;
        int j = 0;
        for (uint8_t i = 1; i <= 16; i++)
        {
            for (int k = 0, n = ht.lengths[i - 1]; k < n; k++)
            {
                if (j >= 256)
                {
                    snprintf(err, 100, "Huffman table sized increased 256.");
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

        if (class)
            memcpy(acTables + id, &ht, sizeof(jpeg_HT));
        else
            memcpy(dcTables + id, &ht, sizeof(jpeg_HT));

        bytesRead += 17 + ht.size;

        // debugging
        DBG("Class: %s", class ? "AC" : "DC");
        DBG("ID: %i", id);
        DBG("Lengths: ");
        for (size_t i = 0; i < 16; i++)
            VER("%i ", ht.lengths[i]);
        VER("\n");
        if (ht.size <= 16)
        {
            DBG("Table (%i): ", ht.size);
            for (size_t i = 0; i < ht.size; i++)
            {
                printb_uint16(ht.table[i].code);
                VER(": %x\n", ht.table[i].symbol);
            }
        }
        DBG("--------------------------");
    }

    // ---
    return true;
}

// ---
IMJ char *jpeg_get_marker_name(uint16_t marker)
{
    switch (marker)
    {
    case SOF0:
        return "SOF 0 - Baseline DCT";
    case SOF1:
        return "SOF 1 - Extended Sequential DCT";
    case SOF2:
        return "SOF 2 - Progressive DCT";
    case SOF3:
        return "SOF 3 - Lossless";
    case SOF5:
        return "SOF 5 - Differential Sequential DCT";
    case SOF6:
        return "SOF 6 - Differential Progressive DCT";
    case SOF7:
        return "SOF 7 - Differential Lossless DCT";
    case SOF9:
        return "SOF 9 - Extended Sequential DCT (with Arithematic coding)";
    case SOF10:
        return "SOF 10 - Progressive DCT (with Arithematic coding)";
    case SOF11:
        return "SOF 11 - Lossless (with Arithematic coding)";
    case SOF13:
        return "SOF 13 - Differential Sequential DCT (with Arithematic coding)";
    case SOF14:
        return "SOF 14 - Differential Progressive DCT (with Arithematic coding)";
    case SOF15:
        return "SOF 15 - Differential Lossless DCT (with Arithematic coding)";
    case SOI:
        return "Start of Image";
    case APP0:
        return "JFIF";
    case APP1:
        return "Exif";
    case APP2:
        return "ICC Color Profile";
    case DQT:
        return "Quantization Table";
    case DHT:
        return "Define Huffman Table";
    case SOS:
        return "Start of Scan";
    case EOI:
        return "End of Image";
    default:
        return "<unkown>";
    }
}
