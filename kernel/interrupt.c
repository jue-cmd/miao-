#include "header/interrupt.h"
#include "header/stdint.h"
#include "header/globoal.h"
#include "header/strings.h"
#include "header/io.h"

#define IDT_DESC_CNT 0x21

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

static void make_idt_desc(gate_desc *p_gdesc, uint8_t attr, intr_handler function);
static gate_desc idt[IDT_DESC_CNT]; // 创建中断描述符数组

extern intr_handler intr_entry_table[IDT_DESC_CNT]; // 中断处理函数的入口地址,定义在kernel.asm中
intr_handler idt_table[IDT_DESC_CNT];
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

    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    puts("pic init done\n");
}

/*
 *  创建中断门描述符
 */
static void make_idt_desc(gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

/**
 * 初始化中断描述符表
 */

static void idt_desc_init(void)
{
    for (int i = 0; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    puts("idt_desc_init done\n");
}

static void general_intr_handler(uint8_t vec_nr)
{
    // 忽略伪中断
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
    // intr_name[15] 第 15 项是 intel 保留项,未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}
void idt_init()
{
    puts("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    print_num32_hex((uint32_t)idt_operand);

    asm volatile("lidt %0" : : "m"(idt_operand));
    puts("idt_init done\n");
}