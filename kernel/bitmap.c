#include "header/bitmap.h"
#include "header/debug.h"
#include "header/strings.h"

void bitmap_init(struct bitmap *bt){
    memset(bt->bits,0,bt->len);
}

/**
 * 判断第bit_idx位是否为空闲
 */
bool bitmap_scan_test(struct bitmap *bt, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_idx_in_byte = bit_idx % 8;
    return (bt->bits[byte_idx] && (_BITMAP_MASK_ << bit_idx_in_byte)) != 0;
}


/**
 * 找到连续的N个空闲位，返回起始位，失败返回-1
 */
int bitmap_scan(struct bitmap *bt, uint32_t cnt)
{
    uint32_t idx_byte = 0;
    while (0xFF == bt->bits[idx_byte] && idx_byte < bt->len)
    {
        idx_byte++;
    }
    ASSERT(idx_byte < bt->len);
    if (idx_byte == bt->len)
    {
        return -1;
    }
    uint32_t idx_bit = 0;
    while ((uint8_t)(_BITMAP_MASK_ << idx_bit) & bt->bits[idx_byte])
    {
        idx_bit++;
    }
    int bit_idx_start = idx_byte * 8 + idx_bit;
    if (cnt == 1)
    {
        return bit_idx_start;
    }
    uint32_t bit_left = bt->len * 8 - bit_idx_start;
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;
    bit_idx_start = -1;
    while (bit_left-- > 0)
    {
        if (!bitmap_scan_test(bt,next_bit))
        {
            count++;
        }
        else
        {
            count = 0;
        }
        if (count == cnt)
        {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

/**
 * 设置位图第bit_idx位的状态
 */
void bitmap_set(struct bitmap *bt, uint32_t bit_idx, int8_t val)
{
    ASSERT(val == 0 || val == 1);
    uint32_t idx_byte = bit_idx / 8;
    uint32_t idx_bit = bit_idx % 8;
    if (val)
    {
        bt->bits[idx_byte] |= _BITMAP_MASK_ << idx_bit;
    }
    else
    {
        bt->bits[idx_byte] &= ~(_BITMAP_MASK_ << idx_bit);
    }
}