#include "thread.h"
#include "../header/stdint.h"
#include "../header/strings.h"
#include "../header/globoal.h"
#include "../header/memory.h"

#define PG_SIZE 4096

static void kernel_thread(thread_func *function, void *func_arg)
{
    function(func_arg);
}
void thread_create(struct task_struct *pthread, thread_func *function, void *func_arg)
{
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->func = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

void init_thread(struct task_struct *pthread, char *name, int32_t prio)
{
    memset(pthread, 0, sizeof(struct task_struct));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x11451419; // 就是觉得这个数字很吉利
}

/**
 * @function: thread_start
 * @brief: 创建一个内核线程
 * @param name: 线程名
 * @param prio: 线程优先级
 * @param function: 线程函数
 * @param func_arg: 线程函数参数
 * @return: struct task_struct *
 */
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{
    struct task_struct *pthread = get_kernel_pages(1);
    init_thread(pthread, name, prio);
    thread_create(pthread, function, func_arg);

    /* * 关键修正：将 esp 切换到我们构造好的线程栈顶。
     * 此时 pthread->self_kstack 必须指向线程栈中存储寄存器 esi 的位置。
     */
    asm volatile (
        "movl %0, %%esp; "       // 切换栈指针到新线程的栈
        "pop %%ebp; "            // 弹出预留的寄存器环境
        "pop %%ebx; "
        "pop %%edi; "
        "pop %%esi; "
        "ret"                    // 弹出 thread_create 压入的 function 地址并跳转
        : 
        : "g" (pthread->self_kstack) 
        : "memory"
    );

    return pthread; // 实际上这行永远不会被执行
}
