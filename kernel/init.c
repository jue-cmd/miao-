#include "header/init.h"
#include "header/strings.h"
#include "header/interrupt.h"
#include "device/timer.h"
#include "header/memory.h"
#include "thread/thread.h"

void init_all(void)
{
    puts("init_all\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init(); /* 注册时钟中断；开中断放在 main 里、建好线程之后 */
}
