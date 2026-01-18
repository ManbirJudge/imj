#include "pnm.h"

bool imj_pnm_read(FILE *f, ImjImg *img, char err[100]) {
    if (!f || !img) {
        if (err) snprintf(err, 100, "One of the arguments is nullptr.");
        return false;
    }

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1 || magic[0] != 'P') {
        if (err) snprintf(err, 100, "Invalid PNM file.");
        return false;
    }

    int buff_1;
    while ((buff_1 = fgetc(f)) != EOF) {
        if (isspace(buff_1)) continue;
        if (buff_1 == '#') {
            while ((buff_1 = fgetc(f)) != EOF && buff_1 != '\n');
        } else {
            ungetc(buff_1, f);
            break;
        }
    }

    int w_ = 0, h_ = 0;
    if (fscanf(f, "%d %d", &w_, &h_) != 2) {
        if (err) snprintf(err, 100, "Invalid dimensions.");
        return false;
    }

    const size_t width = w_;
    const size_t height = h_;
    const size_t nPixels = width * height;
    ImjPix *data = malloc(nPixels * sizeof(ImjPix));

    switch (magic[1]) {
        case '1': {
            int ch;
            size_t i = 0;
            while (i < nPixels && (ch = fgetc(f)) != EOF) {
                if (isspace(ch)) continue;
                if (ch == '#') {
                    while ((ch = fgetc(f)) != EOF && ch != '\n') {};
                    continue;
                }

                if (ch == '0' || ch == '1') {
                    if (ch == '1') {
                        data[i] = (ImjPix){0, 0, 0, 255};
                    } else {
                        data[i] = (ImjPix){255, 255, 255, 255};
                    }
                    i++;
                }
            }

            if (i < nPixels) {
                if (err) snprintf(err, 100, "Unexpected EOF. Read %zu/%zu pixels.", i, nPixels);
                free(data);
                return false;
            }

            break;
        }
        case '2': {
            int max;
            if (fscanf(f, "%d", &max) != 1) {
                if (err) snprintf(err, 100, "FU");
                return false;
            }

            int val;
            size_t i = 0;
            if (max <= 255) {
                while (i < nPixels && fscanf(f, "%d", &val) == 1) {
                    uint8_t c = (uint8_t)(((float)val / max) * 255.0f);
                    data[i++] = (ImjPix){c, c, c, 255};
                }
            } else {
                if (err) snprintf(err, 100, "Only bit-depth of 8 is supported for PNM.");
                free(data);
                return false;
            }


            if (i < nPixels) {
                if (err) snprintf(err, 100, "Only read %llu/%llu pixels. Reading stopped unexpectedly.", i, nPixels);
                free(data);
                return false;
            }

            break;
        }
        case '3': {
            int max;
            if (fscanf(f, "%d", &max) != 1) {
                if (err) snprintf(err, 100, "FU");
                return false;
            }

            int r, g, b;
            if (max <= 255) {
                for (size_t y = 0; y < height; y++) {
                    for (size_t x = 0; x < width; x++) {
                        if (fscanf(f, "%d %d %d", &r, &g, &b) != 3) {
                            if (err) snprintf(err, 100, "Unexpected EOF.");
                            return false;
                        }
                        data[y * width + x] = (ImjPix){
                            (uint8_t)(((float)r / max) * 255.0f),
                            (uint8_t)(((float)g / max) * 255.0f),
                            (uint8_t)(((float)b / max) * 255.0f),
                            255
                        };
                    }
                }
            } else {
                if (err) snprintf(err, 100, "Only bit-depth of 8 is supported for PNM.");
                free(data);
                return false;
            }

            break;
        }
        case '4': {
            int ch;
            while ((ch = fgetc(f)) != EOF && isspace(ch)) {};
            if (ch != EOF) ungetc(ch, f);

            const size_t bytesPerRow = (width + 7) / 8;
            unsigned char *buff_2 = malloc(bytesPerRow);

            for (size_t y = 0; y < height; y++) {
                if (fread(buff_2, 1, bytesPerRow, f) != bytesPerRow) {
                    if (err) snprintf(err, 100, "Unexpected EOF.");
                    free(buff_2);
                    free(data);
                    return false;
                }
                for (size_t x = 0; x < width; x++) {
                    const int bit = (buff_2[x / 8] >> (7 - (x % 8))) & 1;
                    if (bit == 1) {
                        data[y * width + x] = (ImjPix){0, 0, 0, 255};
                    } else {
                        data[y * width + x] = (ImjPix){255, 255, 255, 255};
                    }
                }
            }

            free(buff_2);
            break;
        }
        case '5': {
            int max;
            if (fscanf(f, "%d", &max) != 1) {
                if (err) snprintf(err, 100, "FU");
                return false;
            }

            fgetc(f); // consume mandatory white space

            if (max < 256) {
                unsigned char *buff_2 = malloc(width);
                for (size_t y = 0; y < height; y++) {
                    if (fread(buff_2, 1, width, f) != width) {
                        if (err) snprintf(err, 100, "Unexpected EOF.");
                        free(buff_2);
                        return false;
                    }
                    for (size_t x = 0; x < width; x++) {
                        const uint8_t c = (uint8_t)(((float)buff_2[x] / max) * 255.0f);
                        data[y * width + x] = (ImjPix){c, c, c, 255};
                    }
                }
                free(buff_2);
            } else {
                if (err) snprintf(err, 100, "Only bit-depth of 8 is supported for PNM.");
                free(data);
                return false;
            }

            break;
        }
        case '6': {
            int max;
            if (fscanf(f, "%d", &max) != 1) {
                if (err) snprintf(err, 100, "FU");
                return false;
            }

            fgetc(f); // consume mandatory white space

            if (max < 256) {
                for (size_t y = 0; y < height; y++) {
                    for (size_t x = 0; x < width; x++) {
                        if (!fread(data + y * width + x, 1, 3, f)) {
                            if (err) snprintf(err, 100, "Unexpected EOF.");
                            free(data);
                            return false;
                        }
                        data[y * width + x].a = 255;
                    }
                }
            } else {
                if (err) snprintf(err, 100, "Bit-depth of 8 is supported for PNM.");
                free(data);
                return false;
            }

            break;
        }
        default: {
            if (err) snprintf(err, 100, "Unknown PNM magic number: P%c", magic[1]);
            free(data);
            return false;
        }
    }

    img->data = data;
    img->width = width;
    img->height = height;

    return true;
}
