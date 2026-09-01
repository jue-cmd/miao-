#ifndef __KERNEL_INTERRUPT_H__
#define __KERNEL_INTERRUPT_H__

#include "stdint.h"

#define IDT_DESC_CNT 0x21
#define IRQ0_VECTOR  0x20
#define IRQ_BASE     0x20

/* C 侧中断处理函数：入参为中断向量号 */
typedef void (*intr_handler_t)(uint8_t vec_nr);

/* 兼容旧名：汇编 idt_table 存的是函数指针 */
typedef intr_handler_t intr_handler;

typedef struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
} gate_desc;

void idt_init(void);

/**
 * 动态插入 / 替换某向量的处理函数。
 * vector_no: 0x00~0x20；若为 IRQ（>=0x20）会自动打开对应 PIC 屏蔽位。
 * 返回原先挂接的处理函数（便于链式包装）。
 */
intr_handler_t register_handler(uint8_t vector_no, intr_handler_t function);

/** 按 IRQ 号（0~15）注册，等价于 register_handler(IRQ_BASE + irq, ...) */
intr_handler_t irq_register(uint8_t irq, intr_handler_t function);

intr_handler_t get_handler(uint8_t vector_no);

enum intr_status
{
    INTR_ON,
    INTR_OFF
};

enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);

#endif
