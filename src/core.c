#include "core.h"

// ---
IMJ void print_bytes(byte bytes[], size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        printf("%x ", bytes[i]);
    }
}

IMJ void print_chars(byte chars[], size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        printf("%c", chars[i]);
    }
}

void printb_uint16(uint16_t val)
{
    for (int i = 15; i >= 0; i--)
    {
        putchar((val & (1 << i)) ? '1' : '0');
    }
}

// ---
void swap_uint16(uint16_t *val)
{
    uint16_t v = *val;
    *val = ((v >> 8) & 0x00FF) |
           ((v << 8) & 0xFF00);
}

void swap_uint32(uint32_t *val)
{
    uint32_t v = *val;
    *val = ((v >> 24) & 0x000000FF) |
           ((v >> 8) & 0x0000FF00) |
           ((v << 8) & 0x00FF0000) |
           ((v << 24) & 0xFF000000);
}

// ---
static inline uint8_t clamp(int val)
{
    if (val < 0)
        return 0;
    if (val > 255)
        return 255;
    return (uint8_t)val;
}

Pix YCbCr_to_pix(int Y, int Cb, int Cr)
{
    double R = Cr * (2.0 - 2.0 * 0.299) + Y;
    double B = Cb * (2.0 - 2.0 * 0.114) + Y;
    double G = (Y - 0.114 * B - 0.299 * R) / 0.587;

    Pix p;
    p.r = clamp((int)(R + 128));
    p.g = clamp((int)(G + 128));
    p.b = clamp((int)(B + 128));
    return p;
}

// ---
IMJ void imj_img_free(Img *img)
{
    if (img->data)
        free(img->data);
}