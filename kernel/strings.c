#include "header/strings.h"
#include "header/stdint.h"
#include "header/print.h"

static uint8_t hex[]= "0123456789ABCDEF";

int64_t strlen(uint8_t *str)
{
    if (str == NULL)
    {
        return -1;
    }
    int64_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len + 1;
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
        return ;
    }
    int64_t len = 0;
    while (str[len] != '\0')
    {
        putc(str[len]);
        len++;
    }
    return;
}

int8_t strcmp(uint8_t *str_a, uint8_t *str_b)
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
int64_t strcpy(uint8_t *dest, uint8_t *src)
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
    if( num == 0)
    {
        putc('0');
        return;
    }
    print_num32(num/10);
    putc(num % 10 + '0');
    return;
}

void print_num32_hex(uint32_t num){
    
    if( num == 0)
    {
        putc('0');
        return;
    }
    print_num32_hex(num>>4);
    putc(hex[num & 0xF]);
    return;
}