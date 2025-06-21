#include "header/print.h"
#include "header/strings.h"
#include "header/init.h"
#include "header/debug.h"
#include "header/memory.h"

void main()
{
    init_all();
    void *addr = get_kernel_pages(1);

    puts("\n get_kernel_page start vaddr is ");
    print_num32_hex((uint32_t)addr);

    puts("\n");

    while (1)
        ;
}