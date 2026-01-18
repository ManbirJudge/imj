#include <stdio.h>

#include "imj.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("File path is required.");
        return 1;
    }

    char *path = argv[1];
    ImjImg img;
    char err[100];

    const clock_t begin = clock();
    const bool res = imj_img_from_file(path, &img, err);
    const clock_t end = clock();

    if (res) {
        const double duration = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
        printf("Read in %.0f ms.\n", duration);

        FILE *f = fopen("test.ppm", "wb");
        if (f) {
            fprintf(f, "P7\nWIDTH %llu\nHEIGHT %llu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", img.width, img.height);
            for (size_t i = 0; i < img.width * img.height; i++) {
                fputc(img.data[i].r, f);
                fputc(img.data[i].g, f);
                fputc(img.data[i].b, f);
                fputc(img.data[i].a, f);
            };
            fclose(f);
        } else {
            printf("Error while writing PPM file.\n");
        }

        imj_img_free(&img);
    } else {
        printf("Error:\n%s\n", err);
    }

    return 0;
}