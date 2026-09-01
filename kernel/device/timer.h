#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H

#include "../header/stdint.h"

#define TIMER_HOOK_MAX 8

typedef void (*timer_hook_t)(void);

void timer_init(void);

/** 动态插入时钟滴答回调（在 IRQ0 处理里按注册顺序调用） */
int timer_register_hook(timer_hook_t hook);

extern uint32_t ticks;

#endif
