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