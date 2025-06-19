#include "header/init.h"
#include "header/strings.h"
#include "header/interrupt.h"
#include "device/timer.h"
void init_all()
{
    puts("init_all\n");
    idt_init();
    timer_init();
}