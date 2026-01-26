#include "qoi.h"

bool imj_qoi_read(FILE *f, ImjImg *img, char err[100]) {
    if (!f || !img) {
        if (err) snprintf(err, 100, "One of the arguments is nullptr.");
        return false;
    }

    // header
    const char IMJ_IMJ_QOI_MAGIC[] = {'q', 'o', 'i', 'f'};
    const byte IMJ_IMJ_QOI_EOF[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    ImjQoiHeader qoiHeader;
    fread(&qoiHeader.magic, 1, 4, f);
    if (strncmp(qoiHeader.magic, IMJ_IMJ_QOI_MAGIC, 4) != 0) {
        if (err) snprintf(err, 100, "Invalid IMJ_QOI_ magic.");
        return false;
    }

    fread(&qoiHeader.width, 4, 1, f);
    fread(&qoiHeader.height, 4, 1, f);
    imj_swap_uint32(&qoiHeader.width);
    imj_swap_uint32(&qoiHeader.height);
    fread(&qoiHeader.channels, 1, 1, f);
    fread(&qoiHeader.colorSpace, 1, 1, f);

    if (!(qoiHeader.channels == 3 || qoiHeader.channels == 4)) {
        if (err) snprintf(err, 100, "Invalid number of channels: %i", qoiHeader.channels);
        return false;
    }
    #ifdef IMJ_STRICT
        if (!(qoiHeader.colorSpace == 0 || qoiHeader.colorSpace == 1)) {
            if (err) snprintf(err, 100, "Invalid colorspace: %i", qoiHeader.colorSpace);
            return false;
        }
    #endif

    // initialization
    const size_t nPixels = qoiHeader.width * qoiHeader.height;
    ImjPix *data = malloc(nPixels * sizeof(ImjPix));

    ImjPix prevPixels[64] = {{0, 0, 0, 0}};
    ImjPix lastPix = {0, 0, 0, 255};
    size_t pixI = 0;

    // chuck read loop
    byte buff_1;

    while (pixI < nPixels) {
        fread(&buff_1, 1, 1, f);
        ImjPix pix;

        switch (buff_1) {
            case IMJ_QOI_OP_RGB: {
                fread(&pix, 1, 3, f);
                pix.a = lastPix.a;
                break;
            }
            case IMJ_QOI_OP_RGBA: {
                fread(&pix, 1, 4, f);
                break;
            }
            default: {
                switch (buff_1 & 0xC0) {
                    case IMJ_QOI_OP_INDEX: {
                        pix = prevPixels[buff_1 & 0b00111111];
                        break;
                    }
                    case IMJ_QOI_OP_DIFF: {
                        const int dr = ((buff_1 & 0b00110000) >> 4) - 2;
                        const int dg = ((buff_1 & 0b00001100) >> 2) - 2;
                        const int db = (buff_1 & 0b00000011) - 2;

                        pix.r = lastPix.r + dr;
                        pix.g = lastPix.g + dg;
                        pix.b = lastPix.b + db;
                        pix.a = lastPix.a;

                        break;
                    }
                    case IMJ_QOI_OP_LUMA: {
                        const int dg = (buff_1 & 0b00111111) - 32;
                        fread(&buff_1, 1, 1, f);
                        const int dr_dg = ((buff_1 & 0xf0) >> 4) - 8;
                        const int db_dg = (buff_1 & 0x0f) - 8;

                        const int dr = dg + dr_dg;
                        const int db = dg + db_dg;

                        pix.r = lastPix.r + dr;
                        pix.g = lastPix.g + dg;
                        pix.b = lastPix.b + db;
                        pix.a = lastPix.a;

                        break;
                    }
                    case IMJ_QOI_OP_RUN: {
                        uint8_t runLen = (buff_1 & 0x3F) + 1;
                        while (runLen-- && pixI < nPixels) {
                            data[pixI++] = lastPix;
                        }
                        continue;
                    }
                    default: {
                        if (err) snprintf(err, 100, "Invalid IMJ_QOI_ chunk: %x", buff_1);
                        free(data);
                        return false;
                    }
                }
                break;
            }
        }

        data[pixI++] = pix;
        prevPixels[IMJ_QOI_PIX_HASH(pix)] = pix;
        lastPix = pix;

        if (feof(f)) {
            snprintf(err, 100, "Unexpected EOF at pixel %zu.", pixI);
            return false;
        }
    }

    // eof check
    byte buff_2[8];
    fread(&buff_2, 1, 8, f);

    if (memcmp(IMJ_IMJ_QOI_EOF, buff_2, 8) != 0) {
        if (err) snprintf(err, 100, "Invalid IMJ_QOI_ EOF.");
        free(data);
        return false;
    }

    // ---
    img->width = qoiHeader.width;
    img->height = qoiHeader.height;
    img->data = data;

    return true;
}