#ifndef __LIB_STRINGS_H
#define __LIB_STRINGS_H
#include "stdint.h"
int64_t strlen(uint8_t *str);
int8_t putc(uint8_t chr);
void puts(const char *str);
int8_t strcmp(uint8_t *str_a,uint8_t *str_b);
void print_num32(uint32_t num);
void print_num32_hex(uint32_t num);
#endif