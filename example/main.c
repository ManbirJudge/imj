#include <time.h>

#include "core.h"
#include "logging.h"
#include "imj.h"

int main(void)
{
    char *path = "C:\\Users\\manbi\\Downloads\\profile.jpg";
    Img img = {};
    char err[100];

    clock_t start = clock();
    bool res = imj_img_from_file(path, &img, err);
    clock_t end = clock();

    DBG("Done in %.2f ms.", (double)(end - start) / CLOCKS_PER_SEC * 1000);

    if (res)
        SUC("Success.");
    else
        ERR("Error: %.*s", 100, err);

    imj_img_free(&img);
    return (int)res;
}