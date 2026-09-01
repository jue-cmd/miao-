#include "timer.h"
#include "../header/io.h"
#include "../header/strings.h"
#include "../header/stdint.h"
#include "../header/interrupt.h"
#include "../header/debug.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTR0_VALUE (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTR0_PORT 0x40
#define COUNTR0_NO 0
#define COUNTR_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

uint32_t ticks;

static timer_hook_t timer_hooks[TIMER_HOOK_MAX];
static uint32_t timer_hook_cnt;

static void frequency_setter(uint8_t counter_port,
                             uint8_t counter_no,
                             uint8_t rwl,
                             uint8_t counter_mode,
                             uint16_t counter_value)
{
    outb(PIT_CONTROL_PORT,
         (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    outb(counter_port, (uint8_t)counter_value);
    outb(counter_port, (uint8_t)(counter_value >> 8));
}

int timer_register_hook(timer_hook_t hook)
{
    enum intr_status old;

    ASSERT(hook != NULL);
    if (timer_hook_cnt >= TIMER_HOOK_MAX)
    {
        return -1;
    }

    old = intr_disable();
    timer_hooks[timer_hook_cnt++] = hook;
    intr_set_status(old);
    return 0;
}

/* 时钟中断只负责计数 + 分发 hook，业务逻辑由外部动态插入 */
static void intr_timer_handler(uint8_t vec_nr)
{
    uint32_t i;
    (void)vec_nr;

    ticks++;
    for (i = 0; i < timer_hook_cnt; i++)
    {
        timer_hooks[i]();
    }
}

void timer_init(void)
{
    puts("timer_init start\n");
    frequency_setter(COUNTR0_PORT, COUNTR0_NO, READ_WRITE_LATCH,
                     COUNTR_MODE, COUNTR0_VALUE);
    /* 动态插入 IRQ0 处理函数，并自动打开 PIC 屏蔽位 */
    irq_register(0, intr_timer_handler);
    puts("timer_init done\n");
}
