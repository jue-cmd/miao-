#include "header/print.h"
#include "header/strings.h"
#include "header/init.h"

void main()
{   
    init_all();

    asm volatile("sti");

    while (1);
}