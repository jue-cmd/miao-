#ifndef __KERNEL_GLOBOAL_H
#define __KERNEL_GLOBOAL_H

#include "stdint.h"

#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE ((1 << 3) | (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA ((1 << 3) | (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS ((1 << 3) | (TI_GDT << 2) + RPL0)

#define IDT_DESC_P 1

#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3

#define IDT_DESC_32_TYPE 0xE // 32位的中断门
#define IDT_DESC_16_TYPE 0x6

#define PG_P 1
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

#define IDT_DESC_ATTR_DPL0 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)

#define IDT_DESC_ATTR_DPL3 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)

#endif
