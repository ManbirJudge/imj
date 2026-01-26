// #include <stdio.h>
//
// #include "imj.h"
//
// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         printf("File path is required.");
//         return 1;
//     }
//
//     char *path = argv[1];
//     ImjImg img;
//     char err[100];
//
//     const clock_t begin = clock();
//     const bool res = imj_img_from_file(path, &img, err);
//     const clock_t end = clock();
//
//     const double duration = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
//     printf("Done in %.0f ms.\n", duration);
//
//     if (res) {
//         FILE *f = fopen("imj-example-out.pam", "wb");
//         if (f) {
//             fprintf(f, "P7\nWIDTH %llu\nHEIGHT %llu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", img.width, img.height);
//             for (size_t i = 0; i < img.width * img.height; i++) {
//                 fputc(img.data[i].r, f);
//                 fputc(img.data[i].g, f);
//                 fputc(img.data[i].b, f);
//                 fputc(img.data[i].a, f);
//             }
//             fclose(f);
//         } else {
//             printf("Error while writing PAM file.\n");
//         }
//
//         imj_img_free(&img);
//     } else {
//         printf("Error:\n\t%s\n", err);
//     }
//
//     return 0;
// }
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#include "imj.h"

bool processFile(const char *dirPath, const char *fileName) {
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, fileName);

    ImjImg img;
    char err[100];

    printf("--- Processing: %s\n", fileName);

    const clock_t begin = clock();
    const bool res = imj_img_from_file(fullPath, &img, err);
    const clock_t end = clock();

    if (!res) {
        printf("Error: %s\n", err);
        printf("\n");
        return false;
    }

    const double dur = (double)(end - begin) / CLOCKS_PER_SEC * 1000;
    printf("Done in %.0f ms.\n", dur);

    char outName[512];
    snprintf(outName, sizeof(outName), "out_%s.pam", fileName);

    FILE *f = fopen(outName, "wb");
    if (f) {
        fprintf(f, "P7\nWIDTH %llu\nHEIGHT %llu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", img.width, img.height);
        for (size_t i = 0; i < img.width * img.height; i++) {
            fputc(img.data[i].r, f);
            fputc(img.data[i].g, f);
            fputc(img.data[i].b, f);
            fputc(img.data[i].a, f);
        }
        fclose(f);
    }
    imj_img_free(&img);

    printf("\n");
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    char *dirPath = argv[1];
    struct dirent *entry;
    DIR *d = opendir(dirPath);

    if (d == NULL) {
        perror("opendir");
        return 1;
    }

    int nSuccess = 0;
    int nTotal = 0;

    while ((entry = readdir(d))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        nTotal++;
        if (processFile(dirPath, entry->d_name)) nSuccess++;
    }

    printf("Parsed %i out of %i files.", nSuccess, nTotal);

    closedir(d);
    return 0;
}