#include "header/strings.h"
#include "header/stdint.h"
#include "header/print.h"
#include "header/debug.h"

static uint8_t hex[] = "0123456789ABCDEF";

uint32_t strlen(uint8_t *str)
{
    if (str == NULL)
    {
        return -1;
    }
    uint32_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len + 1;
}

char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    while (*str != '\0')
    {
        if (*str == ch)
        {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

int8_t putc(uint8_t chr)
{
    put_char(chr);
    return chr;
}

void puts(const char *str)
{
    if (str == NULL)
    {
        return;
    }
    int64_t len = 0;
    while (str[len] != '\0')
    {
        putc(str[len]);
        len++;
    }
    return;
}

uint8_t strcmp(uint8_t *str_a, uint8_t *str_b)
{
    int i = 0;
    while (str_a[i] != '\0' && str_b[i] != '\0')
    {
        if (str_a[i] < str_b[i])
        {
            return -1;
        }
        else if (str_a[i] > str_b[i])
        {
            return 1;
        }
        i++;
    }
    if (str_a[i] == '\0' && str_b[i] == '\0')
    {
        return 0;
    }
    else if (str_a[i] == '\0')
    {
        return -1;
    }
    else
    {
        return 1;
    }
}
char *strcpy(uint8_t *dest, uint8_t *src)
{
    if (dest == NULL || src == NULL)
    {
        return -1;
    }
    int64_t i = 0;
    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return i + 1;
}

void print_num32(uint32_t num)
{
    if (num == 0)
    {
        putc('0');
        return;
    }
    print_num32(num / 10);
    putc(num % 10 + '0');
    return;
}

void print_num32_hex(uint32_t num)
{

    if (num == 0)
    {
        putc('0');
        return;
    }
    print_num32_hex(num >> 4);
    putc(hex[num & 0xF]);
    return;
}

void menset(void *dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = value;
    }
}

void memcpy(void *dst_, const void *src_, uint32_t size)
{
    ASSERT(dst_ != NULL);
    ASSERT(src_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    uint8_t *src = (uint8_t *)src_;
    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}
int memcmp(void *a_, const void *b_, uint32_t size)
{
    ASSERT(a_ != NULL);
    ASSERT(b_ != NULL);
    uint8_t *a = (uint8_t *)a_;
    uint8_t *b = (uint8_t *)b_;
    for (uint32_t i = 0; i < size; i++)
    {
        if (a[i] != b[i])
        {
            return -1;
        }
    }
    return 0;
}