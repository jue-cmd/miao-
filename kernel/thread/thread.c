#include "thread.h"
#include "header/stdint.h"
#include "header/strings.h"
#include "header/globoal.h"
#include "header/memory.h"
#include "header/interrupt.h"
#include "header/debug.h"
#include "header/list.h"
#include "device/timer.h"

#define PG_SIZE 4096

struct task_struct *main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_node *thread_tag;

extern void switch_to(struct task_struct *cur, struct task_struct *next);

task_struct *running_thread(void)
{
    uint32_t esp;
    asm("mov %%esp, %0" : "=g"(esp));
    return (struct task_struct *)(esp & 0xfffff000);
}

static void kernel_thread(thread_func *function, void *func_arg)
{
    intr_enable();
    function(func_arg);
}

void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->func = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx =
        kthread_stack->esi = kthread_stack->edi = 0;
}

void init_thread(task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(struct task_struct));
    strcpy(pthread->name, name);
    if (pthread == main_thread)
    {
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x11451419;
}

task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{
    struct task_struct *pthread = get_kernel_pages(1);
    init_thread(pthread, name, prio);
    thread_create(pthread, function, func_arg);

    ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
    list_append(&thread_ready_list, &pthread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &pthread->all_list_tag));
    list_append(&thread_all_list, &pthread->all_list_tag);

    return pthread;
}

static void make_main_thread(void)
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct task_struct *cur = running_thread();

    if (cur->status == TASK_RUNNING)
    {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }

    if (list_empty(&thread_ready_list))
    {
        return;
    }

    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next =
        elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}

/* 由 timer_register_hook 动态挂到时钟中断上 */
static void thread_timer_tick(void)
{
    struct task_struct *cur_thread = running_thread();
    ASSERT(cur_thread->stack_magic == 0x11451419);

    cur_thread->elapsed_ticks++;
    if (cur_thread->ticks == 0)
    {
        schedule();
    }
    else
    {
        cur_thread->ticks--;
    }
}

void thread_init(void)
{
    puts("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    ASSERT(timer_register_hook(thread_timer_tick) == 0);
    puts("thread_init done\n");
}
