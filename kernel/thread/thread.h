#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "header/stdint.h"
#include "header/list.h"

typedef void thread_func(void *);

enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

typedef struct intr_stack
{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
} intr_stack;

typedef struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip)(thread_func *func, void *func_arg);

    void (*unused_retaddr)(void);
    thread_func *func;
    void *func_arg;
} thread_stack;

typedef struct task_struct
{
    uint32_t *self_kstack;
    enum task_status status;
    char name[16];
    uint8_t priority;
    uint8_t ticks;
    uint32_t elapsed_ticks;
    list_node general_tag;
    list_node all_list_tag;
    uint32_t *pgdir;
    uint32_t stack_magic;
} task_struct;

task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg);
void init_thread(task_struct *pthread, char *name, int prio);
void thread_create(task_struct *pthread, thread_func function, void *func_arg);
void thread_init(void);
void schedule(void);
task_struct *running_thread(void);

#endif
