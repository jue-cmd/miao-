#include "timer.h"
#include "../header/io.h"
#include "../header/strings.h"
#include "../header/stdint.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTR0_VALUE (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTR0_PORT 0x40
#define COUNTR0_NO 0
#define COUNTR_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

/*
设置计数器
*/
static void frequency_setter(uint8_t counter_port,
                             uint8_t counter_no,
                             uint8_t rwl,
                             uint8_t counter_mode,
                             uint16_t counter_value)
{
    outb(PIT_CONTROL_PORT,
         (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1)); // 设置计数器工作模式
    outb(counter_port, (uint8_t)counter_value);                      // 先写低八位
    outb(counter_port, (uint8_t)(counter_value >> 8));               // 再写高八位
}

void timer_init()
{
    frequency_setter(COUNTR0_PORT, COUNTR0_NO, READ_WRITE_LATCH, COUNTR_MODE, COUNTR0_VALUE);
    puts("timer init success\n");
}