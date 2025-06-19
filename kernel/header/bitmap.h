#ifndef _LIB_KERNEL_BITMAP_H
#define _LIB_KERNEL_BITMAP_H
#define _BITMAP_MASK_ 1
#include "stdint.h"
struct bitmap
{
    uint32_t len;
    uint8_t *bits;
};
void bitmap_init(struct bitmap *bt);
bool scan_test(struct bitmap *bt, uint32_t bit_idx);
int scan(struct bitmap *bt, uint32_t cnt);
void set(struct bitmap *bt, uint32_t bit_idx, int8_t value);
#endif // _LIB_KERNEL_BITMAP_H