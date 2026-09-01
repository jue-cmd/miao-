#include "header/debug.h"
#include "header/stdint.h"
#include "header/strings.h"
#include "header/interrupt.h"

#define STACK_MAX_FRAMES 16
/* 内核栈大致范围：loader 栈映射到高半区后仍偏低，主线程栈约在 0xc009f000 */
#define STACK_LO 0xc0000000u
#define STACK_HI 0xc0100000u

static int stack_ptr_sane(uint32_t p)
{
    return p >= STACK_LO && p < STACK_HI && (p % 4u) == 0u;
}

void dump_stack_from(uint32_t ebp)
{
    uint32_t frame = ebp;
    int i;

    puts("Call stack (ebp chain, ret addrs):\n");
    puts("(map with: nm -n build/kernel.bin | less)\n");

    for (i = 0; i < STACK_MAX_FRAMES; i++)
    {
        uint32_t ret;
        uint32_t next;

        if (!stack_ptr_sane(frame) || !stack_ptr_sane(frame + 4))
        {
            puts("  <stop: bad ebp ");
            print_num32_hex(frame);
            puts(">\n");
            break;
        }

        next = *(uint32_t *)frame;
        ret = *(uint32_t *)(frame + 4);

        puts("  #");
        print_num32((uint32_t)i);
        puts("  ebp=");
        print_num32_hex(frame);
        puts("  ret=");
        print_num32_hex(ret);
        puts("\n");

        /* 栈向低地址增长，上一帧 ebp 应更高；相等或回绕则结束 */
        if (next <= frame)
        {
            break;
        }
        if (!stack_ptr_sane(next))
        {
            puts("  <stop: next ebp out of range ");
            print_num32_hex(next);
            puts(">\n");
            break;
        }
        frame = next;
    }
}

void dump_stack(void)
{
    uint32_t ebp;
    asm volatile("movl %%ebp, %0" : "=r"(ebp));
    dump_stack_from(ebp);
}

void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    uint32_t panic_ebp;

    /* 在破坏寄存器 / 再调其它函数之前先保住 ebp，供回溯 */
    asm volatile("movl %%ebp, %0" : "=r"(panic_ebp));

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

    dump_stack_from(panic_ebp);
    dump_regs();

    while (1)
        ;
}

void dump_regs(void)
{
    puts("Dumping registers...\n");
    uint32_t reg;
    uint16_t seg_reg;

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

    puts("EIP: ");
    asm volatile("call next_label");
    asm volatile("next_label: pop %0" : "=r"(reg) : : "memory");
    print_num32_hex(reg);
    puts("\n");

    puts("EFLAGS: ");
    asm volatile("pushfl");
    asm volatile(" pop %0" : "=r"(reg) : : "memory");
    print_num32_hex(reg);
    puts("\n");

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
