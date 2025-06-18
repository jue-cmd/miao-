#ifndef __KERNEL_INTERRUPT_H__
#define __KERNEL_INTERRUPT_H__
#include "stdint.h"
typedef void * intr_handler;
typedef struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
} gate_desc;

static void idt_desc_init(void);
static void make_idt_desc(gate_desc *p_gdesc, uint8_t attr, intr_handler function);
void idt_init();


enum intr_status{
    INTR_ON,
    INTR_OFF
};

enum intr_status intr_get_status(void);
enum intr_status intr_set_status (enum intr_status);
enum intr_status intr_enable (void);
enum intr_status intr_disable (void);
#endif //__KERNEL_INTERRUPT_H__