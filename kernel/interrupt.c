#include "header/interrupt.h"
#include "header/stdint.h"
#include "header/globoal.h"
#include "header/strings.h"
#include "header/io.h"
#include "header/debug.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1
#define EFLAGS_IF 0x00000200

static void make_idt_desc(gate_desc *p_gdesc, uint8_t attr, intr_handler_t function);
static gate_desc idt[IDT_DESC_CNT];

extern intr_handler_t intr_entry_table[IDT_DESC_CNT];
intr_handler_t idt_table[IDT_DESC_CNT];
char *intr_name[IDT_DESC_CNT];

static void pic_init(void)
{
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);

    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);

    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);

    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);

    /* 初始全部屏蔽，由 register_handler / irq_register 按需打开 */
    outb(PIC_M_DATA, 0xff);
    outb(PIC_S_DATA, 0xff);

    puts("pic init done\n");
}

static void irq_unmask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC_M_DATA;
    }
    else if (irq < 16)
    {
        port = PIC_S_DATA;
        irq -= 8;
    }
    else
    {
        return;
    }

    value = (uint8_t)(inb(port) & ~(1u << irq));
    outb(port, value);
}

static void make_idt_desc(gate_desc *p_gdesc, uint8_t attr, intr_handler_t function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    puts("idt_desc_init done\n");
}

static void general_intr_handler(uint8_t vec_nr)
{
    if (vec_nr == 0x27 || vec_nr == 0x2f)
    {
        return;
    }
    puts("int vector: 0x");
    print_num32_hex(vec_nr);
    putc('\n');
}

static void exception_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

intr_handler_t get_handler(uint8_t vector_no)
{
    if (vector_no >= IDT_DESC_CNT)
    {
        return NULL;
    }
    return idt_table[vector_no];
}

intr_handler_t register_handler(uint8_t vector_no, intr_handler_t function)
{
    enum intr_status old;
    intr_handler_t prev;

    ASSERT(vector_no < IDT_DESC_CNT);
    ASSERT(function != NULL);

    old = intr_disable();
    prev = idt_table[vector_no];
    idt_table[vector_no] = function;

    /* IRQ 向量：打开 PIC 对应屏蔽位，实现“插入即生效” */
    if (vector_no >= IRQ_BASE)
    {
        irq_unmask((uint8_t)(vector_no - IRQ_BASE));
    }

    intr_set_status(old);
    return prev;
}

intr_handler_t irq_register(uint8_t irq, intr_handler_t function)
{
    ASSERT(irq < 16);
    return register_handler((uint8_t)(IRQ_BASE + irq), function);
}

void idt_init(void)
{
    puts("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    {
        uint64_t idt_operand =
            ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
        print_num32_hex((uint32_t)idt_operand);
        asm volatile("lidt %0" : : "m"(idt_operand));
    }
    puts("idt_init done\n");
}

enum intr_status intr_enable(void)
{
    enum intr_status old_status;
    if (intr_get_status() == INTR_ON)
    {
        return INTR_ON;
    }
    old_status = INTR_OFF;
    asm volatile("sti");
    return old_status;
}

enum intr_status intr_disable(void)
{
    enum intr_status old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory");
        return old_status;
    }
    return INTR_OFF;
}

enum intr_status intr_set_status(enum intr_status status)
{
    return status == INTR_ON ? intr_enable() : intr_disable();
}

enum intr_status intr_get_status(void)
{
    uint32_t eflags = 0;
    asm volatile("pushfl");
    asm volatile("popl %0" : "=g"(eflags));
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
