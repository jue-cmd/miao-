#include "header/debug.h"
#include "header/strings.h"
#include "header/interrupt.h"

void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    intr_disable();
    puts("\n KERNEL PANIC!!!\n");
    puts("File: ");
    puts(filename);
    puts("\nLine: ");
    print_num32(line);
    puts("\nFunc: ");
    puts(func);
    puts("\nCondition: ");
    puts(condition);
    puts("\n");

    dump_regs();

    while (1)
        ;
}

void dump_regs()
{
    puts("Dumping registers...\n");
    uint32_t reg;
    uint16_t seg_reg; // 用于段寄存器

    // 通用寄存器
    const char *regs[] = {"EAX", "EBX", "ECX", "EDX", "ESP", "EBP", "ESI", "EDI"};
    for (int i = 0; i < 8; i++)
    {
        puts(regs[i]);
        puts(": ");
        switch (i)
        {
        case 0:
            asm volatile("movl %%eax, %0" : "=r"(reg));
            break;
        case 1:
            asm volatile("movl %%ebx, %0" : "=r"(reg));
            break;
        case 2:
            asm volatile("movl %%ecx, %0" : "=r"(reg));
            break;
        case 3:
            asm volatile("movl %%edx, %0" : "=r"(reg));
            break;
        case 4:
            asm volatile("movl %%esp, %0" : "=r"(reg));
            break;
        case 5:
            asm volatile("movl %%ebp, %0" : "=r"(reg));
            break;
        case 6:
            asm volatile("movl %%esi, %0" : "=r"(reg));
            break;
        case 7:
            asm volatile("movl %%edi, %0" : "=r"(reg));
            break;
        }
        print_num32_hex(reg);
        puts("\n");
    }

    // EIP（特殊处理）
    puts("EIP: ");
    asm volatile("call next_label");
    asm volatile("next_label: pop %0" : "=r"(reg) : : "memory");
    print_num32_hex(reg);
    puts("\n");

    // EFLAGS（特殊处理）
    puts("EFLAGS: ");
    asm volatile("pushfl");
    asm volatile(" pop %0" : "=r"(reg) : : "memory");
    print_num32_hex(reg);
    puts("\n");

    // 段寄存器（16位处理）
    const char *seg_regs[] = {"CS", "DS", "ES", "FS", "GS", "SS"};
    for (int i = 0; i < 6; i++)
    {
        puts(seg_regs[i]);
        puts(": ");
        switch (i)
        {
        case 0:
            asm volatile("movw %%cs, %0" : "=r"(seg_reg));
            break;
        case 1:
            asm volatile("movw %%ds, %0" : "=r"(seg_reg));
            break;
        case 2:
            asm volatile("movw %%es, %0" : "=r"(seg_reg));
            break;
        case 3:
            asm volatile("movw %%fs, %0" : "=r"(seg_reg));
            break;
        case 4:
            asm volatile("movw %%gs, %0" : "=r"(seg_reg));
            break;
        case 5:
            asm volatile("movw %%ss, %0" : "=r"(seg_reg));
            break;
        }
        print_num32_hex(seg_reg);
        puts("\n");
    }
}