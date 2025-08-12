#include "header/print.h"
#include "header/strings.h"
#include "header/init.h"
#include "header/debug.h"
#include "header/memory.h"
#include "thread/thread.h"

void just_a_test(void *arg);

void main()
{
    init_all();

    thread_start("just_a_test", 20, just_a_test, "just a test");
    puts("check point");
    while (1)
        ;
}

void just_a_test(void *arg)
{
    puts(arg);
    return;
}