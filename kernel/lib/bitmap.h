#ifndef __LIB_KERNRL_BITMAP_H
#define __LIB_KERNRL_BITMAP_H
#include "../header/stdint.h"
#define BITMAP_MASK 1
class bitmap
{
    uint32_t btmp_bytes_len;
    uint8_t *bits;

public:
    bitmap();
    bool scan_test(uint32_t bit_idx);
    int scan(uint32_t cnt);
    void set(uint32_t bit_idx, int8_t value);
};
#endif /* __LIB_KERNRL_BITMAP_H */