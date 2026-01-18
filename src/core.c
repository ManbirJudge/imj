#include "core.h"

void imj_swap_uint16(uint16_t *val) {
    const uint16_t v = *val;
    *val = ((v >> 8) & 0x00FF) |
           ((v << 8) & 0xFF00);
}

void imj_swap_uint32(uint32_t *val) {
    const uint32_t v = *val;
    *val = ((v >> 24) & 0x000000FF) |
           ((v >> 8) & 0x0000FF00) |
           ((v << 8) & 0x00FF0000) |
           ((v << 24) & 0xFF000000);
}