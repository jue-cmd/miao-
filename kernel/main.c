#include "header/print.h"
#include "header/strings.h"
#include "header/init.h"
#include "header/debug.h"
#include "header/memory.h"
#include "thread/thread.h"

void just_a_test(void *arg);
uint32_t feibo(uint32_t a);
void main()
{
    init_all();
    thread_start("just_a_test", 20, just_a_test, "just a test");
    puts("check point");
    print_num32(feibo(10));
    while (1)
        ;
}

uint32_t feibo(uint32_t a)
{
    if(a==0)
    {
        return 1;
    }
    if(a==1){
        return 1;
    }
    return feibo(a-1)+feibo(a-2);
}

void just_a_test(void *arg)
{
    puts(arg);
    print_num32(feibo(10));
    while(1);
    return;
}