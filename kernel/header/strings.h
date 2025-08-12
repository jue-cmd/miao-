#ifndef __LIB_STRINGS_H
#define __LIB_STRINGS_H
#include "stdint.h"
uint32_t strlen(uint8_t *str);
int8_t putc(uint8_t chr);
void puts(const char *str);
int8_t strcmp(uint8_t *str_a,uint8_t *str_b);
void print_num32(uint32_t num);
void print_num32_hex(uint32_t num);
void memset(void *dst_, uint8_t value, uint32_t size);
void memcpy(void *dst_, const void *src_, uint32_t size);
int memcmp(void *a_, const void *b_, uint32_t size);
uint32_t strcpy(uint8_t *dest, uint8_t *src);
#endif